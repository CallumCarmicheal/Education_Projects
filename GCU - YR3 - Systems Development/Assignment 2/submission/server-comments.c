// Cwk2: server.c - multi-threaded server using readn() and writen()

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <sys/utsname.h>
#include <limits.h>
#include <dirent.h>
#include <signal.h>
#include <time.h>
#include "rdwrn.h"

#ifdef DEBUG_SERVER
    #define DBG_PRINT printf
#else
    #define DBG_PRINT(...)
#endif

// ---- GLOBAL VARIABLES ----

time_t time_begin; 				// clock

// ---- PROTOTYPES ----

// thread function
void *client_handler(void *);

typedef enum _PACKET_TYPES {
    PT_ERROR_UNKNOWN = 0, // UNKNOWN

    PT_RANDOM,
    PT_UNAME,
    PT_UPLOAD,
    PT_DOWNLOAD,
	PT_BROWSER,

    PT_EXIT
} PACKET_TYPES;

struct NETWORK_INTERFACE {
    struct ifaddrs* interface;          // malloc
    char            host[NI_MAXHOST];
};

void send_hello(int);
void server_endpoint(int, char*, size_t);
void get_interface(char*, struct NETWORK_INTERFACE**);

void _exit(int x) {
	time_t end;
	time(&end);

	int seconds = (int)difftime(end, time_begin);	
	int h = seconds / 3600;
	int m = (seconds / 60) % 60;
	int s = (seconds % 60);

	printf("\nSERVER ONLINE TIME: %02d:%02d:%02d.\n", h,m,s);
	exit(x);
}

void signalHandler() {
    _exit(EXIT_SUCCESS);
} 

// you shouldn't need to change main() in the server except the port number
int main(void) {
	// Get the current time and setup the signal handler
   	time(&time_begin);
	signal(SIGINT, signalHandler);

	int listenfd = 0, connfd = 0;

    struct sockaddr_in serv_addr;
    struct sockaddr_in client_addr;
    socklen_t socksize = sizeof(struct sockaddr_in);
    
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(50031);

    bind(listenfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));

    if (listen(listenfd, 10) == -1) {
		perror("Failed to listen");
		_exit(EXIT_FAILURE);
    }
    // end socket setup

    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    while (1) {
	    printf("Waiting for a client to connect...\n");
	    connfd = accept(listenfd, (struct sockaddr *) &client_addr, &socksize);
	    printf("Connection accepted... \n");

	    pthread_t sniffer_thread;
        // third parameter is a pointer to the thread function, fourth is its actual parameter
        if (pthread_create(&sniffer_thread, NULL, client_handler, (void *) &connfd) < 0) {
            perror("could not create thread");
            _exit(EXIT_FAILURE);
        }

		// We have assigned our handler
	    pthread_detach(sniffer_thread);
        printf("Handler assigned\n");
    }

    // never reached...
    // ** should include a signal handler to clean up
    _exit(EXIT_SUCCESS);
} // end main()

void request_random(int s) {
    // A place to store the random no's
    int random[5];

    // Setup the seeding
    time_t t;
    srand((unsigned) time(&t));

    for( int x = 0 ; x < 5 ; x++ )
        random[x] = (int)(.0 + 1000.0 * rand() / RAND_MAX);

    // Send the array
    printf("recv random request (%d)\n", s);
    writen(s, (unsigned char *) &random, sizeof(random));
}

void request_uname(int s) {
    struct utsname buffer;
    int status = 0;

    printf("uname request (%d)\n", s);

    errno = 0;
    if (uname(&buffer) != 0) {
        // Write errno to the client to state an error
        status = errno;
        writen(s, (unsigned char *) &status, sizeof(int));

        printf("uname request failed for (%d), error = %d\n", s, status);
        return;
    }

    // Write SUCCESS to the socket
    writen(s, (unsigned char *) &status, sizeof(int));

    // Write the utsname struct to the client
    writen(s, (unsigned char *) &buffer, sizeof(buffer));
}

