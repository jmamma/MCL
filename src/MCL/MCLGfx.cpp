#include "oled.h"
#include "MCLGfx.h"
#include "ResourceManager.h"
#include "MCLGUI.h"

void MCLGfx::init_oled() {
  oled_display.begin();

  oled_display.clearDisplay();
  oled_display.invertDisplay(0);
  oled_display.setRotation(2);
  oled_display.setTextSize(1);
  oled_display.setTextColor(WHITE, BLACK);
  oled_display.setCursor(0, 0);
  oled_display.setTextWrap(false);
  oled_display.display();
}

#define BITMAP_MCL_LOGO_W 58
#define BITMAP_MCL_LOGO_H 19

void MCLGfx::draw_evil(unsigned char* evil) {
  auto oldfont = oled_display.getFont();
  oled_display.setFont(&TomThumb);
  for (int8_t x = -45; x < 128 - 45; x += 2) {
  oled_display.fillScreen(WHITE);
  oled_display.setCursor(x - 60, 10);
  oled_display.setTextColor(BLACK, WHITE);
  oled_display.println("THIS PIECE OF");
  oled_display.setCursor(x - 60, 20);
  oled_display.println("MACHINE IS O.K.");

  oled_display.drawBitmap(x, 1, evil, 33, 31, BLACK);
  oled_display.display();
}
  delay(2000);
  oled_display.setTextColor(WHITE, BLACK);
  oled_display.setFont(oldfont);
}

void MCLGfx::splashscreen(unsigned char* bitmap) {
  oled_display.setFont();

  oled_display.drawBitmap(35, 8, bitmap, BITMAP_MCL_LOGO_W,
                          BITMAP_MCL_LOGO_H, WHITE);
  /* oled_display.println("MEGA");
   oled_display.setCursor(22, 15);
   oled_display.println("COMMAND");
   */
  oled_display.setCursor(90, 8);
  oled_display.setTextSize(1);
  oled_display.print(F("V"));
  oled_display.print(VERSION_STR);
  /*  for (float length = 0; length < 32; length += 0.7) {

      // display.fillRect(0, 0, 128, length, BLACK);
      for (uint8_t x = 0; x < 50 + (length * 5); x++) {

        oled_display.drawPixel(random(20, 110), 30 - random(0, (int)length),
                               BLACK);
      }

   √   //:    display.drawLine(0, length, 128, length, BLACK);

      oled_display.display();
    }  oled_display.clearDisplay();
    */
  oled_display.display();
  delay(750);

  for (uint8_t a = 0; a < 32; a++) {
    oled_display.drawLine(35, a, BITMAP_MCL_LOGO_W + 35 + 31, a, BLACK);
    oled_display.drawLine(35, 32 - a, BITMAP_MCL_LOGO_W + 35 + 31, 32 - a,
                          BLACK);
    oled_display.display();
  }

}

void MCLGfx::alert(const char *str1, const char *str2) {
  mcl_gui.draw_infobox(str1, str2);
  oled_display.display();
  delay(700);
  oled_display.clearDisplay();
  DEBUG_PRINTLN(str1);
  DEBUG_PRINTLN(str2);
}

MCLGfx gfx;
