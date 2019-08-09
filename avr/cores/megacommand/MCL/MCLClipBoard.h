/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef MCLCLIPBOARD_H__
#define MCLCLIPBOARD_H__
#include "Grid.h"
#include "SdFat.h"
#include "Shared.h"

#define FILENAME_CLIPBOARD "clipboard.tmp"

class MCLClipBoard {
public:
  int t_col;
  int t_row;
  int t_w;
  int t_h;
  File file;

  bool init();
  bool open();
  bool close();

  bool copy(uint16_t col, uint16_t row, uint16_t w, uint16_t h);
  bool paste(uint16_t col, uint16_t row);

};

extern MCLClipBoard mcl_clipboard;

#endif /* MCLCLIPBOARD_H__ */
