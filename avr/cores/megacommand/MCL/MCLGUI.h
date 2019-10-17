/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MCLGUI_H__
#define MCLGUI_H__

#include "TextInputPage.h"

class MCLGUI {
public:
  bool wait_for_input(char *dst, const char *title, uint8_t len);
  void draw_infobox(const char* line1, const char* line2);
  void draw_vertical_dashline(uint8_t x);
  void draw_vertical_separator(uint8_t x);
  void draw_vertical_scrollbar(uint8_t x, uint8_t n_items, uint8_t n_window, uint8_t n_current);
  ///  Clear the content area of a popup
  void clear_popup();
  void draw_popup(const char* title, bool deferred_display = false);
  void draw_progress(const char* msg, uint8_t cur, uint8_t _max, bool deferred_display = false);

  void clear_leftpane();
  void clear_rightpane();

  static constexpr uint8_t s_menu_w = 96;
  static constexpr uint8_t s_menu_h = 24;
  static constexpr uint8_t s_menu_x = (128 - s_menu_w) / 2;
  static constexpr uint8_t s_menu_y = (32 - s_menu_h) / 2;
  static constexpr uint8_t s_title_x = 31;
  static constexpr uint8_t s_title_w = 64;

  static constexpr uint8_t s_rightpane_offset_x = 43;
  static constexpr uint8_t s_rightpane_offset_y = 8;

};

extern MCLGUI mcl_gui;

#endif /* MCLGUI_H__ */
