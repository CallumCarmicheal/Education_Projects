// Cwk2: client.c - message length headers with variable sized payloads
//  also use of readn() and writen() implemented in separate code module

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <ncurses.h>
#include <fcntl.h>
#include <curses.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <limits.h>
#include <dirent.h>
#include "rdwrn.h"

/** ========== Packets ========== */

typedef enum _PACKET_TYPES {
	PT_ERROR_UNKNOWN = 0, // UNKNOWN

	PT_RANDOM,
	PT_UNAME,
	PT_UPLOAD,
	PT_DOWNLOAD,
	PT_BROWSER,

	PT_EXIT
} PACKET_TYPES;

/** ========== Macros ========== */

#define clrscr() \
    printf("\e[1;1H\e[2J")
#define MENU_TEXT_SIZE PATH_MAX

#define bool			_Bool
#define true			1
#define false			0
//#define MENU_DEBUG 	1

/** ========== Menu Definitions ========== */

struct Menu;

// Item and Menu Types
/**
 * Menu item type
 */
enum ItemType   { IT_TEXT, IT_BUTTON };

/**
 * The menu item
 */
struct MenuItem {
    enum    ItemType        Type;
            char            Text[MENU_TEXT_SIZE];
            int             Tag;
    struct  MenuItem*       pPrevious;
    struct  MenuItem*       pNext;
};

/**
 * Handles the menu items.
 */
struct MenuItems {
    struct  MenuItem*       pFirst; // First item in the list
    struct  MenuItem*       pLast;  // Last item in the list
            int             Count;  // The amount of items in the list (starts at 1)
};

/**
 * Callback for the menu rendering (prerender)
 */
typedef void (*CBMenuRender)(WINDOW*, struct Menu*);

/**
 * The Menu Settings
 */
struct MenuSettings {
            int             RenderSize;     // Amount of items to render.
            int             ViewStartIndex; // States the start of the rendering view.
            CBMenuRender    pRenderCb;      // Callback to prerender for each menu.
            void*           AttachedData;   // Data that have been attached to the menu for the prerender's usage.
};

/**
 * The menu structure
 */
struct Menu {
            char            Title[MENU_TEXT_SIZE];  // Menu Title
            int             HighlightedIndex;       // Currently Active Item
    struct  MenuItem*       pSelectedItem;          // Pointer to Selection
    struct  MenuItems       Items;                  // Items in the Menu
    struct  MenuSettings    Settings;               // Menu Settings
};

/**
 * The file structure used when returning infromation from the file selection
 */
struct file_s { 
    char Path[PATH_MAX];		// The full path of the file
};

// Environment functions
void environment_setup(void);
void environment_end(void);

// Menu functions
struct  Menu*       menu_create(char Title[MENU_TEXT_SIZE]);
        void        menu_free(struct Menu* pMenu, bool freeItems);
struct  MenuItem*   menu_item_add(struct Menu* pMenu, enum ItemType Type, char Text[MENU_TEXT_SIZE], int Tag);
        int         menu_item_find_index(struct Menu* pMenu, struct MenuItem* pWanted);
struct  MenuItem*   menu_item_find_tag(struct Menu *pMenu, int Tag);
        int         menu_item_find_next_type_index_up(struct Menu *pMenu, struct MenuItem *pItem, enum ItemType type, int currentIndex);
        int         menu_item_find_next_type_index_down(struct Menu *pMenu, struct MenuItem *pItem, enum ItemType type, int currentIndex);
        int         menu_item_find_nearest_type_index(struct Menu* pMenu, struct MenuItem* pItem, enum ItemType type, bool deletion);
        void        menu_item_free(struct Menu* pMenu, struct MenuItem* pItem);
        void        menu_item_free_all(struct Menu *pMenu);
		int			menu_prompt_file_select(WINDOW *pWin, struct file_s *pFile);
        int         menu_render(WINDOW *pWin, struct Menu *pMenu);

/**
 * Get the hello string and display it.
 *
 * @param socket  The socket fd
 */
void get_hello(int socket)
{
    char hello_string[256];
    size_t k;

    readn(socket, (unsigned char *) &k, sizeof(size_t));	
    readn(socket, (unsigned char *) hello_string, k);

    printf("Hello String: %s\n", hello_string);
    printf("Received: %zu bytes\n\n", k);
}

/**
 * Get the hello string and store it.
 *
 * @param socket  The socket fd
 * @param dest    The destination of the hello string
 */
void sget_hello(int socket, char* dest) {
    char hello_string[256];
    size_t k;

    readn(socket, (unsigned char *) &k, sizeof(size_t));
    readn(socket, (unsigned char *) hello_string, k);
    sprintf(dest, "%s", hello_string);
}

/**
 * Perform the user action: Random
 * Request an array of random numbers from the server.
 *
 * @param s The socket identifier.
 */
int action_random(int s) {
	// Clear the screen
	clear();

	printw("Sending packet request... ");

	// Send our request for the arrays
	PACKET_TYPES req = PT_RANDOM;

	// We failed to write to the server
	if (writen(s, (unsigned char *) &req, sizeof(PACKET_TYPES)) == -1)
		return 1;

	printw("DONE.\nReading random numbers...\n\n");

	// Store our random numbers
	int random[5];
	memset(random, 0, sizeof(random));

	// Read our array
	if (readn(s, &random, sizeof(random)) == -1)
		// We failed to communicate with the server
		return 1;

	// Loop our array and print the elements
	for(int x = 0; x < 5; x++)
		printw("Random [%d] = %d\n", x, random[x]);

	printw("\nPress any key to continue: \n");
	getch();

	return 0;
}

/**
 * Perform the user action: Uname
 * Request the uname information from the server.
 *
 * @param s The socket identifier.
 */
