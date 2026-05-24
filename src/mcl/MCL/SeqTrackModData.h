#pragma once

#include "platform.h"
#include <inttypes.h>
#include <stddef.h>

#define SEQ_TRACK_MOD_STORAGE_VERSION 1
#define SEQ_TRACK_SWING_STORAGE_VERSION 2
#define SEQ_TRACK_MICROTIMING_STORAGE_VERSION 3

class ATTR_PACKED() SeqLFODataV1 {
public:
  uint8_t flags;
  uint8_t dest_track;
  uint8_t dest_param;
  uint8_t shape_pack;
  uint8_t type;
  uint16_t speed;
  uint8_t depth;
  uint8_t mix;
};

class ATTR_PACKED() SeqLFOParamData {
public:
  uint8_t dest;
  uint8_t param;
  uint8_t depth;
  uint8_t offset;

  void init() {
    dest = 0;
    param = 0;
    depth = 0;
    offset = 0;
  }
};

class ATTR_PACKED() SeqLFOData {
public:
  SeqLFOParamData params[2];
  uint8_t wav_type;
  uint8_t speed;
  uint8_t mode;
  uint64_t pattern_mask;
  uint8_t enable;
  uint8_t length;

  void init() {
    for (uint8_t i = 0; i < 2; ++i) {
      params[i].init();
    }
    wav_type = 0;
    speed = 0;
    mode = 0;
    pattern_mask = 0;
    enable = 0;
    length = 16;
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
  uint8_t reserved[sizeof(SeqLFODataV1)];
  ArpSeqData arp;
  SeqLFOData lfo;

  void init() {
    for (uint8_t i = 0; i < sizeof(reserved); ++i) {
      reserved[i] = 0;
    }
    lfo.init();
    arp.init();
  }
};

class ATTR_PACKED() SeqTrackModStorage : public SeqTrackModData {
public:
  SeqTrackModData &mod() { return *this; }
  const SeqTrackModData &mod() const { return *this; }
  void init_mod() { SeqTrackModData::init(); }
};

static_assert(sizeof(SeqLFODataV1) == 9, "SeqLFODataV1 storage size changed");
static_assert(sizeof(SeqLFOParamData) == 4,
              "SeqLFOParamData storage size changed");
static_assert(sizeof(SeqLFOData) == 21, "SeqLFOData storage size changed");
static_assert(sizeof(ArpSeqData) == 21, "ArpSeqData storage size changed");
static_assert(offsetof(SeqTrackModData, arp) == sizeof(SeqLFODataV1),
              "SeqTrackModData arp offset changed");
static_assert(offsetof(SeqTrackModData, lfo) ==
                  sizeof(SeqLFODataV1) + sizeof(ArpSeqData),
              "SeqTrackModData lfo offset changed");
static_assert(sizeof(SeqTrackModData) == 51,
              "SeqTrackModData storage size changed");
static_assert(sizeof(SeqTrackModStorage) == sizeof(SeqTrackModData),
              "SeqTrackModStorage storage size changed");
