/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef MCLCLIPBOARD_H__
#define MCLCLIPBOARD_H__
#include "Grid.h"
#include "SdFat.h"
#include "Shared.h"
#include "MDSeqTrackData.h"
#include "PerfData.h"
#define FILENAME_CLIPBOARD "clipboard.tmp"

class MCLClipBoard {
public:
  int t_col;
  int t_row;
  int t_w;
  int t_h;

  uint8_t copy_track;
  bool copy_scene_active;

  Grid grids[NUM_GRIDS];

  MDSeqStep steps[16];
  PerfScene scene;

  bool init();
  bool open();
  bool close();

  void copy_scene(PerfScene *s1);
  bool paste_scene(PerfScene *s1);

  bool copy_sequencer(uint8_t offset = 0);
  bool copy_sequencer_track(uint8_t track);
  bool paste_sequencer(uint8_t offset = 0);
  bool paste_sequencer_track(uint8_t source_track, uint8_t track);

  bool copy(uint8_t col, uint16_t row, uint8_t w, uint16_t h);
  bool paste(uint8_t col, uint16_t row);

};

extern MCLClipBoard mcl_clipboard;

#endif /* MCLCLIPBOARD_H__ */
