#pragma once

#include "Grid.h"

#define EMPTY_TRACK_TYPE 0

enum GridGroup {
  GROUP_DEV,
  GROUP_PERF,
  GROUP_AUX,
  GROUP_TEMPO,
};

class SeqTrack;

class GridDeviceTrack {
public:
  uint8_t device_idx;
  uint8_t track_type;
  GridGroup group_type;
  uint8_t mem_slot_idx;
  SeqTrack *seq_track;

  GridDeviceTrack() {
    init();
  }

  void init(uint8_t _track_type = EMPTY_TRACK_TYPE, GridGroup _group_type = GROUP_DEV, uint8_t _device_idx = 255, SeqTrack *_seq_track = nullptr, uint8_t _mem_slot_idx = 255) {
    track_type = _track_type;
    group_type = _group_type;
    mem_slot_idx = _mem_slot_idx;
    seq_track = _seq_track;
    device_idx = _device_idx;
  }

  SeqTrack *get_seq_track() { return seq_track; }
  bool isActive() { return track_type != EMPTY_TRACK_TYPE; }
};


class MidiDeviceGrid : public Grid {

  public:
  GridDeviceTrack tracks[GRID_WIDTH];

  void add_track(uint8_t track_idx, GridDeviceTrack *gdt) {
    if (gdt->mem_slot_idx == 255) { gdt->mem_slot_idx = track_idx; }
    DEBUG_PRINTLN("adding device track");
    DEBUG_PRINTLN(track_idx);
    DEBUG_PRINTLN(gdt->track_type);
    DEBUG_PRINTLN(gdt->mem_slot_idx);
    memcpy(tracks + track_idx, gdt, sizeof(GridDeviceTrack));
  }
  void cleanup(uint8_t device_idx) {
    for (uint8_t n = 0; n < GRID_WIDTH; n++) {
      if (tracks[n].device_idx == device_idx) { tracks[n].init(); }
    }
  }

  void init() {
    for (uint8_t n = 0; n < GRID_WIDTH; n++) {
      tracks[n].init();
    }
  }
};
