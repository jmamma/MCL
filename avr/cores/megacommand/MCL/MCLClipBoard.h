/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef MCLCLIPBOARD_H__
#define MCLCLIPBOARD_H__
#include "Grid.h"
#include "SdFat.h"
#include "Shared.h"
#include "MDSeqTrackData.h"
#define FILENAME_CLIPBOARD "clipboard.tmp"

class MCLClipBoard {
public:
  uint8_t t_col;
  uint8_t t_row;
  uint8_t t_w;
  uint8_t t_h;

  uint8_t copy_track;

  File file;

  MDSeqStep step;

  bool init();
  bool open();
  bool close();

  bool copy_sequencer(uint8_t offset = 0);
  bool copy_sequencer_track(uint8_t track);
  bool paste_sequencer(uint8_t offset = 0);
  bool paste_sequencer_track(uint8_t source_track, uint8_t track);

  bool copy(uint8_t col, uint8_t row, uint8_t w, uint8_t h);
  bool paste(uint8_t col, uint8_t row);

};

extern MCLClipBoard mcl_clipboard;

#endif /* MCLCLIPBOARD_H__ */
