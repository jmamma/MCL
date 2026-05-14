/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef MCLCLIPBOARD_H__
#define MCLCLIPBOARD_H__
#include "Grid.h"
#include "Shared.h"
#include "MDSeqTrackData.h"
#if !defined(__AVR__)
#include "SPSXSeqTrackData.h"
#endif
#include "PerfData.h"
#define FILENAME_CLIPBOARD "clipboard.tmp"

class MCLClipBoard {
public:
  GridSlot t_col;
  GridRow t_row;
  GridSpan t_w;
  GridSpan t_h;

  GridSlot copy_track;
  bool copy_scene_active;

  Grid grids[NUM_GRIDS];

  MDSeqStep steps[16];
#if !defined(__AVR__)
  SPSXSeqStep spsx_steps[16];
#endif
  PerfScene scene;

  bool init();
  bool open();
  bool close();

  void copy_scene(PerfScene *s1);
  bool paste_scene(PerfScene *s1);

  bool copy_sequencer(GridSlot offset = 0);
  bool copy_sequencer_track(GridSlot track);
  bool paste_sequencer(GridSlot offset = 0);
  bool paste_sequencer_track(GridSlot source_track, GridSlot track);

  bool copy(GridSlot col, GridRow row, GridSpan w, GridSpan h);
  bool paste(GridSlot col, GridRow row);

};

extern MCLClipBoard mcl_clipboard;

#endif /* MCLCLIPBOARD_H__ */