int action_uname(int s) {
	// Clear the screen
	clear();

	printw("Sending packet request... ");

	// Send our request for the arrays
	PACKET_TYPES req = PT_UNAME;

	// We failed to write to the server
	if (writen(s, (unsigned char *) &req, sizeof(PACKET_TYPES)) == -1)
		return 1;

	printw("DONE.\nReading uname from server... ");

	// Store our random numbers
	int status = -1;
	struct utsname uname;

	// Read our status
	if (readn(s, &status, sizeof(int)) == -1)
		// We failed to communicate with the server
		return 1;

	// If the status does not eq 0 then there was an error
	if (status != 0) {
		printw("ERROR.\n\nFailed to receive uname from server, Error no = %d\n\n", status);
		goto wait;
	}

	// Get the uname struct
	if (readn(s, &uname, sizeof(uname)) == -1)
		return 1; // We failed to recv from socket, DISCONNECT.

	printw("DONE.\n\n");
	
	// Print our uname information
	printw("system name = %s\n", uname.sysname);
	printw("node name   = %s\n", uname.nodename);
	printw("release     = %s\n", uname.release);
	printw("version     = %s\n", uname.version);
	printw("machine     = %s\n\n", uname.machine);

wait:
	printw("\nPress any key to continue: \n");
	getch();

	return 0;
}

/**
 * Perform the user action: Upload
 * Upload files from the current computer to the 
 *   upload directory on the server.
 *
 * @param s The socket identifier.
 */
int action_upload(int s) {
	struct file_s file;
	ssize_t r;
	bool wait = true;

	// Clear our garbage
	memset (&file, 0, sizeof(file));
	
	// 2 = File selection	
	if (menu_prompt_file_select(stdscr, &file) == 2) {
	    clear();
		printw("Selected file: %s\n", file.Path);
		printw("READING FILE...                      ");

		// Read the file		
		FILE* f =fopen(file.Path,"rb");

		fseek(f, 0, SEEK_END); 	 // seek to end of file
		long size = ftell(f) ;   // get current file size
		fseek(f, 0, SEEK_SET); 	 // seek back to beginning of file

		// Read the file	
		unsigned char* buffer = (unsigned char*) malloc(size * sizeof(unsigned char));
		memset(buffer, 0, size * sizeof(unsigned char));
		size_t result = fread (buffer,1,size,f);

		// Error check: if we have not read the correct size
		if (result != size) {
			printw("[ NOT OK ]\n");
			printw("  There was an error while reading the file.\n");

			free(buffer);
			fclose(f);
			goto wait;
		}
		
		printw("[ OK ]\n");
		printw("SENDING FILE TO SERVER...            ");

		// Send the file to the server
		// send fmt:{header[PACKET_TYPES]}
		// 			{fnsize[int]}
		// 			{fname[fnsize]}
		// 			{fsize[long]}
		// 			{buffer[unsigned char:fsize]}
		// resp fmt:{status[int]}	
		
		// Send packet header
		PACKET_TYPES req = PT_UPLOAD;
		if (writen(s, (unsigned char *) &req, sizeof(PACKET_TYPES)) == -1)
			return 1; // failed to write to the server

		char*  fname = basename(file.Path);
		size_t nameSize = strlen(fname);

        // String size
        if (writen(s, (unsigned char *) &nameSize, sizeof(size_t)) == -1)
            return 1; // failed to write to the server.

        // File name
        if (writen(s, (unsigned char *) fname, nameSize * sizeof(char)) == -1)
            return 1; // failed to write to the server.

        // File size
		if (writen(s, (unsigned char *) &size, sizeof(long)) == -1)
			return 1; // failed to write to the server.

		// File buffer
		if ((r=writen(s, (unsigned char *) buffer, size * sizeof(unsigned char))) == -1)
			return 1; // failed to write to the server.

		printw("[ OK ]\n");

		// Debug print information
		//printw("  fname = {%s}, nameSize = {%zd}\n", fname, nameSize);
		//printw("  size = %d\n", size);	
			
		// Close the file
		fclose(f);

		// Free up the buffers memory
		free(buffer);

        // Now check what the server status is.
		printw("WAITING FOR SERVER TO RESPOND...     ");
		
		//FILE_STATUS, -2 WRITE_ERROR, -1 ILLEGAL_PATH, 0 UNKNOWN_ERROR, 1 SUCCESS
		int server_status = 0; 
		if ((readn(s, (unsigned char*) &server_status, sizeof(int))) == -1) 
			return 1; // failed to read from server

		// Print status
		printw("%s", server_status == 1 ? "[ OK ]\n" : "[ NOT OK ]\n");

		switch (server_status) {
		case -2: 
			printw("  The server failed to write the file, please try again later.");
			break;
		case -1:
			printw("  The path send to the server was flagged to be invalid, please try again later.");
			break;
		case 0:
		default:
			printw("  There was an unknown error / exception thrown on the server, please try again later.");
			break;
		case 1:break;
		}
	} else {
		// We did not select a file so assume the user pressed Q or ESCAPE.
		wait = false;
	}

wait: // Return no error and if we have to wait for the user 
	  // wait for user input.
	if (wait) { 
		printw("\nPRESS ANY KEY TO CONTINUE\n");
		getch(); 
	}
	
	return 0;
}


void action_download_prerender(WINDOW* pWin, struct Menu* pMenu) {
	wprintw(pWin, "Press ESC or Q to Cancel file selection!\n\n");
	wprintw(pWin, "Select a file to download:\n=====================\n  (%d/%d)\n", 
				   pMenu->HighlightedIndex+1, pMenu->Items.Count);
}

/**
 * Perform the user action: Download
 * Download files from the upload directory on the server
 *
 * @param s The socket identifier.
 */
