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
  mcl_gui.draw_vertical_separator(x + 1);
  oled_display.fillRect(x + 1, y + 1, 3, length - 2, BLACK);
  oled_display.drawRect(x, y, 5, length, WHITE);
}

void MCLGUI::draw_popup(char *title, bool deferred_display) {
  oled_display.setFont(&TomThumb);

  // draw menu body
  oled_display.fillRect(s_menu_x - 1, s_menu_y - 1, s_menu_w + 2, s_menu_h + 2,
                        BLACK);
  oled_display.drawRect(s_menu_x, s_menu_y, s_menu_w, s_menu_h, WHITE);
  oled_display.fillRect(s_menu_x + 1, s_menu_y + 1, s_menu_w - 2, 3, WHITE);

  // draw the title '____/**********\____' part
  oled_display.drawRect(s_title_x, s_menu_y - 3, s_title_w, 3, BLACK);
  oled_display.drawRect(s_title_x, s_menu_y - 2, s_title_w, 2, WHITE);
  oled_display.drawPixel(s_title_x, s_menu_y - 2, BLACK);
  oled_display.drawPixel(s_title_x + s_title_w - 1, s_menu_y - 2, BLACK);

  oled_display.setTextColor(BLACK);
  // auto len = strlen(msg) * 3;
  // oled_display.setCursor(64 - (len / 2), s_menu_y);
  oled_display.setCursor(s_title_x + 2, s_menu_y + 3);
  oled_display.println(title);
  oled_display.setTextColor(WHITE);
  if (!deferred_display) {
    oled_display.display();
  }
}

void MCLGUI::clear_popup() {
  oled_display.fillRect(s_menu_x + 1, s_menu_y + 4, s_menu_w - 2, s_menu_h - 5,
                        BLACK);
}

static constexpr uint8_t s_progress_x = 31;
static constexpr uint8_t s_progress_y = 16;
static constexpr uint8_t s_progress_w = 64;
static constexpr uint8_t s_progress_h = 5;

static uint8_t s_progress_cookie = 0;

void MCLGUI::draw_progress(char *msg, uint8_t cur, uint8_t _max,
                           bool deferred_display) {
  draw_popup(msg, true);

  oled_display.fillRect(s_progress_x + 1, s_progress_y + 1, s_progress_w - 2,
                        s_progress_h - 2, BLACK);

  float prog = (float)cur / (float)_max;
  auto progx = (uint8_t)(s_progress_x + 1 + prog * (s_progress_w - 2));
  // draw the progress
  oled_display.fillRect(s_progress_x + 1, s_progress_y + 1,
                        progx - s_progress_x + 1, s_progress_h - 2, WHITE);
  s_progress_cookie = (s_progress_cookie + 1) % 3;

  // draw the '///////' pattern, note the cookie
  for (uint8_t i = s_progress_cookie + s_progress_x + 1; i < progx; i += 3) {
    oled_display.drawLine(i, s_progress_y + s_progress_h - 2, i + 2,
                          s_progress_y + 1, BLACK);
  }

  oled_display.drawRect(s_progress_x, s_progress_y, s_progress_w, s_progress_h,
                        WHITE);
  if (!deferred_display) {
    oled_display.display();
  }
}

