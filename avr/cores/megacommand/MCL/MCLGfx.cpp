#include "MCLGfx.h"

void MCLGfx::splashscreen() {

  char str1[17] = "MEGACOMMAND LIVE";
  char str2[17] = "V2.x.x";
  str1[16] = '\0';
  LCD.goLine(0);
  LCD.puts(str1);
  LCD.goLine(1);
  LCD.puts(str2);

  delay(100);
  // while (rec_global == 0) {


  GUI.setPage(&page);
}

void MCLGfx:draw_notes(uint8_t line_number) {
 if (line_number == 0) {
    GUI.setLine(GUI.LINE1);
  }
  else {
    GUI.setLine(GUI.LINE2);
  }
  /*Initialise the string with blank steps*/
  char str[17] = "----------------";

  /*Display 16 track cues on screen,
   For 16 tracks check to see if there is a cue*/
  for (int i = 0; i < 16; i++) {
    if (curpage == CUE_PAGE) {

      if  (IS_BIT_SET32(cfg.cues, i)) {
        str[i] = 'X';
      }
    }
    if (notes[i] > 0)  {
      /*If the bit is set, there is a cue at this position. We'd like to display it as [] on screen*/
      /*Char 219 on the minicommand LCD is a []*/

      str[i] = (char) 219;
    }

  }

  /*Display the cues*/
  GUI.put_string_at(0, str);
}
MCLGfx gfx;