int action_download(int s) {
	clear(); // Clear the screen

	// Variables
	unsigned char *buffer = NULL;
	FILE* write_ptr = NULL;
	int fileCount = 0;

	// Initialize the request
	PACKET_TYPES request = PT_BROWSER;
	if ( writen(s, (unsigned char *) &request, sizeof(int)) == -1 ) 
		return 1;

	// Receive the file count
	printw("RECEIVING FILE COUNT...     ");
	if ( readn(s, (unsigned char *) &fileCount, sizeof(int)) == -1 ) 
		return 1; // failed to receive
	
	printw("[ OK ]   %d files found.\n", fileCount);
	
	// If we don't have any files
	if (fileCount == 0) {
		printw("  There are 0 files in the remote folder.");
		printw("\n\nPress Any Key to Continue.\n");
		getch();
		refresh();
		
		return 0;
	}
	
	// Create the menu to display the files
	struct Menu* menu = menu_create("");
	menu->Settings.pRenderCb = action_download_prerender;

	// Tag number	
	int n = 0;
	char files[fileCount][PATH_MAX]; // Files 

	// Read in the files
	for(int x = 0; x < fileCount; x++) {
		// Read the file from the stream
		if (readn(s, (unsigned char *) files[n], PATH_MAX) == -1)
			goto socketFailure;
		
		// Add the file to the menu
		menu_item_add(menu, IT_BUTTON, files[n], n);
		n++; // Increment n.
	}

	// Render the menu and then dispose the menu
	if (menu_render(stdscr, menu) == 1) {
		wclear(stdscr);
		printw("SELECTED FILE: %s\n", files[menu->pSelectedItem->Tag]);
		printw("SENDING SERVER REQUEST...            ");
		
		// Tell the server we are uploading a file
		request = PT_DOWNLOAD;
		if ( writen(s, (unsigned char *) &request, sizeof(int)) == -1 )
			goto socketFailure;
	
		// Tell the server what file we want
		if ( writen(s, (unsigned char *) &files[menu->pSelectedItem->Tag], PATH_MAX) == -1 )
			goto socketFailure;

		printw("[ OK ]\n");
		printw("CHECKING FILE AVAILABILITY...        ");

		// Check if the file is available for download...	
		int status = 0;
		if ( readn(s, (unsigned char *) &status, sizeof(int)) == -1 )
			goto socketFailure;
			
		if (status != 1) {
			printw("[ NOT OK ]\n");
			printw("  Server failed to read the file. (status = %d)\n", status);
			goto finish;
		}

		printw("[ OK ]\n");
		
		printw("READING IN FILE SIZE...              ");

		// We now need to get the file size
		size_t fileSize;
		if ( readn(s, (unsigned char *) &fileSize, sizeof(size_t)) == -1 )
			goto socketFailure;
		
		printw("[ OK ]\n");
		printw("READING IN FILE BUFFER...            ");

		// We need to now create the buffer for the file and read in the data
		buffer = (unsigned char*) malloc(fileSize);
		if ( readn(s, buffer, fileSize) == -1 )
			goto socketFailure;
		
		printw("[ OK ]\n");
		
		printw("WRITING FILE DATA...                 ");
		
		// Setup file path data, realpath needs a buffer for files.
		char tmp__filePath[PATH_MAX];
		char filePath[PATH_MAX];
		memset(tmp__filePath, 0, sizeof(tmp__filePath)); // clear out garbage data

		// Get the filePath ( "./" + files[...] + \0) -> /home/.../dir
		strcat   ( tmp__filePath, "./" );
		strcat   ( tmp__filePath, files[menu->pSelectedItem->Tag] );
		tmp__filePath[PATH_MAX-1] = 0;
		realpath ( tmp__filePath, filePath );

		// Attempt to open the file
    	write_ptr = fopen(filePath,"wb");
		
		if (write_ptr == NULL) {
			printw("[ NOT OK ]\n");
			printw("  Failed to write because file pointer is null.");
			goto finish;
		}	

		// Write the file contents
		if (fwrite(buffer, fileSize, 1, write_ptr) != 1) {
			// Write the file
			printw("[ NOT OK ]\n");
			printw("  Failed to write file. (fwrite != 1)");
			goto finish;
		}

		printw("[ OK ]");

		goto finish;

socketFailure: // socket failed so we need to clean all buffers
		if (menu != NULL)
			menu_free(menu, true);
		
		// If our buffer was set, free it
		if (buffer != NULL)		free(buffer);
	
		// If our write pointer was set.
		if (write_ptr != NULL)	fclose(write_ptr);

		return 1;
	}
	
finish: // Finished actions 

	// Free the menu
	menu_free(menu,true);
		
	// If our buffer was set, free it
	if (buffer != NULL)		free(buffer);
	
	// If our write pointer was set.
	if (write_ptr != NULL)	fclose(write_ptr);

	printw("\n\nPress Any Key to Continue.\n");
	refresh();
	getch();
	return 0;
}

void action_browse_prerender(WINDOW* pWin, struct Menu* pMenu) {
	wprintw(pWin, "Files on server:\n=====================\n  (%d/%d)\n", 
				   pMenu->HighlightedIndex+1, pMenu->Items.Count);
}

/**
 * Perform the user action: Browse
 * Browse files in the upload directory on the server
 *
 * @param s The socket identifier.
 */
int action_browse(int s) {
	clear();

	// Initialize the request
	PACKET_TYPES request = PT_BROWSER;
	if ( writen(s, (unsigned char *) &request, sizeof(int)) == -1 ) 
		return 1;
	
	// Variables
	int fileCount = 0;
	char file[PATH_MAX];
	
	printw("RECEIVING FILE COUNT...     ");
	if ( readn(s, (unsigned char *) &fileCount, sizeof(int)) == -1 ) 
		return 1; // failed to receive
	
	printw("[ OK ]   %d files found.\n", fileCount);
	
	// If we dont have any files
	if (fileCount == 0) {
		printw("  There are 0 files in the remote folder.");
		printw("\n\nPress Any Key to Continue.\n");
		getch();
		refresh();
		
		return 0;
	}
	
	// Create the menu to display the files
	struct Menu* menu = menu_create("");
	menu->Settings.pRenderCb = action_browse_prerender;

	// Read in the files
	for(int x = 0; x < fileCount; x++) {
		// Read the file from the stream
		if (readn(s, (unsigned char *) file, PATH_MAX) == -1)
			return 1;
		
		// Add the file to the menu
		menu_item_add(menu, IT_BUTTON, file, -1);
		
		// Print the file (in case of many files being sent)
		printw("file: %s\n", file);
		refresh();
	}

	// Render the menu and then dispose the menu
	menu_render(stdscr, menu);
	menu_free(menu,true);
		
	printw("\n\nPress Any Key to Continue.\n");
	refresh();
	getch();
	return 0;
}

