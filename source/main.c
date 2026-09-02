/*---------------------------------------------------------------------------------

	Basic template code for starting a DS app

---------------------------------------------------------------------------------*/
//Includes
#include <fat.h>
#include <nds.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

//Definitions
#define MAX_ITEMS 32

//Constants
//const char* items[] = {"MittROMney", "ROMnaldRegan", "ROMnaldMcdonald", "Barack OROMba"};
//const int itemcount = sizeof(items)/sizeof(items[0]);

//Variables
char* items[MAX_ITEMS]; //static array for printing dump from SD
char  item_storage[MAX_ITEMS][256]; //character storage for readdir buffer, 32 items, each with a 256 byte buffer
char item_type[MAX_ITEMS]; //array to test if entry is a directory
//int itemcount = sizeof(items)/sizeof(items[0]); 
int itemcount = 0;
//Functions
void drawMenu(int selection) {
		//clears screen
		iprintf("\x1b[2J");
		//prints list, conditional selector print based on state, and directory signifier prefix
		for (int i = 0 ; i < itemcount; i++) {
			const char* prefix = (i == selection) ? ">" : " ";
			const char* dprefix = (item_type[i] == DT_DIR) ? "/" : " ";
			iprintf("\x1b[%d;1H %s %s %s", 5 + i, prefix, dprefix, items[i]);
		}
	}
//---------------------------------------------------------------------------------
int main(int argc, char* argv[]) {
//---------------------------------------------------------------------------------

	consoleDemoInit();
	if (fatInitDefault()) {
		iprintf("\x1b[3;2H FAT loads");
	}else{
		iprintf("\x1b[3;2H FAT fails");
	}
	struct dirent * entry; // pointer for each entry in filesystem
	DIR * dir; //pointer for directory
	int selection = 0; //selector state variable
	//char buffer[256]; //buffer variable from testing getcwd()
	dir = opendir(".");
	//iprintf("\x1b[4;1H %p", dir); diagnostic line, shows if opendir is returning null or not
	if (dir != NULL ){
		while ((entry = readdir(dir)) != NULL && itemcount < MAX_ITEMS){
			if (entry->d_name[0] == '.') 
				continue;
			if (entry->d_type == DT_DIR){
				item_type[itemcount] = entry->d_type;
				strncpy(item_storage[itemcount], entry->d_name, 255); //readdir returns pointer to buffer, strncpy used to copy bytes(characters) from buffer to persistent storage
				item_storage[itemcount][255] = '\0'; //adds null byte to end of string to guarantee it is terminated, preventing over read (Heartbleed reference)
				items[itemcount] = item_storage[itemcount]; //appends string to array for SD dump
				itemcount++;
				continue;
			}
			char *dot_loc;
			dot_loc = strrchr(entry->d_name, '.');
			if (dot_loc != NULL){
				if (strcasecmp(dot_loc, ".nds") != 0)
					continue;
			}
			else{
				continue;
			}
			item_type[itemcount] = entry->d_type;
			strncpy(item_storage[itemcount], entry->d_name, 255); //readdir returns pointer to buffer, strncpy used to copy bytes(characters) from buffer to persistent storage
			item_storage[itemcount][255] = '\0'; //adds null byte to end of string to guarantee it is terminated, preventing over read (Heartbleed reference)
			items[itemcount] = item_storage[itemcount]; //appends string to array for SD dump
			itemcount ++;
		}
		if (readdir(dir) != NULL){
			iprintf("\x1b[3;1H output truncated by display limit");
		}
		closedir(dir);
	}else{
		iprintf("\x1b[3;1H error: null pointer from open directory operation");
	}
	if (itemcount == 0){
		iprintf("\x1b[4;2H No files found"); //when items is empty, prevents potential crashes and prints helpful message
	}
	drawMenu(selection); //initial menu draw
	

	while(pmMainLoop()) {
		swiWaitForVBlank();
		scanKeys();
		int pressed = keysDown();
		//Scroll up, wraps to bottom
		if(pressed & KEY_UP && itemcount > 0) {
			selection --;
			//Sets selector to itemcount-1 (last place in 0 index array) if selector goes "above" top
			if (selection < 0){
				selection = itemcount - 1;
			}
			drawMenu(selection);
		}
		//Scroll down, wraps to top
		if(pressed & KEY_DOWN && itemcount > 0) {
			selection ++;
			//Sets selector to 0 (first place in 0 index array) if selector goes "below" bottom
			if (selection == itemcount) {
				selection = 0;
			}
			drawMenu(selection);
		}
		//Displays select message when a button is pressed
		if(pressed & KEY_A && itemcount > 0) {
			drawMenu(selection);
			iprintf("\x1b[4;1H ROM %s selected", items[selection]);
		}
		if(pressed & KEY_START) break;
	}

	return 0;

}
