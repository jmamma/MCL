/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef MCLCLIPBOARD_H__
#define MCLCLIPBOARD_H__
#include "Grid.h"
#include "Shared.h"
#include "MDSeqTrackData.h"
#include "SeqExtStepTypes.h"
#if !defined(__AVR__)
#include "SPSXSeqTrackData.h"
#endif
#include "PerfData.h"
#define FILENAME_CLIPBOARD "clipboard.tmp"

enum ExtNoteClipMode : uint8_t {
  EXT_NOTE_CLIP_NONE = 0,
  EXT_NOTE_CLIP_RECTANGLE = 1,
  EXT_NOTE_CLIP_PAGE = 2,
};

struct ATTR_PACKED() ExtNoteClipEvent {
  seq_extstep_tick_t tick_offset;
  seq_extstep_tick_t note_length;
  uint8_t pitch_offset;
  uint8_t velocity;
  uint8_t condition;
};

class ExtNoteClip {
public:
  uint8_t mode;
  uint8_t count;
  uint16_t ticks_per_step;
  ExtNoteClipEvent notes[EXT_NOTE_CLIP_MAX_NOTES];

  void clear(uint8_t mode_ = EXT_NOTE_CLIP_NONE) {
    mode = mode_;
    count = 0;
    ticks_per_step = 0;
  }

  bool add(const ExtNoteClipEvent &note) {
    if (count >= EXT_NOTE_CLIP_MAX_NOTES) {
      return false;
    }
    notes[count++] = note;
    return true;
  }

  bool valid() const { return mode != EXT_NOTE_CLIP_NONE && count > 0; }
};

#if defined(__AVR__)
static_assert(sizeof(ExtNoteClip) <= 1024,
              "AVR ext note clipboard must stay under 1KB");
#endif

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
  ExtNoteClip ext_note_clip;
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
