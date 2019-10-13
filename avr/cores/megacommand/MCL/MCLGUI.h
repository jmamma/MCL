/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MCLGUI_H__
#define MCLGUI_H__

#include "TextInputPage.h"

class MCLGUI {
public:
  bool wait_for_input(char *dst, char *title, uint8_t len);
  void draw_vertical_dashline(uint8_t x);
  void draw_vertical_separator(uint8_t x);
  void draw_vertical_scrollbar(uint8_t x, uint8_t n_items, uint8_t n_window, uint8_t n_current);
  void draw_progress(char* msg, uint8_t cur, uint8_t _max);
};

extern MCLGUI mcl_gui;

#endif /* MCLGUI_H__ */
