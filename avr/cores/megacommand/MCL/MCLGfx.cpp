#include "MCL.h"
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

  GUI.setPage(&grid_page);
}

MCLGfx gfx;
