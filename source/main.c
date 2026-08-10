/*---------------------------------------------------------------------------------

	Basic template code for starting a DS app

---------------------------------------------------------------------------------*/
//Includes
#include <fat.h>
#include <nds.h>
#include <stdio.h>

//Constants
const char* items[] = {"MittROMney", "ROMnaldRegan", "ROMnaldMcdonald", "Barack OROMba"};
const int itemcount = sizeof(items)/sizeof(items[0]);
//Functions
void drawMenu(int selection) {
		//clears screen
		iprintf("\x1b[2J");
		//prints list and conditional selector print based on state
		for (int i = 0 ; i < itemcount; i++) {
			const char* prefix = (i == selection) ? "-->" : "   ";
			iprintf("\x1b[%d;10H %s %s", 5 + i, prefix, items[i]);
		}
	}
//---------------------------------------------------------------------------------
int main(int argc, char* argv[]) {
//---------------------------------------------------------------------------------

	consoleDemoInit();
	int selection = 0; //selector state variable
	drawMenu(selection); //initial menu draw
	//storage check
	if (fatInitDefault()) {
		iprintf("\x1b[8;1H DSi mode: %d", isDSiMode());
		iprintf("\x1b[10;1H FAT OK");
	} else {
		iprintf("\x1b[8;1H DSi mode: %d", isDSiMode());
		iprintf("\x1b[10;1H FAT FAILED");
	}
	
	while(pmMainLoop()) {
		swiWaitForVBlank();
		scanKeys();
		int pressed = keysDown();
		//Scroll up, wraps to bottom
		if(pressed & KEY_UP) {
			selection --;
			//Sets selector to itemcount-1 (last place in 0 index array) if selector goes "above" top
			if (selection < 0){
				selection = itemcount - 1;
			}
			drawMenu(selection);
		}
		//Scroll down, wraps to top
		if(pressed & KEY_DOWN) {
			selection ++;
			//Sets selector to 0 (first place in 0 index array) if selector goes "below" bottom
			if (selection == itemcount) {
				selection = 0;
			}
			drawMenu(selection);
		}
		//Displays select message when a button is pressed
		if(pressed & KEY_A) {
			drawMenu(selection);
			iprintf("\x1b[10;2H ROM %s selected", items[selection]);
		}
		if(pressed & KEY_START) break;
	}

	return 0;

}
