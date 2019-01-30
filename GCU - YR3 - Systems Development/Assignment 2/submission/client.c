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

typedef enum _PACKET_TYPES {
	PT_ERROR_UNKNOWN = 0, 

	PT_RANDOM,
	PT_UNAME,
	PT_UPLOAD,
	PT_DOWNLOAD,
	PT_BROWSER,

	PT_EXIT
} PACKET_TYPES;

#define clrscr() \
    printf("\e[1;1H\e[2J")
#define MENU_TEXT_SIZE PATH_MAX

#define bool	_Bool
#define true	1
#define false	0

struct Menu;

enum ItemType   { IT_TEXT, IT_BUTTON };

struct MenuItem {
    enum    ItemType        Type;
            char            Text[MENU_TEXT_SIZE];
            int             Tag;
    struct  MenuItem*       pPrevious;
    struct  MenuItem*       pNext;
};

struct MenuItems {
    struct  MenuItem*       pFirst; 
    struct  MenuItem*       pLast;
            int             Count;
};

typedef void (*CBMenuRender)(WINDOW*, struct Menu*);

struct MenuSettings {
            int             RenderSize;
            int             ViewStartIndex;
            CBMenuRender    pRenderCb;
            void*           AttachedData;
};

struct Menu {
            char            Title[MENU_TEXT_SIZE];
            int             HighlightedIndex;
    struct  MenuItem*       pSelectedItem;
    struct  MenuItems       Items;
    struct  MenuSettings    Settings;
};

struct file_s { 
    char Path[PATH_MAX];
};

void environment_setup(void);
void environment_end(void);

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

void get_hello(int socket)
{
    char hello_string[256];
    size_t k;

    readn(socket, (unsigned char *) &k, sizeof(size_t));	
    readn(socket, (unsigned char *) hello_string, k);

    printf("Hello String: %s\n", hello_string);
    printf("Received: %zu bytes\n\n", k);
}

void sget_hello(int socket, char* dest) {
    char hello_string[256];
    size_t k;

    readn(socket, (unsigned char *) &k, sizeof(size_t));
    readn(socket, (unsigned char *) hello_string, k);
    sprintf(dest, "%s", hello_string);
}

int action_random(int s) {
	clear();

	printw("Sending packet request... ");

	PACKET_TYPES req = PT_RANDOM;
	if (writen(s, (unsigned char *) &req, sizeof(PACKET_TYPES)) == -1)
		return 1;

	printw("DONE.\nReading random numbers...\n\n");

	int random[5];
	memset(random, 0, sizeof(random));

	if (readn(s, &random, sizeof(random)) == -1)
		return 1;

	for(int x = 0; x < 5; x++)
		printw("Random [%d] = %d\n", x, random[x]);

	printw("\nPress any key to continue: \n");
	getch();

	return 0;
}

