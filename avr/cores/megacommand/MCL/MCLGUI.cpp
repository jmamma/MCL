#include "MCL.h"

bool MCLGUI::wait_for_input(char *dst, char *title, uint8_t len) {
  text_input_page.init();
  text_input_page.init_text(dst, title, len);
  GUI.pushPage(&text_input_page);
  while (GUI.currentPage() == &text_input_page) {
    GUI.loop();
  }
  return text_input_page.return_state;
}

void MCLGUI::draw_vertical_dashline(uint8_t x) {
  for (uint8_t y = 1; y < 32; y += 2) {
    oled_display.drawPixel(x, y, WHITE);
  }
}

void MCLGUI::draw_vertical_separator(uint8_t x) {
  auto x_ = x + 2;
  for (uint8_t y = 0; y < 32; y += 2) {
    oled_display.drawPixel(x, y, WHITE);
    oled_display.drawPixel(x_, y, WHITE);
  }
  x_ = x + 1;
  for (uint8_t y = 1; y < 32; y += 2) {
    oled_display.drawPixel(x_, y, WHITE);
  }
}

void MCLGUI::draw_vertical_scrollbar(uint8_t x, uint8_t n_items,
                                     uint8_t n_window, uint8_t n_current) {
  uint8_t length = round(((float)(n_window - 1) / (float)(n_items - 1)) * 32);
  uint8_t y = round(((float)(n_current) / (float)(n_items - 1)) * 32);
  mcl_gui.draw_vertical_separator(x);
  oled_display.fillRect(x + 1, y + 1, 3, length - 2, BLACK);
  oled_display.drawRect(x, y, 5, length, WHITE);
}

