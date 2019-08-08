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

  bool copy(int col, int row, int w, int h);
  bool paste(int col, int row);

};

extern MCLClipBoard mcl_clipboard;

#endif /* MCLCLIPBOARD_H__ */