void request_upload(int s) {
	printf("upload request (%d)\n", s);
	ssize_t r = 0;
	ssize_t fns = 0;
	size_t totalFS = 0;

	// FILE_STATUS, -2 WRITE_ERROR, -1 ILLEGAL_PATH, 0 UNKNOWN_ERROR, 1 SUCCESS
    int response = 0;
    
	char* fileName = NULL;
    unsigned char* buffer = NULL;

    // send fmt:{fileNameSize[int]}
	// 			{fileName[fileNameSize]}
	// 			{fsize[long]}
	// 			{buffer[unsigned char:fsize]}
	
	// fileNameSize[int]
	size_t fileNameSize;
	if ((r=readn(s, (unsigned char *) &fileNameSize, sizeof(size_t))) == -1)
    	{ printf("failed to read fileNameSize\n"); goto cleanup; }//goto cleanup;	


	// fileName[fileNameSize]
	fileName = malloc(fileNameSize * sizeof(char) + 1); // +1 plus null term
	memset (fileName, 0, fileNameSize * sizeof(char) + 1);
		// Reads up to fileNameSize but does not include the final null terminator
	if ((fns = readn(s, (unsigned char *) fileName, fileNameSize * sizeof(char))) == -1)
    	{ printf("failed to read fileName\n"); goto cleanup; }//goto cleanup;
	
	// fsize[long]
	long fsize;
	if ((r=readn(s, (unsigned char *) &fsize, sizeof(long))) == -1)
    	{ printf("failed to read fsize\n"); goto cleanup; }//goto cleanup;
	
	// buffer
	totalFS = fsize * sizeof(unsigned char);
    buffer = malloc(totalFS);
    memset(buffer, 0, totalFS);
    if ((r=readn(s, buffer, totalFS)) == -1)
    	{ printf("failed to read buffer\n"); goto cleanup; }//goto cleanup;

	// If we received the wrong size of fileName then throw an error
	if (fns != fileNameSize * sizeof(char)) {
		printf("fileName != r, %zd != %zd\n", fns, fileNameSize * sizeof(char));
		goto cleanup;	
	} 

	// A little bit of security, detect any ../
	if (strstr(fileName, "../") != NULL) {
        printf("ILLEGAL PATH DETECTED during upload (%d)\n",s);
		response = -1;

        writen(s, (unsigned char *) &response, sizeof(int));
        goto cleanup;
    }

	// Now we need to check if the file exists
	char uploadDir[PATH_MAX];
	char uploadFile_Temp[PATH_MAX];
	char uploadFile[PATH_MAX];
	memset(uploadDir, 0, sizeof(uploadDir));
	memset(uploadFile_Temp, 0, sizeof(uploadDir));
	memset(uploadFile, 0, sizeof(uploadDir));

	realpath("./upload", uploadDir);

	// Check if our directory exists, if not then create it.
    struct stat st = {0};
    if (stat(uploadDir, &st) == -1) {
        mkdir(uploadDir, 0700);
		printf("Created sub directory for uploads, 'upload'.\n");
	}

	// Setup the file output path
	// (this needs to be done after the dir creation or
	//  realpath will cap to .../upload)
	strcat(uploadFile_Temp, uploadDir);
    strcat(uploadFile_Temp, "/");
	strcat(uploadFile_Temp, fileName);
	realpath(uploadFile_Temp, uploadFile);
	uploadFile[PATH_MAX-1] = 0; // CAP The max path

    // Write the file
    FILE* write_ptr;
    write_ptr = fopen(uploadFile,"wb");
   		
	if (write_ptr == NULL) {
		response = 0; // UNKNOWN_ERROR
		printf("Failed to create file pointer... write_ptr == NULL on (%d)\n| dir = {%s}\n| file = {%s}\n",
				s, uploadDir, uploadFile);
	} else {
		size_t status;
		if ((status = fwrite(buffer, totalFS, 1, write_ptr)) != 1) {
			// WRITE_ERROR
			response = -2; 
			printf("failed to write buffer on (%d), error = %zd\n", s, status);
		}
		else {
			response = 1; // SUCCESS
			printf("fileName = {%s} [%ld bytes], DOWNLOADED from (%d)\n", fileName, fsize, s);
		}

		fclose(write_ptr);
	}
		
    // Return the response
    writen(s, (unsigned char *) &response, sizeof(int));

cleanup:	// Clean up our dynamic memory and any other pointers
    if (fileName != NULL)   free(fileName);
	if (buffer != NULL)     free(buffer);
}