struct main_menu_bag { char hello[256]; };
void main_menu_prerender(WINDOW* win, struct Menu* menu) {
	struct main_menu_bag *bag = menu->Settings.AttachedData;
	wprintw(win, "Server: %s\n" 
				 "\nPlease select a option below\n" 
				 "=========================\n", bag->hello);
}

/**
 * This is the user interaction menu handler, this displays and executs all actions
 * that the user can perform
 *
 * @param sockfd The socket
 * @return 0 = SEND_GOODBYE, 1 = SOCKET_DISCONNECTED
 */
int application(int sockfd) {
	int server_state = 0;

    // Setup our render bag and get the hello message
    struct main_menu_bag bag;
    sget_hello(sockfd, bag.hello);

    // Setup the environment
    environment_setup();

#if MENU_DEBUG
    printw("Attach a debugger now!");
    refresh();
    getch();
#endif

    // Menu setup and settings
    struct Menu* menu = menu_create("Main Menu");
    menu->Settings.AttachedData = &bag;
    menu->Settings.pRenderCb = &main_menu_prerender;

    // Setup the main menu environment
	menu_item_add(menu, IT_TEXT, "Server:", -1);
	struct MenuItem* pbRand  = menu_item_add(menu, IT_BUTTON, "Random Numbers", -1);
	struct MenuItem* pbUname = menu_item_add(menu, IT_BUTTON, "Uname Information", -1);

	menu_item_add(menu, IT_TEXT, "Remote Files:", -1);
	struct MenuItem* pbUpload   = menu_item_add(menu, IT_BUTTON, "Upload", -1);
	struct MenuItem* pbDownload = menu_item_add(menu, IT_BUTTON, "Download", -1);
	struct MenuItem* pbBrowse   = menu_item_add(menu, IT_BUTTON, "Browse Directory", -1);

	menu_item_add(menu, IT_TEXT, "Application:", -1);
	struct MenuItem* pbExit = menu_item_add(menu, IT_BUTTON, "Exit", -1);

	// Render the menu
	bool exiting = false;
	
	while (!exiting) {
        // Render the menu
        int state = menu_render(stdscr, menu);
        if (state != 1) {
            // We are exiting
            exiting = true;
            break; // Q or ESCAPE (quit code)
        }

        // Selected Item
        struct MenuItem* i = menu->pSelectedItem;

        // If we have a null selection (for some reason)
        // we will just loop
        if (i == NULL)
            continue;

		// Exit
		if (i == pbExit) {
			exiting = true;
			break;
		}

		// Random
		else if (i == pbRand) {
			if (action_random(sockfd)) {
				server_state = 1;
				goto cleanup;
			};
		}

		// Uname
		else if (i == pbUname) {
			if (action_uname(sockfd)) {
				server_state = 1;
				goto cleanup;
			};
		}
		
		// File Upload
		else if (i == pbUpload) {
			if (action_upload(sockfd)) {
				server_state = 1;
				goto cleanup;
			};
		}

		// File Download
		else if (i == pbDownload) {
			if (action_download(sockfd)) {
				server_state = 1;
				goto cleanup; 
			};
		}

		// File browser
		else if (i == pbBrowse) {
			if (action_browse(sockfd)) {
				server_state = 1;
				goto cleanup;
			}
		}
	}

cleanup:
    // Cleanup the main menu environment
	menu_free(menu, true);

    // Clear the environment
    environment_end();

    return server_state;
}

/**
 * Application entry point
 */
int main(void)
{
    // *** this code down to the next "// ***" does not need to be changed except the port number
    int sockfd = 0;
    struct sockaddr_in serv_addr;

	// Check if we failed to get the socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Error - could not create socket");
		exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;

    // IP address and port of server we want to connect to
    serv_addr.sin_port = htons(50031);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // try to connect...
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)  {
	    perror("Error - connect failed");
	    exit(1);
    } else
       printf("Connected to server...\n");

	// Run our application menu
	// If application == 1 then we have disconnected (ungracefully)
	if (application(sockfd)) {
		printf("Disconnected from server! (CONNECTION DROPPED)\n");
		goto cleanup;
	}

	// We are exiting so try to gracefully tell the server that we are closing.
	PACKET_TYPES req = PT_EXIT;
	writen(req, (unsigned char *) &req, sizeof(PACKET_TYPES));

cleanup:
    // *** make sure sockets are cleaned up
    close(sockfd);
    exit(EXIT_SUCCESS);
} // end main()



// **************************** //
//     Below is all code        //
//     Related to rendering     //
//     The menu environment     //
// **************************** //

/**
 * Setup the environment
 */
void environment_setup(void) {
    // Init curses
    initscr();

    // Settings
    start_color();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
}

/**
 * End the environment
 */
