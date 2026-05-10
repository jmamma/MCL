#pragma once

#include "platform.h"
#include <inttypes.h>

#define SEQ_TRACK_MOD_STORAGE_VERSION 1

class ATTR_PACKED() SeqLFOData {
public:
  uint8_t flags;
  uint8_t dest_track;
  uint8_t dest_param;
  uint8_t shape_pack;
  uint8_t type;
  uint16_t speed;
  uint8_t depth;
  uint8_t mix;

  SeqLFOData() { init(); }

  void init() {
    flags = 0;
    dest_track = 0;
    dest_param = 0;
    shape_pack = 0;
    type = 0;
    speed = 0;
    depth = 0;
    mix = 0;
  }
};

class ATTR_PACKED() ArpSeqData {
public:
  uint8_t enabled : 4;
  uint8_t range : 4;
  uint8_t oct;
  uint8_t mode;
  uint8_t fine_tune;
  uint8_t rate;
  uint64_t note_mask[2];

  ArpSeqData() { init(); }

  void init() {
    enabled = 0;
    range = 0;
    oct = 1;
    mode = 0;
    fine_tune = 0;
    rate = 2;
    note_mask[0] = 0;
    note_mask[1] = 0;
  }
};

class ATTR_PACKED() SeqTrackModData {
public:
  SeqLFOData lfo;
  ArpSeqData arp;

  SeqTrackModData() { init(); }

  void init() {
    lfo.init();
    arp.init();
  }
};

static_assert(sizeof(SeqLFOData) == 9, "SeqLFOData storage size changed");
static_assert(sizeof(ArpSeqData) == 21, "ArpSeqData storage size changed");
static_assert(sizeof(SeqTrackModData) == 30,
              "SeqTrackModData storage size changed");