void request_download(int s) {
	printf("download requested from (%d)\n", s);

	int status = 0;

	// Setup the file paths
	//   CLIENT         | CONCAT         | OUTPUT PATH
	char file[PATH_MAX], _temp[PATH_MAX], path[PATH_MAX];
	memset(_temp, 0, sizeof(_temp));

	// Get the file name
	if ( readn(s, (unsigned char*) &file, PATH_MAX) == -1 )
		return;

	// Get the real path of the file
	strcat(_temp, "./upload/");
	strcat(_temp, file);
	realpath(_temp, path);

	// Attempt to read the file
	FILE *f = fopen(path, "rb");

	// If we failed to open the file	
	if (f == NULL) {
		printf("failed to open {%s} from (%d)\n", path, s);
		
		// Tell the client we failed
		writen(s, (unsigned char *) &status, sizeof(int) );
		return;
	}
	
	// Display message of uploading the file
	printf("uploading {%s} to (%d)\n", file, s);
		
	fseek(f, 0, SEEK_END);	// seek to end of file
	long size = ftell(f);	// get current file size
	fseek(f, 0, SEEK_SET);	// seek back to beginning of fil

	// Read the file
	size_t bsize = size * sizeof(unsigned char);
	unsigned char* buffer = (unsigned char*) malloc(bsize);
	memset(buffer, 0, bsize);
	size_t result = fread (buffer,1,size,f);

	// Error checking: if we are not able to read the file.
	if (result != size) {
		// Tell the client we failed
		writen(s, (unsigned char *) &status, sizeof(int));
		printf("{%zu, %zu} failed to read in file {%s} to (%d)\n",result, size, file, s);
		free(buffer);
		fclose(f);
		return;
	}
	
	// Tell the client we are ready to send the file
	status = 1;
	if (writen(s, (unsigned char*)&status, sizeof(int)) == -1)
		goto cleanup;
	
	// Write the file to the client
	if (writen(s, (unsigned char*)&bsize, sizeof(size_t)) == -1) 
		goto cleanup;

	if (writen(s, buffer, bsize) == -1)
		goto cleanup;

	printf("done sending the file to client (%d)\n", s);

cleanup: // cleanup
	if (f != NULL) 		fclose(f);
	if (buffer != NULL) free(buffer);
}

void request_browser(int s) {
	// SHow the request
	printf("requesting files (%d)\n", s);	
		
	// Setup the directory
	DIR *d;
	struct dirent *dir;

	// Get the upload path and open the dir.
	char path[PATH_MAX];
	realpath("./upload", path);
	d = opendir(path);

	// Count the amount of files
	int fileCount = 0;
	while ( d != NULL && (dir = readdir(d)) != NULL ) {
		// Ignore ".", ".."
		if (strcmp(dir->d_name, ".") == 0
			|| strcmp(dir->d_name, "..") == 0) 
			continue;
		
		// Increment the file count
		fileCount++;
	}

	// Now we want to send the file count
	if (writen(s, (unsigned char *) &fileCount, sizeof(int)) == -1)
		{ if (d!=NULL) closedir(d); return; }; // failed to write, return ERROR.

	if (fileCount == 0) {
		printf("no files found in upload directory (%d)\n", s);
		if (d != NULL) closedir(d);

	} else {
		printf("files found = %d (%d)\n", fileCount, s);
	}

	// Now we reset the directory and then send all the files
	int x = 0;
	if (d != NULL) closedir(d);
	d = opendir(path);
	
	char file[PATH_MAX];

	while ( d != NULL && (dir = readdir(d)) != NULL ) {
		// Ignore ".", ".."
		if (strcmp(dir->d_name, ".") == 0
			|| strcmp(dir->d_name, "..") == 0) 
			continue;

			
		// If there was a dir change, we want to
		// accommodate this.
		if (x >= fileCount) { 
			printf("fileCount >= x, (%d >= %d)\n", fileCount, x);
			x++;
			continue; 
		}
		
		// Reset file
		memset(file, 0, sizeof(file));
		strcpy(file, dir->d_name);

		// Write the name to the stream
		if (writen(s, (unsigned char *) file, sizeof(file)) == -1) { 
			printf("Failed to write file to stream\n"); 
			return; 
		}
		
		// Next item
		x++;
	}

	// Close the directory
	closedir(d);
}