void environment_end(void) {
    endwin();
    setvbuf(stdout, NULL, _IOLBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
}

/**
 * Create a menu
 *
 * @param Title
 * @return
 */
struct Menu* menu_create(char Title[MENU_TEXT_SIZE]) {
    // Create menu
    struct Menu* menu = malloc(sizeof(struct Menu));
    memset(menu, 0, sizeof(struct Menu));

	// Copy the title
    strncpy(menu->Title, Title, sizeof(menu->Title));
    menu->HighlightedIndex = -1; // The default highlighted action
    menu->pSelectedItem = NULL;  // The selected item

    // Setup default settings values
    menu->Settings.RenderSize = 10; // Display up to 20 items by default
    menu->Settings.ViewStartIndex = 0; // Start at 0 always.
    menu->Settings.pRenderCb = NULL;
    menu->Settings.AttachedData = NULL;

    return menu;
}

/**
 * Free a menu
 *
 * @param pMenu
 * @param freeItems
 */
void menu_free(struct Menu* pMenu, bool freeItems) {
	// If we are going to free the menu items
    if (freeItems)
        // Free the menu items
        menu_item_free_all(pMenu);

    // Free the menu
    free(pMenu);
}

/**
 * Add an item to the menu
 *
 * @param pMenu The menu item.
 * @param Type The item type.
 * @param Text The text of the item.
 * @param Tag A tag to identify the item.
 * @return
 */
struct MenuItem* menu_item_add(struct Menu* pMenu, enum ItemType Type, char Text[MENU_TEXT_SIZE], int Tag) {
    // Create item, Set the Type, Copy the Text and Set the Tag.
    struct MenuItem* item = malloc(sizeof(struct MenuItem));
    memset(item, 0, sizeof(struct MenuItem));

	// Set the next and previous items to null
    item->pNext = NULL;
    item->pPrevious = NULL;

	// Set our type and tag
    item->Type = Type;
    strncpy(item->Text, Text, sizeof(item->Text)); // Copy our display text
    item->Tag  = Tag;

    // Set the first item
    if (pMenu->Items.Count == 0) {
        pMenu->Items.pFirst = item;

        // Highlight index to 0 if the type is a button
        if (item->Type == IT_BUTTON)
            pMenu->HighlightedIndex = 0;
    }

    // We have items
    else {
        // Add to the end of the list
        pMenu->Items.pLast->pNext = item;

        // Set the previous item
        item->pPrevious = pMenu->Items.pLast;
    }

    // Increment our count and set the last item
    pMenu->Items.Count++;
    pMenu->Items.pLast = item;
    return item;
}

/**
 * Find index for menu item
 *
 * @param pMenu
 * @param pWanted
 * @return Index number
 */
int menu_item_find_index(struct Menu* pMenu, struct MenuItem* pWanted) {
    // Get the first item
    struct MenuItem* pItem = pMenu->Items.pFirst;

    // Go through the linked list
    int x = 0;
    while (pItem != NULL) {
        // If we are the wanted item return the index
        if (pItem == pWanted)
            return x;

        // Loop the next item
        pItem = pItem->pNext;
        x++;
    }

    // We could not find it return -1
    return -1;
}


/**
 * Find the next item by a item type in the downwards direction
 *
 * @return NULL if not found.
 */
struct MenuItem* menu_item_find_tag(struct Menu *pMenu, int Tag) {
    // If we dont have any items return null
    if (pMenu->Items.Count == 0)
        return NULL;

    // Loop the items
    struct MenuItem* pCurrent = pMenu->Items.pFirst;
    while (pCurrent != NULL) {
        // If we have found the tag return the item
        if (pCurrent->Tag == Tag)
            return pCurrent;

        // Loop to the next item
        pCurrent = pCurrent->pNext;
    }

    // We did not find it return NULL
    return NULL;
}


/**
 * Find the next item by a item type in the upwards direction
 *  
 * @return [-1 For not found, >0 When found]
 */
int menu_item_find_next_type_index_up(struct Menu *pMenu, struct MenuItem *pItem, enum ItemType type,
                                      int currentIndex) {
    // Set the current index to +1 because we are getting the next item
	int index = currentIndex + 1;

	// Loop the items
	struct MenuItem* pCurrent = pItem->pNext;
	while (pCurrent != NULL) {
		// If we found it return the index
	    if (pCurrent->Type == type)
			return index;

		// Next item
		pCurrent = pCurrent->pNext;
		index++;
	}	

	// Return -1, we could not find it.
	return -1;
}

/**
 * Find the next item by a item type in the downwards direction
 *
 * @return [-1 For not found, >0 When found]
 */
int menu_item_find_next_type_index_down(struct Menu *pMenu, struct MenuItem *pItem, enum ItemType type,
                                        int currentIndex) {
    int index = currentIndex - 1;
    struct MenuItem* pCurrent = pItem->pPrevious;

    while (pCurrent != NULL) {
        if (pCurrent->Type == type)
            return index;

        pCurrent = pCurrent->pPrevious;
        index--;
    }

    return -1;
}

/**
 * Search for the nearest index of
 *
 * @param pMenu The menu that contains the items
 * @param pItem The item we start the search froom
 * @param type The menu type we are looking for
 * @param deletion States if we are about to pop the current item of the stack
 * @return
 */
int menu_item_find_nearest_type_index(struct Menu* pMenu, struct MenuItem* pItem, enum ItemType type,
                                      bool deletion) {
	// Check if the list is empty then just return -1
	if (pMenu->Items.Count == 0
        // This just states if we are popping a item off and still return -1
		|| (deletion && pMenu->Items.Count == 1))
		return -1;

	// Check if we are the only item in the list
    if (pMenu->Items.Count == 1) return -1;
	

    // Setup the indexes.
	int currentIndex = menu_item_find_index(pMenu, pItem);
	int newIndex = -1;
	
	// If we are the first item then we must go up
	if (currentIndex == 0) {
	    newIndex = menu_item_find_next_type_index_up(pMenu, pItem, type, currentIndex);
	}

	// If we are the last item then we must go down
	else if (currentIndex + 1 == pMenu->Items.Count) {
		newIndex = menu_item_find_next_type_index_down(pMenu, pItem, type, currentIndex);
	}

	// We are in the center of the list, we gotta check both directions.
	// for this we will first go down the list and then up.
	else {
		// If we failed to find a item going down then lets search up.
        newIndex = menu_item_find_next_type_index_down(pMenu, pItem, type, currentIndex);

        if (newIndex == -1) {
            // We failed to find a type going up, so we now want to search down
            newIndex = menu_item_find_next_type_index_up(pMenu, pItem, type, currentIndex);

            // If we are deleting the item then we need to account for it shifting the menu.
            if (newIndex > -1 && deletion)
                newIndex--;
		}
    }
	
	// Return our result
    return newIndex;
}	

/**
 * Free a specific menu item.
 *
 * @param pMenu
 * @param pItem
 */
void menu_item_free(struct Menu* pMenu, struct MenuItem* pItem) {
	// Get the next and previous items
    struct MenuItem *Previous = pItem->pPrevious;
    struct MenuItem *Next = pItem->pNext;

    // Now we update the highlight index
    if (pMenu->Items.Count == 1)
        // if its 1, then the item we are removing is the item 1
        pMenu->HighlightedIndex = -1;
	else
        // if we are not one, that means we have more then 1 item, so we want to attempt
        // to find the nearest available item of the BUTTON type.
        pMenu->HighlightedIndex = menu_item_find_nearest_type_index(pMenu, pItem, IT_BUTTON, true);

    // Check if Previous is null (then we are the first element)
    //      and Next is set.
    //      - We are removing the first element
    if (Previous == NULL && Next != NULL) {
        // We now want to set Next to the first item in the array
        pMenu->Items.pFirst = Next;
        pMenu->Items.Count--;

        // Remove the pointer from Next->Previous
        Next->pPrevious = NULL;
    }

    // Check if Previous is not NULL but Next is
    //      - We are removing the last element
    else if (Previous != NULL && Next == NULL) {
        // Now we want to set the last element to Previous
        pMenu->Items.pLast = Previous;
        pMenu->Items.Count--;

        // Remove the pointer from Previous->Next
        Previous->pNext = NULL;
    }

    // If Previous and Next are NULL then we are removing
    //      the last item from the list
    //      - We are resetting the list and removing all elements
    else if (Previous == NULL && Next == NULL) {
        pMenu->Items.Count = 0;
        pMenu->Items.pLast = NULL;
        pMenu->Items.pFirst = NULL;
        pMenu->pSelectedItem = NULL;
    }

    // We are removing an item in the middle of the array
    //      - Updating the Next and Previous items.
    //
    //  This is basically the else condition but its written like this
    //  even though it is always TRUE when reached to make the code more
    //  readable.
    else if (Previous != NULL && Next != NULL) {
        // Update the Next and Previous pointers
        Previous->pNext = Next;
        Next->pPrevious = Previous;
        pMenu->Items.Count--;
    }

    // Remove the selected item if its pItem
    if (pMenu->pSelectedItem == pItem)
        pMenu->pSelectedItem = NULL;

    // AND FINALLY we free up the item
    free(pItem);
}

/**
 * Free the menu and its items
 *
 * @param pMenu
 */
void menu_item_free_all(struct Menu *pMenu) {
    // Get the first item
    struct MenuItem* pItem = pMenu->Items.pFirst;

    // Go through the linked list
    while (pItem != NULL) {
	    struct MenuItem* currentItem = pItem;
	    pItem = pItem->pNext;

        // Free our menu item
        free(currentItem);
    }

    pMenu->pSelectedItem = NULL;
    pMenu->Items.Count = 0;
}

/**
 * Render the menu item
 *
 * @param pMenu
 */
void menu_render_item(WINDOW *pWin, int index, struct Menu *this, struct MenuItem *item) {
#if DEBUG_MENU
	// Render menu items and their pointers for memory mapping.

    // Text display
    if (item->Type == IT_TEXT) {
        wprintw(pWin, "  %s\t[%d](%p)\n", item->Text, index, item);
	}
    // Buttons
    else if (item->Type == IT_BUTTON) {
        // If we are the selection
        if (index == this->HighlightedIndex) {
            wprintw(pWin, " ");
            attron(A_REVERSE);
            wprintw(pWin, " > %s < ", item->Text);
            attroff(A_REVERSE);
       		wprintw(pWin, "\t[%d](%p)\n", index, item);
		}

        // If we are not then just render normally.
        else {
            wprintw(pWin, "    %s   \t[%d](%p)\n", item->Text, index, item);
		}
    }
#else
	// Text display
	if (item->Type == IT_TEXT) {
		wprintw(pWin, " %s\n", item->Text);
	}
		// Buttons
	else if (item->Type == IT_BUTTON) {
		// If we are the selection
		if (index == this->HighlightedIndex) {
			wprintw(pWin, " ");
			attron(A_REVERSE);
			wprintw(pWin, " > %s < ", item->Text);
			attroff(A_REVERSE);
			wprintw(pWin, "\n");
		}

		// If we are not then just render normally.
		else {
			wprintw(pWin, "    %s\n", item->Text);
		}
	}
#endif
}

/**
 * Move the selection down (and the viewport) in the menu
 *
 * @param pMenu
 */
void menu_render_move_down(struct Menu *pMenu) {
    // Loop until we have our item
    struct MenuItem *pItem = pMenu->Items.pFirst;
    bool foundButton = false;

	// Loop the items
    int i = 0;
    while (pItem != NULL) {
        // If we are the highlighted index
        if (i == pMenu->HighlightedIndex) {
            // Check if we have a next item increment our index
            if (pItem->pNext != NULL) {
                // We now need to loop to find the next button
                struct MenuItem *pButton = pItem->pNext;
                i++;

                while(pButton != NULL) {
                    // If we are a button then we are updating the highlight index
                    if (pButton->Type == IT_BUTTON) {
                        pMenu->HighlightedIndex = i;
                        foundButton = true;
                        goto viewport; // Now handle the viewport
                    }
					
					// Loop to next item
                    pButton = pButton->pNext;
                    i++;
                }
            }

            goto viewport; // Now handle the viewport
        }

		// Loop to next item
        pItem = pItem->pNext;
        i++;
    }

viewport: 
	// If the button is not rendered, move into view
	if (foundButton) {
		int hi = pMenu->HighlightedIndex;
		int vsi = pMenu->Settings.ViewStartIndex;
		int rs = pMenu->Settings.RenderSize;
		
		// If we are not in the view 
		if (hi > (vsi + rs - 1)) {
			pMenu->Settings.ViewStartIndex++;

			// If we still need to scroll then set Highlight index to the bottom of the view.
			if (hi > (vsi + rs)) {
				int newvsi = hi - vsi - 1;
				if (newvsi < 0) newvsi = 0;

				pMenu->Settings.ViewStartIndex = newvsi;	
			}
		}
	} else {
		// Just increment the view start index if we can
		if (pMenu->Settings.ViewStartIndex < (pMenu->Items.Count - pMenu->Settings.RenderSize)) {
			pMenu->Settings.ViewStartIndex++;
		}
	}
}

/**
 * Move the selection up (and the viewport) in the menu
 * @param pMenu
 */
void menu_render_move_up(struct Menu *pMenu) {
    // Loop until we have our item
    bool foundButton = false;

    // We cant go any higher then 0
    if (pMenu->HighlightedIndex == 0)
        goto viewport;

    struct MenuItem *pItem = pMenu->Items.pFirst;
    int i = 0;
    while (pItem != NULL) {
        // We found the current item
        if (i == pMenu->HighlightedIndex) {
            // Check if we have a next item increment our index
            if (pItem->pPrevious != NULL) {

                // We now need to loop to find the next button
                struct MenuItem *pButton = pItem->pPrevious; i--;
                while (pButton != NULL) {
                    // If we are a button then set the highlighted index
                    if (pButton->Type == IT_BUTTON) {
                        pMenu->HighlightedIndex = i;

                        // Set found button to true and update the viewport.
                        foundButton = true;
                        goto viewport;
                    }

                    // Loop next item
                    pButton = pButton->pPrevious;
                    i--;
                }
            }

            // If we could not find anything update the viewport.
            goto viewport;
        }

        // Loop next item.
        pItem = pItem->pNext;
        i++;
    }

viewport:
    if (foundButton) {
        // Move the button into view if its not current visible
        if (pMenu->HighlightedIndex < pMenu->Settings.ViewStartIndex)
            pMenu->Settings.ViewStartIndex = pMenu->HighlightedIndex;
    } else {
        // If we can still scroll
        if (pMenu->Settings.ViewStartIndex > 0)
            // Scroll up
            pMenu->Settings.ViewStartIndex--;
    }
}

/**
 * Finds and sets the current active Item
 *
 * @param pMenu
 * @return
 */
int menu_render_select_highlighted(struct Menu *pMenu) {
    // Loop until we have our item
    struct MenuItem* pItem = pMenu->Items.pFirst;
    int i = 0;
    while (pItem != NULL) {
        // We found our item
        if (i == pMenu->HighlightedIndex) {
            // Check if index is not a button
            if (pItem->Type != IT_BUTTON)
                return 1; // We found it

            // Set the selected item
            pMenu->pSelectedItem = pItem;
            return 0; // Failed to find it
        }

        // Get our next item
        pItem = pItem->pNext;
        i++;
    }

    // Keep rendering
    return 1;
}

#ifdef DEBUG_MENU

/**
 * DEBUG: Add a item to the end of the list.
 *
 * @param pWin
 * @param pMenu
 * @param type
 */
void menu_dbg_add_item(WINDOW* pWin, struct Menu *pMenu, enum ItemType type) {
    wclear(pWin);
    wprintw(pWin, "Enter a button name: ");
	
    echo();
    char input[MENU_TEXT_SIZE];
    wgetstr(pWin, input);
    noecho();
	
    menu_item_add(pMenu, type, input, -1);
    wclear(pWin);
}

/**
 * DEBUG: Delete the highlighted item
 *
 * @param pMenu
 */
void menu_dbg_delete_highlighted(struct Menu *pMenu) {
    struct MenuItem *pItem = pMenu->Items.pFirst;
    int i = 0;
    while (pItem  != NULL) {
        if (i == pMenu->HighlightedIndex) {
            menu_item_free(pMenu, pItem); // This should be freed
            return;
        }

        pItem = pItem->pNext;
        i++;
    }
}

#endif

/**
 * Render the menu
 * @param pWin The window to render the menu onto
 * @param pMenu The menu to render
 * @return The menu exit state, 0=No selection, 1=Selection.
 */
int menu_render(WINDOW *pWin, struct Menu *pMenu) {
    // We are rendering
    int rendering = 1,
        result = 0; // Our return result
	
    // Reset our selection
    pMenu->pSelectedItem = NULL;

    // If we have items then we want to set the selection index
    if (pMenu->Items.Count > 0) {
        // If the first item is a button
        if (pMenu->Items.pFirst->Type == IT_BUTTON)
            pMenu->HighlightedIndex = 0; // Set the highlight index to the first item
        else // If the first item is not a button then we want to find the nearest button (going up)
            pMenu->HighlightedIndex = menu_item_find_next_type_index_up(pMenu, pMenu->Items.pFirst, IT_BUTTON, 0);
    }

    // Render our menu
    while(rendering == 1) {
        clear();

#if DEBUG_MENU
        // Print our header
        wprintw(pWin, "HI(%d) VSI(%d)\n", pMenu->HighlightedIndex, pMenu->Settings.ViewStartIndex);
		wprintw(pWin, "Item count: %d\n\n", pMenu->Items.Count);
        wprintw(pWin, "%s\n======-======-======-======\n", pMenu->Title);
#endif
        if (pMenu->Settings.pRenderCb != NULL)
            pMenu->Settings.pRenderCb(pWin, pMenu);

        // Just loop the list for now and render it
        struct MenuItem* pItem = pMenu->Items.pFirst;
        int i = 0, v = 1;// i = ItemIndex, v = Item rendering index (ViewIndex)
        while (pItem != NULL) {
			// Make sure are in range and v is not past the max render size.
            if (i >= pMenu->Settings.ViewStartIndex && v <= pMenu->Settings.RenderSize) {
                // Render the item
                menu_render_item(pWin, i, pMenu, pItem);
                v++;
            }

            // Next item
            pItem = pItem->pNext;
            i++;
        }

        // Get our key press
        switch(wgetch(pWin)) {
            case 10:                    // ENTER
                rendering = menu_render_select_highlighted(pMenu);
                result = 1;
                break;
            case 113:                   // Q
            case 27:                    // ESCAPE
                rendering = 0;
                result = 0;
                break;
            case 258:                   // DOWN_ARROW
                menu_render_move_down(pMenu);
				break;
            case 259:                   // UP_ARROW
                menu_render_move_up(pMenu);
				break;
#ifdef DEBUG_MENU
	        case 100:                   // D
                menu_dbg_delete_highlighted(pMenu);
                break;
            case 260:                   // LEFT_ARROW
                break;
            case 261:                   // RIGHT_ARROW
                break;
            case 98:                    // Debug - B (add text)
                menu_dbg_add_item(pWin, pMenu, IT_TEXT);
                break;
            case 97:                    // Debug - A (add button)
                menu_dbg_add_item(pWin, pMenu, IT_BUTTON);
                break;
#endif
            default: break;
        }

        // Refresh the screen
        wrefresh(pWin);
    }

    return result;
}

struct menu_prompt_file_render_menu_bag {
	char Path[PATH_MAX];
};

void menu_prompt_file_render_menu_prerender(WINDOW* pWin, struct Menu* pMenu) {
	struct menu_prompt_file_render_menu_bag* bag = pMenu->Settings.AttachedData;
	wprintw(pWin, "Press ESC or Q to Cancel file selection!\n\n");
	wprintw(pWin, "Please select a file:\n[%s]:\n=====================\n  (%d/%d)\n", 
				   bag->Path, pMenu->HighlightedIndex+1, pMenu->Items.Count);
}

/**
 * Render the select file menu onto pWin
 *
 * @param pWin             This is the window the menu will be rendered onto
 * @param path   MODIFIED  This is the beginning path and the file/new dir output.
 *
 * @return 0 = DIR_ERROR, -1 = NO_FILES, -2 = NO_SELECTION, 1 = DIR, 2 = FILE
 *
 * If returned 1, you are expected to interperate the result as either 
 * i ) A directory selection
 * ii) A sub directory go into selection and then loop while 1 is returned.
 */
int menu_prompt_file_render_menu(WINDOW* pWin, char path[PATH_MAX]) {
	// Our viewbag to pass to the prerender state.
	struct menu_prompt_file_render_menu_bag bag;

	// Directory pointers
	DIR *d;
  	struct dirent *dir;

	// Get the actual path for path and store result into resolved_path
	char resolved_path[PATH_MAX];
	realpath(path, resolved_path);
	
	// Open the directory
	d = opendir(resolved_path);
  	
	// Copy our current path to the viewbag.
	strncpy(bag.Path, resolved_path, sizeof(bag.Path));
	
	// If we have a directory
	if (d) {
		// Create the menu
		// ITEM TYPES:
		// 	-1 = ../
		// 	 0 = UNKNOWN
		// 	 1 = FILE
		//   2 = DIRECTORY
		struct Menu* menu = menu_create("Select a file");
		menu->Settings.AttachedData = &bag;
		menu->Settings.pRenderCb = menu_prompt_file_render_menu_prerender;

		// We need to make 2 passes on the directory. 
		// 1. Count how many files are in the directory
		// 2. Loop the items in the directory and store them in an array
		// 	  and create the menu items
		int cnt = 0;
		while ((dir = readdir(d)) != NULL)
			cnt++;
		
		// Add our go back code if we can go back (/ == root)
		if (strcmp(resolved_path, "/") != 0) {
			menu_item_add(menu, IT_BUTTON, "..", -1); 			
		}

		// If we have files
		if (cnt > 0) {
			// Loop the files again
			closedir(d);
			d = opendir(resolved_path);

			// Show the files
			typedef struct {
				char file[PATH_MAX];
				int type;
			} f_item;

			// List of files
			f_item files[cnt];

			// Display string for the menu item.
			char cf[PATH_MAX + 4]; // PATH_MAX + "[.] "
			
			// Set the files and add the menu items
			// Loop the directory
			int x = 0;
			while((dir = readdir(d)) != NULL) {
				// Ignore ".", ".." and other files
				if ( strcmp(dir->d_name, ".") == 0
				     || strcmp(dir->d_name, "..") == 0 ) {
					x++;
					continue;
				}

				// Setup the array
				strncpy(files[x].file, dir->d_name, sizeof(files[x].file));				
				
				// Setup the display string
				memset(cf, 0, sizeof(cf));
				
				// Setup the prefix for the selection (file type indicator)
				// EG: "[D] ", "[F] ", "[?] "
				strcat(cf, "[");
				
				// If the file is a dir
				if(dir->d_type == DT_DIR) {
					files[x].type = 2;
					strcat(cf, "D");
				} 
				
				// If the file is a REG(ular) file.
				else if (dir->d_type == DT_REG) {
					files[x].type = 1;
					strcat(cf, "F");
				} 
				
				// We have something else, these are just ignored
				// for the sake of simplicity, such as symlinks etc.
				else {
					files[x].type = 0;
					strcat(cf, "?");
				}
				
				// Close off our prefix.
				strcat(cf, "] ");
				strncat(cf, files[x].file, sizeof(cf)-4);
				
				// Add the menu item
				menu_item_add(menu, IT_BUTTON, cf, x);
				x++;
			} // loop.

			// Free our directory
			closedir(d);

			// Render the menu
			if (menu_render(pWin, menu)) {
				struct MenuItem* pItem = menu->pSelectedItem;
				
				// We are going back a directory
				if (pItem->Tag == -1) {
					strcat(path, "/..");

					// This is safer then using strcpy or strncpy.
					// we just resolve the new path and then reverse resolved into the
					// path variable.
                    realpath(path, resolved_path);
                    realpath(resolved_path, path);
					menu_free(menu,true);
					return 1; // Loop again
				}

				// Get our item
				f_item item = files[pItem->Tag];

				// If the item is a direrctory or a file.
				if (item.type == 1 || item.type == 2) {
					strcat(path, "/");
					strcat(path, item.file);

					// We have a file
					if (item.type == 1) {
                        menu_free(menu,true);
						return 2;
					}
				}

				menu_free(menu,true); // Free our menu
                return 1; // Loop back
			}

			menu_free(menu,true); // Free our menu
            // We did not select anything
            return -2;
		}

		menu_free(menu,true); // Free our menu
  		return -1;
	}
	
	return 0;
}

/**
 * Render a file selection menu and return the result
 *
 * @param pWin         This is the window the menu will be rendered onto
 * @param pFile  OUT   The file path output
 *
 * @return 0 = DIR_ERROR, -1 = NO_FILES, -2 = NO_SELECTION, 2 = FILE
 */
int menu_prompt_file_select(WINDOW *pWin, struct file_s *pFile) {
	// Clear the window
	wclear(pWin);

	// Set our file output to "." cwd.	
	realpath(".", pFile->Path);

	// Select a file.
	// if r = 1 then we want to loop back and rerender the new directory,
	//
	// 1 = DIR_SELECT
	int r = 1;
	while ((r = menu_prompt_file_render_menu(pWin, pFile->Path)) == 1);
	return r;
}
