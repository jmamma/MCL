#include "MCL.h"
#include "MCLGfx.h"

void MCLGfx::splashscreen() {
#ifdef OLED_DISPLAY
  oled_display.setTextSize(2);
  oled_display.setTextColor(WHITE,BLACK);
  oled_display.setCursor(40, 0);
  oled_display.println("MEGA");
  oled_display.setCursor(22, 15);
  oled_display.println("COMMAND");
  for (float length = 0; length < 32; length += 0.7) {

    // display.fillRect(0, 0, 128, length, BLACK);
    for (uint8_t x = 0; x < 50 + (length * 5); x++) {

      oled_display.drawPixel(random(20, 110), 30 - random(0, (int)length),
                             BLACK);
    }

    //:    display.drawLine(0, length, 128, length, BLACK);

    oled_display.display();
  }
  oled_display.clearDisplay();
  oled_display.display();
  oled_display.setTextSize(1);
#else
  char str1[17] = "MEGACOMMAND LIVE";
  char str2[17] = "V2.x.x";
  str1[16] = '\0';
  LCD.goLine(0);
  LCD.puts(str1);
  LCD.goLine(1);
  LCD.puts(str2);

  delay(100);
#endif
  // while (rec_global == 0) {

  //  GUI.setPage(&grid_page);
}

MCLGfx gfx;