// thread function - one instance of each for each connected client
// this is where the do-while loop will go
void *client_handler(void *socket_desc) {
    //Get the socket descriptor
    int s = *(int *) socket_desc;

    //Send our header
    send_hello(s);

    int running = 1;
    while(running == 1) {
        // Get the packet request type
        PACKET_TYPES reqType;
        ssize_t recvn = readn(s, (unsigned char *) &reqType, sizeof(PACKET_TYPES));

        // -1 for error, and 0 for killed / forced exit (C-c).
        if (recvn == -1 || recvn == 0) {
            // There was a socket error, assume client closed or SIGPIPE
            running = 0;
            continue;
        }

        switch(reqType) {
            case PT_RANDOM:
                request_random(s);
                break;

            case PT_UNAME:
                request_uname(s);
                break;
		
			case PT_DOWNLOAD:
				request_download(s);
				break;

			case PT_UPLOAD:
				request_upload(s);
				break;
			
			case PT_BROWSER:
				request_browser(s);
				break;
					
            case PT_EXIT:
                running = 0;
                break;

            // Ignore unexpected information
            default: break;
        }
    }

    printf("Thread %lu exiting\n", (unsigned long) pthread_self());

    // always clean up sockets gracefully
    shutdown(s, SHUT_RDWR);
    close(s);

    return 0;
}  // end client_handler()

// how to send a string
void send_hello(int socket) {
	// Allocate our strings and Get our endpoint
    char szFormatString[] = "%s - hello SP (Callum Carmicheal, S1829709)";
	char szHelloString[256];
	char szEndpoint[47]; // Support for ipv6, 46 size + \n

	// Get our ip endpoint
	server_endpoint(socket, szEndpoint, sizeof(szEndpoint));
	
	// Format the string
	snprintf(szHelloString, sizeof(szHelloString), szFormatString, szEndpoint);

	// Write the string to the client
    size_t n = strlen(szHelloString) + 1;
    writen(socket, (unsigned char *) &n, sizeof(size_t));
    writen(socket, (unsigned char *) szHelloString, n);
} // end send_hello()

void server_endpoint(int socket, char* buffer, size_t bufferSize) {
    // Get the network information for eth0 or wifi0 (WIFI0 for my pc, ETH0 for VM)
    struct NETWORK_INTERFACE* net = NULL;
    get_interface("wifi0", &net);

    // Check if we failed to find wifi or eth0
    if (net == NULL) {
        // If we cant get the wifi0 then try eth0
        get_interface("eth0", &net);

        // If we can find the information for eth0 just jump to the copy section
        if (net != NULL)
            goto setInformation;

        // Display our warning
        printf("[WARNING] Failed to get network for eth0 or wifi0\n");
        snprintf(buffer, bufferSize, "%s", "(UNKNOWN-IP)");
        return;
    }

setInformation:
    // Copy our network host
    snprintf(buffer, bufferSize, "%s", net->host);

    free(net->interface); // Free our interface
    free(net); // Free our network
}

void get_interface(char* interface, struct NETWORK_INTERFACE** pInterface) {
    // Setup variables
    struct ifaddrs *plInterfaces, *pIf;
    int s;
    char host[NI_MAXHOST];

    // Check if we dont have any interfaces
    if (getifaddrs(&plInterfaces) == -1) {
        perror("[WARNING] Unable to find any interfaces");
        return;
    }

    for (pIf = plInterfaces; pIf != NULL; pIf = pIf->ifa_next) {
        // If the interface is not online
        if (pIf->ifa_addr == NULL)
            continue;

        // Get the socket information
        s = getnameinfo(pIf->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

        // Check if this is our wanted interface
        int isWantedInterface = ( strcmp(pIf->ifa_name, interface) == 0 );
        int isCorrectFamily   = ( pIf->ifa_addr->sa_family == AF_INET );

        // Return the interface if is correct
        if ( isWantedInterface && isCorrectFamily ) {
            if (s != 0) {
                // Print the error
                fprintf(stderr, "[WARNING] getnameinfo(%s) failed: %s\n", interface, gai_strerror(s));
                return;
            }

            // Debug
            DBG_PRINT("\tInterface: %s\n", pIf->ifa_name);
            DBG_PRINT("\t  Address: %s\n", host);

            // Allocate the new network interface
            *pInterface = malloc(sizeof(struct NETWORK_INTERFACE));
            memset(*pInterface, 0, sizeof(struct NETWORK_INTERFACE));

            // Allocate the new interface and copy the
            (*pInterface)->interface = malloc(sizeof(struct ifaddrs));
            memcpy((*pInterface)->interface, pIf, sizeof(struct ifaddrs));

            // Copy the host to the interface
            strcpy((*pInterface)->host, host);
        }
    }

    // Free up our interfaces
    freeifaddrs(plInterfaces);
}
