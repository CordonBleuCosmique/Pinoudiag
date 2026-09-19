// (C)2010, cphx (Stefan Klempera)
//
// Fork integre tel quel dans pinouNDSdiag (module boutons-tactile).
// Origine, licence et modifications de build : voir PROVENANCE.md a cote
// de ce fichier. Logique de test inchangee par rapport a la source
// d'origine (Input Test DS, domaine public).

   #include <nds.h>
   #include <maxmod9.h>
   #include <stdio.h>

   #include "logo.h"
   #include "soundbank.h"
   #include "soundbank_bin.h"


int main(void) {

 	touchPosition touch;
    // set the mode for 2 text layers and two extended background layers
	videoSetMode(MODE_5_2D);

	// set the sub background up for text display (we could just print to one
	// of the main display text backgrounds just as easily
	videoSetModeSub(MODE_1_2D); //sub bg 0 will be used to print text

	vramSetBankA(VRAM_A_MAIN_BG);

	consoleDemoInit();
	
	mmInitDefaultMem((mm_addr)soundbank_bin);
	
	// load the module
	mmLoad( MOD_XENON );

	// Start playing module
	mmStart( MOD_XENON , MM_PLAY_LOOP );




	// set up our bitmap background
	bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0,0);
	
	decompress(logoBitmap, BG_GFX,  LZ77Vram);
	
	while(1) {
        int NumButtonsPressed;
		int keys_pressed, keys_released;
		int held = keysHeld();		// buttons currently held
		
		swiWaitForVBlank();
		// get key state
		scanKeys();
		// get touchscreen touch coord.
		touchRead(&touch);

    	iprintf("\x1b[2J");
		NumButtonsPressed = 0;
	    if ( not held ) {
		iprintf("\n\n\n\t   CPHX - INPUT TEST DS\n");
		iprintf("\n\tHardware Diagnostic Utility\n");
		iprintf("\n\t\t (U), (08/02/2010)\n");

		iprintf("\n\n\n\t+ Hold 'Start'  for about");
		iprintf("\n\t+ Hold 'Select' for info");
		iprintf("\n\n\n\n\t for the latests updates");
		iprintf("\n\t\t\t please visit");
		iprintf("\n\n\thttp:\\www.sourceforge.net");
		}
		

		keys_pressed = keysDown();
		keys_released = keysUp();
		
		if ( held & KEY_START) {
               iprintf("\n About :");
	 	   iprintf("\n\n This is a hardware diagnostic");
	 	   iprintf("\n utility used to verify the ");
	 	   iprintf("\n integrity of the Nintendo DS");
		   iprintf("\n controls and the touchscreen.");
		   iprintf("\n Useful to quickly examine");
		   iprintf("\n used Nintendo DS devices.");
		   
	 	   iprintf("\n\n\n Have you any jobs to offer ? :");
 		   iprintf("\n\n + Graphic Design");
		   iprintf("\n + Sound Design");
		   iprintf("\n + Web Design");
 		   iprintf("\n + PC Development (DOS16/WIN32)");
 		   iprintf("\n + Mobile & Console Development");
    	   iprintf("\n + Reverse Engineering");
 		   iprintf("\n + Protection Systems");
		   iprintf("\n\n # QB,VB,ASM,C++,HTML,Paintshop");


          NumButtonsPressed++;
		   goto skip;
	    }
		if ( held & KEY_SELECT) {
	 	if ( NumButtonsPressed > 0) {
	 	   goto skip;
		}
              iprintf("\n Credits :");
	 	   iprintf("\n\n Programming: cphx");
	 	   iprintf("\n Graphics:    cphx");
	 	   iprintf("\n\n Music:       Xenon (Remix)");
	 	   iprintf("\n\n Contact :");
		   iprintf("\n\n Mail: cphx@hushmail.com ");
 		   iprintf("\n       cphx@gmx.de ");
           iprintf("\n\n Web:  www.sourceforge.net ");
          iprintf("\n\n Thanks & Greets to ...");
          iprintf("\n\n ... the DevKitPro Team ");
          iprintf("\n ... www.r4i-sdhc.com.tw");
		  iprintf("\n ... Iczilion, Woodman, etc. ");
 		  iprintf("\n ... Kevin Mitnick ");
		  iprintf("\n ... Captn. Crunch ");
 		  iprintf("\n ... Tactic (T4C)");
	    
   	       NumButtonsPressed++;
		   goto skip;
	    }
		if ( held & KEY_A ) {
	       iprintf("\n\t\t+ Button 'A' pressed");
		   NumButtonsPressed++;
		}

		if ( held & KEY_B ) {
	       iprintf("\n\t\t+ Button 'B' pressed");
		   NumButtonsPressed++;
		}

	    if ( held & KEY_X ) {
	       iprintf("\n\t\t+ Button 'X' pressed");
		   NumButtonsPressed++;
		}

	    if ( held & KEY_Y ) {
	       iprintf("\n\t\t+ Button 'Y' pressed");
		   NumButtonsPressed++;
		}

	    if ( held & KEY_L ) {
	       iprintf("\n\t\t+ Button 'L' pressed");
		   NumButtonsPressed++;
		}
		
		
	    if ( held & KEY_R ) {
	       iprintf("\n\t\t+ Button 'R' pressed");
		   NumButtonsPressed++;
		}
        
		if (NumButtonsPressed > 0) {
		iprintf("\n");
		}
		
	    if ( held & KEY_UP ) {
	       iprintf("\n\t\t+ DPAD 'U' pressed");
		   NumButtonsPressed++;
		}
	    if ( held & KEY_DOWN ) {
	       iprintf("\n\t\t+ DPAD 'D' pressed");
		   NumButtonsPressed++;
		}

		if ( held & KEY_LEFT ) {
	       iprintf("\n\t\t+ DPAD 'L' pressed");
		   NumButtonsPressed++;
		}

		if ( held & KEY_RIGHT ) {
	       iprintf("\n\t\t+ DPAD 'R' pressed");
		   NumButtonsPressed++;
		}
		
		if (NumButtonsPressed > 0) {
		iprintf("\n");
	    }

		if ( held & KEY_TOUCH ) {
	 	   iprintf("\n\t\t+ Touchscreen touched");
	 	   iprintf("\n\n\t\t+ X = %04X, %04X", touch.rawx, touch.px);
		   iprintf("\n\t\t+ Y = %04X, %04X", touch.rawy, touch.py);		
           NumButtonsPressed++;
		}
		

	if (NumButtonsPressed > 0) {
		iprintf("\n\n\t\tButtons pressed : %i", NumButtonsPressed);
        }
skip:;
        
	}
	return 0;
}