int action_uname(int s) {
	clear();

	printw("Sending packet request... ");

	PACKET_TYPES req = PT_UNAME;
	if (writen(s, (unsigned char *) &req, sizeof(PACKET_TYPES)) == -1)
		return 1;

	printw("DONE.\nReading uname from server... ");

	int status = -1;
	struct utsname uname;

	if (readn(s, &status, sizeof(int)) == -1)
		return 1;

	if (status != 0) {
		printw("ERROR.\n\nFailed to receive uname from server, Error no = %d\n\n", status);
		goto wait;
	}

	if (readn(s, &uname, sizeof(uname)) == -1)
		return 1;

	printw("DONE.\n\n");
	
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

int action_upload(int s) {
	struct file_s file;
	ssize_t r;
	bool wait = true;

	memset (&file, 0, sizeof(file));
	
	if (menu_prompt_file_select(stdscr, &file) == 2) {
	    clear();
		printw("Selected file: %s\n", file.Path);
		printw("READING FILE...                      ");

		FILE* f =fopen(file.Path,"rb");

		fseek(f, 0, SEEK_END);
		long size = ftell(f);
		fseek(f, 0, SEEK_SET);

		unsigned char* buffer = (unsigned char*) malloc(size * sizeof(unsigned char));
		memset(buffer, 0, size * sizeof(unsigned char));
		size_t result = fread (buffer,1,size,f);

		if (result != size) {
			printw("[ NOT OK ]\n");
			printw("  There was an error while reading the file.\n");

			free(buffer);
			fclose(f);
			goto wait;
		}
		
		printw("[ OK ]\n");
		printw("SENDING FILE TO SERVER...            ");

		PACKET_TYPES req = PT_UPLOAD;
		if (writen(s, (unsigned char *) &req, sizeof(PACKET_TYPES)) == -1)
			return 1;

		char*  fname = basename(file.Path);
		size_t nameSize = strlen(fname);

        if (writen(s, (unsigned char *) &nameSize, sizeof(size_t)) == -1)
            return 1;
		
        if (writen(s, (unsigned char *) fname, nameSize * sizeof(char)) == -1)
            return 1;
		
		if (writen(s, (unsigned char *) &size, sizeof(long)) == -1)
			return 1;
		if ((r=writen(s, (unsigned char *) buffer, size * sizeof(unsigned char))) == -1)
			return 1;

		printw("[ OK ]\n");
		fclose(f);
		free(buffer);
		printw("WAITING FOR SERVER TO RESPOND...     ");
		
		int server_status = 0; 
		if ((readn(s, (unsigned char*) &server_status, sizeof(int))) == -1) 
			return 1;

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
		wait = false;
	}

wait:
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

int action_download(int s) {
	clear();

	unsigned char *buffer = NULL;
	FILE* write_ptr = NULL;
	int fileCount = 0;

	PACKET_TYPES request = PT_BROWSER;
	if ( writen(s, (unsigned char *) &request, sizeof(int)) == -1 ) 
		return 1;

	printw("RECEIVING FILE COUNT...     ");
	if ( readn(s, (unsigned char *) &fileCount, sizeof(int)) == -1 ) 
		return 1;
	
	printw("[ OK ]   %d files found.\n", fileCount);
	
	if (fileCount == 0) {
		printw("  There are 0 files in the remote folder.");
		printw("\n\nPress Any Key to Continue.\n");
		getch();
		refresh();
		
		return 0;
	}
	
	struct Menu* menu = menu_create("");
	menu->Settings.pRenderCb = action_download_prerender;

	int n = 0;
	char files[fileCount][PATH_MAX];

	for(int x = 0; x < fileCount; x++) {
		if (readn(s, (unsigned char *) files[n], PATH_MAX) == -1)
			goto socketFailure;
		
		menu_item_add(menu, IT_BUTTON, files[n], n);
		n++;
	}

	if (menu_render(stdscr, menu) == 1) {
		wclear(stdscr);
		printw("SELECTED FILE: %s\n", files[menu->pSelectedItem->Tag]);
		printw("SENDING SERVER REQUEST...            ");
		
		request = PT_DOWNLOAD;
		if ( writen(s, (unsigned char *) &request, sizeof(int)) == -1 )
			goto socketFailure;
	
		if ( writen(s, (unsigned char *) &files[menu->pSelectedItem->Tag], PATH_MAX) == -1 )
			goto socketFailure;

		printw("[ OK ]\n");
		printw("CHECKING FILE AVAILABILITY...        ");
	
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

		size_t fileSize;
		if ( readn(s, (unsigned char *) &fileSize, sizeof(size_t)) == -1 )
			goto socketFailure;
		
		printw("[ OK ]\n");
		printw("READING IN FILE BUFFER...            ");

		buffer = (unsigned char*) malloc(fileSize);
		if ( readn(s, buffer, fileSize) == -1 )
			goto socketFailure;
		
		printw("[ OK ]\n");
		
		printw("WRITING FILE DATA...                 ");
		
		char tmp__filePath[PATH_MAX];
		char filePath[PATH_MAX];
		memset(tmp__filePath, 0, sizeof(tmp__filePath));

		strcat   ( tmp__filePath, "./" );
		strcat   ( tmp__filePath, files[menu->pSelectedItem->Tag] );
		tmp__filePath[PATH_MAX-1] = 0;
		realpath ( tmp__filePath, filePath );

    	write_ptr = fopen(filePath,"wb");
		
		if (write_ptr == NULL) {
			printw("[ NOT OK ]\n");
			printw("  Failed to write because file pointer is null.");
			goto finish;
		}	

		if (fwrite(buffer, fileSize, 1, write_ptr) != 1) {
			printw("[ NOT OK ]\n");
			printw("  Failed to write file. (fwrite != 1)");
			goto finish;
		}

		printw("[ OK ]");

		goto finish;

socketFailure:
		if (menu != NULL)
			menu_free(menu, true);
		
		if (buffer != NULL)		free(buffer);
		if (write_ptr != NULL)	fclose(write_ptr);

		return 1;
	}
	
finish:
	menu_free(menu,true);
	
	if (buffer != NULL)		free(buffer);
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

int action_browse(int s) {
	clear();

	PACKET_TYPES request = PT_BROWSER;
	if ( writen(s, (unsigned char *) &request, sizeof(int)) == -1 ) 
		return 1;
	
	int fileCount = 0;
	char file[PATH_MAX];
	
	printw("RECEIVING FILE COUNT...     ");
	if ( readn(s, (unsigned char *) &fileCount, sizeof(int)) == -1 ) 
		return 1;
	
	printw("[ OK ]   %d files found.\n", fileCount);
	
	if (fileCount == 0) {
		printw("  There are 0 files in the remote folder.");
		printw("\n\nPress Any Key to Continue.\n");
		getch();
		refresh();
		
		return 0;
	}
	
	struct Menu* menu = menu_create("");
	menu->Settings.pRenderCb = action_browse_prerender;

	for(int x = 0; x < fileCount; x++) {
		if (readn(s, (unsigned char *) file, PATH_MAX) == -1)
			return 1;
		
		menu_item_add(menu, IT_BUTTON, file, -1);
		printw("file: %s\n", file);
		refresh();
	}

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

int application(int sockfd) {
	int server_state = 0;

    struct main_menu_bag bag;
    sget_hello(sockfd, bag.hello);
    environment_setup();

#if MENU_DEBUG
    printw("Attach a debugger now!");
    refresh();
    getch();
#endif

    struct Menu* menu = menu_create("Main Menu");
    menu->Settings.AttachedData = &bag;
    menu->Settings.pRenderCb = &main_menu_prerender;

	menu_item_add(menu, IT_TEXT, "Server:", -1);
	struct MenuItem* pbRand  = menu_item_add(menu, IT_BUTTON, "Random Numbers", -1);
	struct MenuItem* pbUname = menu_item_add(menu, IT_BUTTON, "Uname Information", -1);

	menu_item_add(menu, IT_TEXT, "Remote Files:", -1);
	struct MenuItem* pbUpload   = menu_item_add(menu, IT_BUTTON, "Upload", -1);
	struct MenuItem* pbDownload = menu_item_add(menu, IT_BUTTON, "Download", -1);
	struct MenuItem* pbBrowse   = menu_item_add(menu, IT_BUTTON, "Browse Directory", -1);

	menu_item_add(menu, IT_TEXT, "Application:", -1);
	struct MenuItem* pbExit = menu_item_add(menu, IT_BUTTON, "Exit", -1);

	bool exiting = false;
	
	while (!exiting) {
        int state = menu_render(stdscr, menu);
        if (state != 1) {
            exiting = true;
            break;
        }

        struct MenuItem* i = menu->pSelectedItem;
        if (i == NULL)
            continue;

		if (i == pbExit) {
			exiting = true;
			break;
		}
		
		else if (i == pbRand) {
			if (action_random(sockfd)) {
				server_state = 1;
				goto cleanup;
			};
		}
		
		else if (i == pbUname) {
			if (action_uname(sockfd)) {
				server_state = 1;
				goto cleanup;
			};
		}
		
		else if (i == pbUpload) {
			if (action_upload(sockfd)) {
				server_state = 1;
				goto cleanup;
			};
		}

		else if (i == pbDownload) {
			if (action_download(sockfd)) {
				server_state = 1;
				goto cleanup; 
			};
		}

		else if (i == pbBrowse) {
			if (action_browse(sockfd)) {
				server_state = 1;
				goto cleanup;
			}
		}
	}

cleanup:
	menu_free(menu, true);
    environment_end();

    return server_state;
}

int main(void)
{
    int sockfd = 0;
    struct sockaddr_in serv_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Error - could not create socket");
		exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(50031);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)  {
	    perror("Error - connect failed");
	    exit(1);
    } else
       printf("Connected to server...\n");

	if (application(sockfd)) {
		printf("Disconnected from server! (CONNECTION DROPPED)\n");
		goto cleanup;
	}

	PACKET_TYPES req = PT_EXIT;
	writen(req, (unsigned char *) &req, sizeof(PACKET_TYPES));

cleanup:
    close(sockfd);
    exit(EXIT_SUCCESS);
}

void environment_setup(void) {
    initscr();
    start_color();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
}

void environment_end(void) {
    endwin();
    setvbuf(stdout, NULL, _IOLBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
}

struct Menu* menu_create(char Title[MENU_TEXT_SIZE]) {
    struct Menu* menu = malloc(sizeof(struct Menu));
    memset(menu, 0, sizeof(struct Menu));

    strncpy(menu->Title, Title, sizeof(menu->Title));
    menu->HighlightedIndex = -1;
    menu->pSelectedItem = NULL;

    menu->Settings.RenderSize = 10;
    menu->Settings.ViewStartIndex = 0;
    menu->Settings.pRenderCb = NULL;
    menu->Settings.AttachedData = NULL;

    return menu;
}

void menu_free(struct Menu* pMenu, bool freeItems) {
    if (freeItems)
        menu_item_free_all(pMenu);

    free(pMenu);
}

struct MenuItem* menu_item_add(struct Menu* pMenu, enum ItemType Type, char Text[MENU_TEXT_SIZE], int Tag) {
    struct MenuItem* item = malloc(sizeof(struct MenuItem));
    memset(item, 0, sizeof(struct MenuItem));

    item->pNext = NULL;
    item->pPrevious = NULL;

    item->Type = Type;
    strncpy(item->Text, Text, sizeof(item->Text));
    item->Tag  = Tag;

    if (pMenu->Items.Count == 0) {
        pMenu->Items.pFirst = item;
		
        if (item->Type == IT_BUTTON)
            pMenu->HighlightedIndex = 0;
    }

    else {
        pMenu->Items.pLast->pNext = item;
        item->pPrevious = pMenu->Items.pLast;
    }

    pMenu->Items.Count++;
    pMenu->Items.pLast = item;
    return item;
}

int menu_item_find_index(struct Menu* pMenu, struct MenuItem* pWanted) {
    struct MenuItem* pItem = pMenu->Items.pFirst;

    int x = 0;
    while (pItem != NULL) {
        if (pItem == pWanted)
            return x;

        pItem = pItem->pNext;
        x++;
    }
	
    return -1;
}

struct MenuItem* menu_item_find_tag(struct Menu *pMenu, int Tag) {
    if (pMenu->Items.Count == 0)
        return NULL;

    struct MenuItem* pCurrent = pMenu->Items.pFirst;
    while (pCurrent != NULL) {
        if (pCurrent->Tag == Tag)
            return pCurrent;

        pCurrent = pCurrent->pNext;
    }

    return NULL;
}

int menu_item_find_next_type_index_up(struct Menu *pMenu, struct MenuItem *pItem, enum ItemType type,
                                      int currentIndex) {
	int index = currentIndex + 1;

	struct MenuItem* pCurrent = pItem->pNext;
	while (pCurrent != NULL) {
	    if (pCurrent->Type == type)
			return index;

		pCurrent = pCurrent->pNext;
		index++;
	}	

	return -1;
}

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

int menu_item_find_nearest_type_index(struct Menu* pMenu, struct MenuItem* pItem, enum ItemType type,
                                      bool deletion) {
	if (pMenu->Items.Count == 0
		|| (deletion && pMenu->Items.Count == 1))
		return -1;

    if (pMenu->Items.Count == 1) return -1;

	int currentIndex = menu_item_find_index(pMenu, pItem);
	int newIndex = -1;
	
	if (currentIndex == 0) {
	    newIndex = menu_item_find_next_type_index_up(pMenu, pItem, type, currentIndex);
	}

	else if (currentIndex + 1 == pMenu->Items.Count) {
		newIndex = menu_item_find_next_type_index_down(pMenu, pItem, type, currentIndex);
	}

	else {
        newIndex = menu_item_find_next_type_index_down(pMenu, pItem, type, currentIndex);

        if (newIndex == -1) {
            newIndex = menu_item_find_next_type_index_up(pMenu, pItem, type, currentIndex);

            if (newIndex > -1 && deletion)
                newIndex--;
		}
    }
	
    return newIndex;
}	

void menu_item_free(struct Menu* pMenu, struct MenuItem* pItem) {
    struct MenuItem *Previous = pItem->pPrevious;
    struct MenuItem *Next = pItem->pNext;

    if (pMenu->Items.Count == 1)
        pMenu->HighlightedIndex = -1;
	else
        pMenu->HighlightedIndex = menu_item_find_nearest_type_index(pMenu, pItem, IT_BUTTON, true);

    if (Previous == NULL && Next != NULL) {
        pMenu->Items.pFirst = Next;
        pMenu->Items.Count--;
        Next->pPrevious = NULL;
    }

    else if (Previous != NULL && Next == NULL) {
        pMenu->Items.pLast = Previous;
        pMenu->Items.Count--;
        Previous->pNext = NULL;
    }

    else if (Previous == NULL && Next == NULL) {
        pMenu->Items.Count = 0;
        pMenu->Items.pLast = NULL;
        pMenu->Items.pFirst = NULL;
        pMenu->pSelectedItem = NULL;
    }

    else if (Previous != NULL && Next != NULL) {
        Previous->pNext = Next;
        Next->pPrevious = Previous;
        pMenu->Items.Count--;
    }

    if (pMenu->pSelectedItem == pItem)
        pMenu->pSelectedItem = NULL;

    free(pItem);
}

void menu_item_free_all(struct Menu *pMenu) {
    struct MenuItem* pItem = pMenu->Items.pFirst;

    while (pItem != NULL) {
	    struct MenuItem* currentItem = pItem;
	    pItem = pItem->pNext;
        free(currentItem);
    }

    pMenu->pSelectedItem = NULL;
    pMenu->Items.Count = 0;
}

void menu_render_item(WINDOW *pWin, int index, struct Menu *this, struct MenuItem *item) {
#if DEBUG_MENU
    if (item->Type == IT_TEXT) {
        wprintw(pWin, "  %s\t[%d](%p)\n", item->Text, index, item);
	}
    else if (item->Type == IT_BUTTON) {
        if (index == this->HighlightedIndex) {
            wprintw(pWin, " ");
            attron(A_REVERSE);
            wprintw(pWin, " > %s < ", item->Text);
            attroff(A_REVERSE);
       		wprintw(pWin, "\t[%d](%p)\n", index, item);
		}

        else {
            wprintw(pWin, "    %s   \t[%d](%p)\n", item->Text, index, item);
		}
    }
#else
	if (item->Type == IT_TEXT) {
		wprintw(pWin, " %s\n", item->Text);
	}
	else if (item->Type == IT_BUTTON) {
		if (index == this->HighlightedIndex) {
			wprintw(pWin, " ");
			attron(A_REVERSE);
			wprintw(pWin, " > %s < ", item->Text);
			attroff(A_REVERSE);
			wprintw(pWin, "\n");
		}
		else {
			wprintw(pWin, "    %s\n", item->Text);
		}
	}
#endif
}

void menu_render_move_down(struct Menu *pMenu) {
    struct MenuItem *pItem = pMenu->Items.pFirst;
    bool foundButton = false;

    int i = 0;
    while (pItem != NULL) {
        if (i == pMenu->HighlightedIndex) {
            if (pItem->pNext != NULL) {
                struct MenuItem *pButton = pItem->pNext;
                i++;

                while(pButton != NULL) {
                    if (pButton->Type == IT_BUTTON) {
                        pMenu->HighlightedIndex = i;
                        foundButton = true;
                        goto viewport;
                    }
					
                    pButton = pButton->pNext;
                    i++;
                }
            }

            goto viewport;
        }

        pItem = pItem->pNext;
        i++;
    }

viewport:
	if (foundButton) {
		int hi = pMenu->HighlightedIndex;
		int vsi = pMenu->Settings.ViewStartIndex;
		int rs = pMenu->Settings.RenderSize;
		
		if (hi > (vsi + rs - 1)) {
			pMenu->Settings.ViewStartIndex++;

			if (hi > (vsi + rs)) {
				int newvsi = hi - vsi - 1;
				if (newvsi < 0) newvsi = 0;

				pMenu->Settings.ViewStartIndex = newvsi;	
			}
		}
	} else {
		if (pMenu->Settings.ViewStartIndex < (pMenu->Items.Count - pMenu->Settings.RenderSize)) {
			pMenu->Settings.ViewStartIndex++;
		}
	}
}

void menu_render_move_up(struct Menu *pMenu) {
    bool foundButton = false;
    if (pMenu->HighlightedIndex == 0)
        goto viewport;

    struct MenuItem *pItem = pMenu->Items.pFirst;
    int i = 0;
    while (pItem != NULL) {
        if (i == pMenu->HighlightedIndex) {
            if (pItem->pPrevious != NULL) {

                struct MenuItem *pButton = pItem->pPrevious; i--;
                while (pButton != NULL) {
                    if (pButton->Type == IT_BUTTON) {
                        pMenu->HighlightedIndex = i;
                        foundButton = true;
                        goto viewport;
                    }

                    pButton = pButton->pPrevious;
                    i--;
                }
            }

            goto viewport;
        }

        pItem = pItem->pNext;
        i++;
    }

viewport:
    if (foundButton) {
        if (pMenu->HighlightedIndex < pMenu->Settings.ViewStartIndex)
            pMenu->Settings.ViewStartIndex = pMenu->HighlightedIndex;
    } else {
        if (pMenu->Settings.ViewStartIndex > 0)
            pMenu->Settings.ViewStartIndex--;
    }
}

int menu_render_select_highlighted(struct Menu *pMenu) {
    struct MenuItem* pItem = pMenu->Items.pFirst;
    int i = 0;
    while (pItem != NULL) {
        if (i == pMenu->HighlightedIndex) {
            if (pItem->Type != IT_BUTTON)
                return 1;

            pMenu->pSelectedItem = pItem;
            return 0;
        }

        pItem = pItem->pNext;
        i++;
    }

    return 1;
}

#ifdef DEBUG_MENU

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

void menu_dbg_delete_highlighted(struct Menu *pMenu) {
    struct MenuItem *pItem = pMenu->Items.pFirst;
    int i = 0;
    while (pItem  != NULL) {
        if (i == pMenu->HighlightedIndex) {
            menu_item_free(pMenu, pItem);
            return;
        }

        pItem = pItem->pNext;
        i++;
    }
}

#endif

int menu_render(WINDOW *pWin, struct Menu *pMenu) {
    int rendering = 1,
        result = 0;
	
    pMenu->pSelectedItem = NULL;

    if (pMenu->Items.Count > 0) {
        if (pMenu->Items.pFirst->Type == IT_BUTTON)
            pMenu->HighlightedIndex = 0;
        else
            pMenu->HighlightedIndex = menu_item_find_next_type_index_up(pMenu, pMenu->Items.pFirst, IT_BUTTON, 0);
    }

    while(rendering == 1) {
        clear();

#if DEBUG_MENU
        wprintw(pWin, "HI(%d) VSI(%d)\n", pMenu->HighlightedIndex, pMenu->Settings.ViewStartIndex);
		wprintw(pWin, "Item count: %d\n\n", pMenu->Items.Count);
        wprintw(pWin, "%s\n======-======-======-======\n", pMenu->Title);
#endif
        if (pMenu->Settings.pRenderCb != NULL)
            pMenu->Settings.pRenderCb(pWin, pMenu);

        struct MenuItem* pItem = pMenu->Items.pFirst;
        int i = 0, v = 1;
        while (pItem != NULL) {
            if (i >= pMenu->Settings.ViewStartIndex && v <= pMenu->Settings.RenderSize) {
                menu_render_item(pWin, i, pMenu, pItem);
                v++;
            }

            pItem = pItem->pNext;
            i++;
        }

        switch(wgetch(pWin)) {
            case 10:
                rendering = menu_render_select_highlighted(pMenu);
                result = 1;
                break;
            case 113:
            case 27:
                rendering = 0;
                result = 0;
                break;
            case 258:
                menu_render_move_down(pMenu);
				break;
            case 259:
                menu_render_move_up(pMenu);
				break;
#ifdef DEBUG_MENU
	        case 100:
                menu_dbg_delete_highlighted(pMenu);
                break;
            case 260: break;
            case 261: break;
            case 98:
                menu_dbg_add_item(pWin, pMenu, IT_TEXT);
                break;
            case 97:
                menu_dbg_add_item(pWin, pMenu, IT_BUTTON);
                break;
#endif
            default: break;
        }

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

int menu_prompt_file_render_menu(WINDOW* pWin, char path[PATH_MAX]) {
	struct menu_prompt_file_render_menu_bag bag;

	DIR *d;
  	struct dirent *dir;

	char resolved_path[PATH_MAX];
	realpath(path, resolved_path);
	
	d = opendir(resolved_path);
	strncpy(bag.Path, resolved_path, sizeof(bag.Path));
	
	if (d) {
		struct Menu* menu = menu_create("Select a file");
		menu->Settings.AttachedData = &bag;
		menu->Settings.pRenderCb = menu_prompt_file_render_menu_prerender;

		int cnt = 0;
		while ((dir = readdir(d)) != NULL)
			cnt++;
		
		if (strcmp(resolved_path, "/") != 0) {
			menu_item_add(menu, IT_BUTTON, "..", -1); 			
		}

		if (cnt > 0) {
			closedir(d);
			d = opendir(resolved_path);
			
			typedef struct {
				char file[PATH_MAX];
				int type;
			} f_item;

			f_item files[cnt];

			char cf[PATH_MAX + 4];
			
			int x = 0;
			while((dir = readdir(d)) != NULL) {
				if ( strcmp(dir->d_name, ".") == 0
				     || strcmp(dir->d_name, "..") == 0 ) {
					x++;
					continue;
				}

				strncpy(files[x].file, dir->d_name, sizeof(files[x].file));				
				memset(cf, 0, sizeof(cf));
				strcat(cf, "[");
				
				if(dir->d_type == DT_DIR) {
					files[x].type = 2;
					strcat(cf, "D");
				} 
				
				else if (dir->d_type == DT_REG) {
					files[x].type = 1;
					strcat(cf, "F");
				} 
				
				else {
					files[x].type = 0;
					strcat(cf, "?");
				}
				
				strcat(cf, "] ");
				strncat(cf, files[x].file, sizeof(cf)-4);
				menu_item_add(menu, IT_BUTTON, cf, x);
				x++;
			}

			closedir(d);

			if (menu_render(pWin, menu)) {
				struct MenuItem* pItem = menu->pSelectedItem;
				
				if (pItem->Tag == -1) {
					strcat(path, "/..");

                    realpath(path, resolved_path);
                    realpath(resolved_path, path);
					menu_free(menu,true);
					return 1;
				}

				f_item item = files[pItem->Tag];

				if (item.type == 1 || item.type == 2) {
					strcat(path, "/");
					strcat(path, item.file);

					if (item.type == 1) {
                        menu_free(menu,true);
						return 2;
					}
				}

				menu_free(menu,true);
                return 1;
			}

			menu_free(menu,true);
            return -2;
		}

		menu_free(menu,true);
  		return -1;
	}
	
	return 0;
}

int menu_prompt_file_select(WINDOW *pWin, struct file_s *pFile) {
	wclear(pWin);
	realpath(".", pFile->Path);
	int r = 1;
	while ((r = menu_prompt_file_render_menu(pWin, pFile->Path)) == 1);
	return r;
}
