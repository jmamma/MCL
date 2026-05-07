#include "oled.h"
#include "MCLGfx.h"
#include "ResourceManager.h"
#include "MCLGUI.h"

#define BITMAP_MCL_LOGO_W 58
#define BITMAP_MCL_LOGO_H 19

#if defined(OLED_WIDTH) && defined(OLED_HEIGHT) && (OLED_WIDTH == 128) &&        \
    (OLED_HEIGHT == 64)
#define MCL_SPLASH_128_64
#endif

void MCLGfx::draw_evil(unsigned char* evil) {
  auto oldfont = oled_display.getFont();
  oled_display.setFont(&TomThumb);
  for (int8_t x = -45; x < 128 - 45; x += 2) {
  oled_display.fillScreen(WHITE);
  oled_display.setCursor(x - 60, 10);
  oled_display.setTextColor(BLACK, WHITE);
  mcl_println_P(mclstr_this_piece_of);
  oled_display.setCursor(x - 60, 20);
  mcl_println_P(mclstr_machine_is_ok);

  oled_display.drawBitmap(x, 1, evil, 33, 31, BLACK);
  oled_display.display();
}
  delay(2000);
  oled_display.setTextColor(WHITE, BLACK);
  oled_display.setFont(oldfont);
}

void MCLGfx::splashscreen(unsigned char* bitmap) {
  oled_display.clearDisplay();
  oled_display.setFont();

#ifdef MCL_SPLASH_128_64
  constexpr uint8_t logo_y = (OLED_HEIGHT - BITMAP_MCL_LOGO_H) / 2;
#else
  constexpr uint8_t logo_y = 8;
#endif

  oled_display.drawBitmap(35, logo_y, bitmap, BITMAP_MCL_LOGO_W,
                          BITMAP_MCL_LOGO_H, WHITE);
  /* oled_display.println("MEGA");
   oled_display.setCursor(22, 15);
   oled_display.println("COMMAND");
   */
  oled_display.setCursor(90, logo_y);
  oled_display.setTextSize(1);
  mcl_print_P(mclstr_v_label);
  oled_display.print(VERSION_STR);
  oled_display.display();

  delay(750);

  for (uint8_t a = 0; a < 32; a++) {
#ifdef MCL_SPLASH_128_64
    constexpr uint8_t wipe_top = (OLED_HEIGHT - 32) / 2;
    constexpr uint8_t wipe_bottom = wipe_top + 31;
    oled_display.drawLine(35, wipe_top + a, BITMAP_MCL_LOGO_W + 35 + 33,
                          wipe_top + a, BLACK);
    oled_display.drawLine(35, wipe_bottom - a, BITMAP_MCL_LOGO_W + 35 + 33,
                          wipe_bottom - a, BLACK);
#else
    oled_display.drawLine(35, a, BITMAP_MCL_LOGO_W + 35 + 33, a, BLACK);
    oled_display.drawLine(35, 32 - a, BITMAP_MCL_LOGO_W + 35 + 33, 32 - a,
                          BLACK);
#endif
    oled_display.display();
#ifndef __AVR__
    delay(10);
#endif
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
