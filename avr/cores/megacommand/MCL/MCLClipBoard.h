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
  int t_col;
  int t_row;
  int t_w;
  int t_h;

  uint8_t copy_track;

  Grid grids[NUM_GRIDS];

  MDSeqStep steps[16];

  bool init();
  bool open();
  bool close();

  bool copy_sequencer(uint8_t offset = 0);
  bool copy_sequencer_track(uint8_t track);
  bool paste_sequencer(uint8_t offset = 0);
  bool paste_sequencer_track(uint8_t source_track, uint8_t track);

  bool copy(uint16_t col, uint16_t row, uint16_t w, uint16_t h, uint8_t grid);
  bool paste(uint16_t col, uint16_t row, uint8_t grid);

};

extern MCLClipBoard mcl_clipboard;

#endif /* MCLCLIPBOARD_H__ */
