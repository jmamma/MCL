#include "MCL_impl.h"

void MCLGfx::init_oled() {
#ifdef OLED_DISPLAY
  oled_display.begin();

  oled_display.clearDisplay();
  oled_display.invertDisplay(0);
  oled_display.setRotation(2);
  oled_display.setTextSize(1);
  oled_display.setTextColor(WHITE, BLACK);
  oled_display.setCursor(0, 0);
  oled_display.display();
#endif
}

#ifdef OLED_DISPLAY
#define BITMAP_MCL_LOGO_W 58
#define BITMAP_MCL_LOGO_H 19
#endif
void MCLGfx::draw_evil(unsigned char* evil) {
#ifdef OLED_DISPLAY
  auto oldfont = oled_display.getFont();
  oled_display.setFont(&TomThumb);
  for (int8_t x = -45; x < 128 - 45; x += 2) {
  oled_display.fillScreen(WHITE);
  oled_display.setCursor(x - 60, 10);
  oled_display.setTextColor(BLACK, WHITE);
  oled_display.println("THIS PIECE OF");
  oled_display.setCursor(x - 60, 20);
  oled_display.println("MACHINE IS O.K.");

  oled_display.drawBitmap(x, 0, evil, 45, 32, BLACK);
  oled_display.display();
}
  delay(1800);
  oled_display.setTextColor(WHITE, BLACK);
  oled_display.setFont(oldfont);
#endif
}

void MCLGfx::splashscreen(unsigned char* bitmap) {
#ifdef OLED_DISPLAY
  DEBUG_PRINTLN(F("OLED enabled"));
  oled_display.setTextSize(2);
  oled_display.setTextColor(WHITE, BLACK);
  oled_display.setCursor(40, 0);

  oled_display.drawBitmap(35, 8, bitmap, BITMAP_MCL_LOGO_W,
                          BITMAP_MCL_LOGO_H, WHITE);
  /* oled_display.println("MEGA");
   oled_display.setCursor(22, 15);
   oled_display.println("COMMAND");
   */
  oled_display.setCursor(90, 8);
  oled_display.setTextSize(1);
  oled_display.print("V");
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

#else

  DEBUG_PRINTLN(F("HD44780 enabled"));
  char str1[17] = "MEGACOMMAND LIVE";
  char str2[17] = VERSION_STR;
  str1[16] = '\0';
  display_text(&str1[0], &str2[0]);
  delay(200);
#endif
  // while (rec_global == 0) {

  //  GUI.setPage(&grid_page);
}
void MCLGfx::display_text(const char *str1, const char *str2) {
  LCD.goLine(0);
  LCD.puts(str1);
  LCD.goLine(1);
  LCD.puts(str2);
}
void MCLGfx::alert(const char *str1, const char *str2) {
#ifdef OLED_DISPLAY
  mcl_gui.draw_infobox(str1, str2);
  oled_display.display();
  delay(700);
  oled_display.clearDisplay();
#else
  GUI.flash_strings_fill(str1, str2);
  GUI.display_lcd();
#endif
  DEBUG_PRINTLN(str1);
  DEBUG_PRINTLN(str2);
}

MCLGfx gfx;
