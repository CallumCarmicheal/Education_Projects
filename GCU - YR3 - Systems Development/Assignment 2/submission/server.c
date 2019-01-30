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

time_t time_begin;
void *client_handler(void *);

typedef enum _PACKET_TYPES {
    PT_ERROR_UNKNOWN = 0,

    PT_RANDOM,
    PT_UNAME,
    PT_UPLOAD,
    PT_DOWNLOAD,
	PT_BROWSER,

    PT_EXIT
} PACKET_TYPES;

struct NETWORK_INTERFACE {
    struct ifaddrs* interface;
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

int main(void) {
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

    puts("Waiting for incoming connections...");
    while (1) {
	    printf("Waiting for a client to connect...\n");
	    connfd = accept(listenfd, (struct sockaddr *) &client_addr, &socksize);
	    printf("Connection accepted... \n");

	    pthread_t sniffer_thread;
        if (pthread_create(&sniffer_thread, NULL, client_handler, (void *) &connfd) < 0) {
            perror("could not create thread");
            _exit(EXIT_FAILURE);
        }

	    pthread_detach(sniffer_thread);
        printf("Handler assigned\n");
    }

    _exit(EXIT_SUCCESS);
}

void request_random(int s) {
    int random[5];

    time_t t;
    srand((unsigned) time(&t));

    for( int x = 0 ; x < 5 ; x++ )
        random[x] = (int)(.0 + 1000.0 * rand() / RAND_MAX);

    printf("recv random request (%d)\n", s);
    writen(s, (unsigned char *) &random, sizeof(random));
}

void request_uname(int s) {
    struct utsname buffer;
    int status = 0;

    printf("uname request (%d)\n", s);

    errno = 0;
    if (uname(&buffer) != 0) {
        status = errno;
        writen(s, (unsigned char *) &status, sizeof(int));

        printf("uname request failed for (%d), error = %d\n", s, status);
        return;
    }

    writen(s, (unsigned char *) &status, sizeof(int));
    writen(s, (unsigned char *) &buffer, sizeof(buffer));
}

void request_upload(int s) {
	printf("upload request (%d)\n", s);
	ssize_t r = 0;
	ssize_t fns = 0;
	size_t totalFS = 0;

    int response = 0;
    
	char* fileName = NULL;
    unsigned char* buffer = NULL;
	size_t fileNameSize;

	if ((r=readn(s, (unsigned char *) &fileNameSize, sizeof(size_t))) == -1)
    	{ printf("failed to read fileNameSize\n"); goto cleanup; }

	fileName = malloc(fileNameSize * sizeof(char) + 1);
	memset (fileName, 0, fileNameSize * sizeof(char) + 1);
	if ((fns = readn(s, (unsigned char *) fileName, fileNameSize * sizeof(char))) == -1)
    	{ printf("failed to read fileName\n"); goto cleanup; }
	
	long fsize;
	if ((r=readn(s, (unsigned char *) &fsize, sizeof(long))) == -1)
    	{ printf("failed to read fsize\n"); goto cleanup; }
	
	totalFS = fsize * sizeof(unsigned char);
    buffer = malloc(totalFS);
    memset(buffer, 0, totalFS);
    if ((r=readn(s, buffer, totalFS)) == -1)
    	{ printf("failed to read buffer\n"); goto cleanup; }

	if (fns != fileNameSize * sizeof(char)) {
		printf("fileName != r, %zd != %zd\n", fns, fileNameSize * sizeof(char));
		goto cleanup;	
	} 

	if (strstr(fileName, "../") != NULL) {
        printf("ILLEGAL PATH DETECTED during upload (%d)\n",s);
		response = -1;

        writen(s, (unsigned char *) &response, sizeof(int));
        goto cleanup;
    }

	char uploadDir[PATH_MAX];
	char uploadFile_Temp[PATH_MAX];
	char uploadFile[PATH_MAX];
	memset(uploadDir, 0, sizeof(uploadDir));
	memset(uploadFile_Temp, 0, sizeof(uploadDir));
	memset(uploadFile, 0, sizeof(uploadDir));

	realpath("./upload", uploadDir);

    struct stat st = {0};
    if (stat(uploadDir, &st) == -1) {
        mkdir(uploadDir, 0700);
		printf("Created sub directory for uploads, 'upload'.\n");
	}

	strcat(uploadFile_Temp, uploadDir);
    strcat(uploadFile_Temp, "/");
	strcat(uploadFile_Temp, fileName);
	realpath(uploadFile_Temp, uploadFile);
	uploadFile[PATH_MAX-1] = 0;
	
    FILE* write_ptr;
    write_ptr = fopen(uploadFile,"wb");
   		
	if (write_ptr == NULL) {
		response = 0;
		printf("Failed to create file pointer... write_ptr == NULL on (%d)\n| dir = {%s}\n| file = {%s}\n",
				s, uploadDir, uploadFile);
	} else {
		size_t status;
		if ((status = fwrite(buffer, totalFS, 1, write_ptr)) != 1) {
			response = -2; 
			printf("failed to write buffer on (%d), error = %zd\n", s, status);
		}
		else {
			response = 1;
			printf("fileName = {%s} [%ld bytes], DOWNLOADED from (%d)\n", fileName, fsize, s);
		}

		fclose(write_ptr);
	}
	
    writen(s, (unsigned char *) &response, sizeof(int));

cleanup:
    if (fileName != NULL)   free(fileName);
	if (buffer != NULL)     free(buffer);
}

void request_download(int s) {
	printf("download requested from (%d)\n", s);

	int status = 0;
	char file[PATH_MAX], _temp[PATH_MAX], path[PATH_MAX];
	memset(_temp, 0, sizeof(_temp));

	if ( readn(s, (unsigned char*) &file, PATH_MAX) == -1 )
		return;

	strcat(_temp, "./upload/");
	strcat(_temp, file);
	realpath(_temp, path);

	FILE *f = fopen(path, "rb");
	
	if (f == NULL) {
		printf("failed to open {%s} from (%d)\n", path, s);
		writen(s, (unsigned char *) &status, sizeof(int) );
		return;
	}
	
	printf("uploading {%s} to (%d)\n", file, s);
		
	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fseek(f, 0, SEEK_SET);

	size_t bsize = size * sizeof(unsigned char);
	unsigned char* buffer = (unsigned char*) malloc(bsize);
	memset(buffer, 0, bsize);
	size_t result = fread (buffer,1,size,f);

	if (result != size) {
		writen(s, (unsigned char *) &status, sizeof(int));
		printf("{%zu, %zu} failed to read in file {%s} to (%d)\n",result, size, file, s);
		free(buffer);
		fclose(f);
		return;
	}
	
	status = 1;
	if (writen(s, (unsigned char*)&status, sizeof(int)) == -1)
		goto cleanup;
	
	if (writen(s, (unsigned char*)&bsize, sizeof(size_t)) == -1) 
		goto cleanup;

	if (writen(s, buffer, bsize) == -1)
		goto cleanup;

	printf("done sending the file to client (%d)\n", s);

cleanup:
	if (f != NULL) 		fclose(f);
	if (buffer != NULL) free(buffer);
}

void request_browser(int s) {
	printf("requesting files (%d)\n", s);	
	DIR *d;
	struct dirent *dir;

	char path[PATH_MAX];
	realpath("./upload", path);
	d = opendir(path);

	int fileCount = 0;
	while ( d != NULL && (dir = readdir(d)) != NULL ) {
		if (strcmp(dir->d_name, ".") == 0
			|| strcmp(dir->d_name, "..") == 0) 
			continue;
			
		fileCount++;
	}

	if (writen(s, (unsigned char *) &fileCount, sizeof(int)) == -1)
		{ if (d!=NULL) closedir(d); return; };

	if (fileCount == 0) {
		printf("no files found in upload directory (%d)\n", s);
		if (d != NULL) closedir(d);

	} else {
		printf("files found = %d (%d)\n", fileCount, s);
	}

	int x = 0;
	if (d != NULL) closedir(d);
	d = opendir(path);
	
	char file[PATH_MAX];

	while ( d != NULL && (dir = readdir(d)) != NULL ) {
		if (strcmp(dir->d_name, ".") == 0
			|| strcmp(dir->d_name, "..") == 0) 
			continue;
		
		if (x >= fileCount) { 
			printf("fileCount >= x, (%d >= %d)\n", fileCount, x);
			x++;
			continue; 
		}
		
		memset(file, 0, sizeof(file));
		strcpy(file, dir->d_name);

		if (writen(s, (unsigned char *) file, sizeof(file)) == -1) { 
			printf("Failed to write file to stream\n"); 
			return; 
		}
		
		x++;
	}

	closedir(d);
}

void *client_handler(void *socket_desc) {
    int s = *(int *) socket_desc;

    send_hello(s);

    int running = 1;
    while(running == 1) {
        PACKET_TYPES reqType;
        ssize_t recvn = readn(s, (unsigned char *) &reqType, sizeof(PACKET_TYPES));

        if (recvn == -1 || recvn == 0) {
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

            default: break;
        }
    }

    printf("Thread %lu exiting\n", (unsigned long) pthread_self());

    shutdown(s, SHUT_RDWR);
    close(s);

    return 0;
}

void send_hello(int socket) {
    char szFormatString[] = "%s - hello SP (Callum Carmicheal, S1829709)";
	char szHelloString[256];
	char szEndpoint[47];

	server_endpoint(socket, szEndpoint, sizeof(szEndpoint));
	snprintf(szHelloString, sizeof(szHelloString), szFormatString, szEndpoint);
	
    size_t n = strlen(szHelloString) + 1;
    writen(socket, (unsigned char *) &n, sizeof(size_t));
    writen(socket, (unsigned char *) szHelloString, n);
}

void server_endpoint(int socket, char* buffer, size_t bufferSize) {
    struct NETWORK_INTERFACE* net = NULL;
    get_interface("wifi0", &net);
	
    if (net == NULL) {
        get_interface("eth0", &net);

        if (net != NULL)
            goto setInformation;

        printf("[WARNING] Failed to get network for eth0 or wifi0\n");
        snprintf(buffer, bufferSize, "%s", "(UNKNOWN-IP)");
        return;
    }

setInformation:
    snprintf(buffer, bufferSize, "%s", net->host);

    free(net->interface);
    free(net);
}

void get_interface(char* interface, struct NETWORK_INTERFACE** pInterface) {
    struct ifaddrs *plInterfaces, *pIf;
    int s;
    char host[NI_MAXHOST];

    if (getifaddrs(&plInterfaces) == -1) {
        perror("[WARNING] Unable to find any interfaces");
        return;
    }

    for (pIf = plInterfaces; pIf != NULL; pIf = pIf->ifa_next) {
        if (pIf->ifa_addr == NULL)
            continue;

        s = getnameinfo(pIf->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

        int isWantedInterface = ( strcmp(pIf->ifa_name, interface) == 0 );
        int isCorrectFamily   = ( pIf->ifa_addr->sa_family == AF_INET );

        if ( isWantedInterface && isCorrectFamily ) {
            if (s != 0) {
                fprintf(stderr, "[WARNING] getnameinfo(%s) failed: %s\n", interface, gai_strerror(s));
                return;
            }

            DBG_PRINT("\tInterface: %s\n", pIf->ifa_name);
            DBG_PRINT("\t  Address: %s\n", host);

            *pInterface = malloc(sizeof(struct NETWORK_INTERFACE));
            memset(*pInterface, 0, sizeof(struct NETWORK_INTERFACE));

            (*pInterface)->interface = malloc(sizeof(struct ifaddrs));
            memcpy((*pInterface)->interface, pIf, sizeof(struct ifaddrs));

            strcpy((*pInterface)->host, host);
        }
    }

    freeifaddrs(plInterfaces);
}
