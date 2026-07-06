/*---------------------------------------------------------------------------------

	Basic template code for starting a DS app

---------------------------------------------------------------------------------*/
//Includes
#include <nds.h>
#include <stdio.h>

//Constants
const char* items[] = {"MittROMney", "ROMnaldRegan", "ROMnaldMcdonald"};
const int itemcount = sizeof(items)/sizeof(items[0]);
//Functions
void drawMenu(int selection) {
		//clears screen
		iprintf("\x1b[2J");
		//conditional selector print based on state
		if (selection == 0) { 
			iprintf("\x1b[5;10H--> %s", items[0]);
		} else {
			iprintf("\x1b[5;10H %s", items[0]);
		}

		if (selection == 1) {
			iprintf("\x1b[6;10H--> %s", items[1]);
		} else {
			iprintf("\x1b[6;10H %s", items[1]);
		}

		if (selection == 2) {
			iprintf("\x1b[7;10H --> %s", items[2]);
		} else {
			iprintf("\x1b[7;10H %s", items[2]);
		}
	}
//---------------------------------------------------------------------------------
int main(int argc, char* argv[]) {
//---------------------------------------------------------------------------------

	consoleDemoInit();
	int selection = 0; //selector state variable
	drawMenu(selection); //initial menu draw

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
