/* Justin Mammarella jmamma@gmail.com 2018 */

#pragma once

#include "AUXTrack.h"
#include "PerfEncoder.h"
#include "MixerPage.h"

#define PERF_TRACK_STORAGE_VERSION_PERF_STATES 3

class ATTR_PACKED() PerfTrackEncoderData {
public:
  char name[PERF_NAME_LENGTH];
  uint8_t src;
  uint8_t param;
  uint8_t min;

  uint8_t active_scene_a;
  uint8_t active_scene_b;

  uint8_t cur;
  void init(uint8_t n) {
    src = param = min = 0;
    active_scene_a = n << 1;
    active_scene_b = active_scene_a + 1;
    strcpy(name, "CONTROL");
  }

};

class ATTR_PACKED() PerfTrackData {
public:
  PerfTrackEncoderData encs[4];
  PerfScene scenes[NUM_SCENES];
  PerfState perf_states[4];
  uint8_t perf_locks[4][4];
  uint8_t load_perf_state;
  uint8_t load_type_mask;
};

class ATTR_PACKED() PerfTrack : public AUXTrack, public PerfTrackData {
public:
  size_t _sizeof() const {
     return sizeof(PerfTrack) - sizeof(void*);
  }

  PerfTrack() {
    active = PERF_TRACK_TYPE;
  }

  virtual void init(uint8_t tracknumber, SeqTrack *seq_track) override {
    init();
  }

  void init() {
    for (uint8_t n = 0; n < 4; n++) {
      encs[n].init(n);
    }
    for (uint8_t n = 0; n < NUM_SCENES; n++) {
      scenes[n].init();
    }
    for (uint8_t n = 0; n < 4; n++) {
      perf_states[n].init();
    }
    memset(perf_locks, 0xFF, sizeof(perf_locks));
    load_perf_state = 255;
    load_type_mask = 0xFF;
  }
  void init_defaults() override { init(); }

  void load_perf(bool immediate, SeqTrack *seq_track);
  void get_perf();

  uint16_t calc_latency(uint8_t tracknumber) override;

  void transition_load(uint8_t tracknumber, SeqTrack *seq_track, GridSlot slotnumber) override;
  virtual void get_online_data(uint8_t merge) override;

  void load_immediate(uint8_t tracknumber, SeqTrack *seq_track) override;

  virtual uint16_t get_track_size() override { return _sizeof(); }
  virtual uintptr_t get_region() override { return BANK1_PERF_TRACK_START; }

  uint16_t grid_slot_label(GridSlotLabelContext ctx) override {
    (void)ctx;
    return make_grid_slot_label('P', 'F');
  }
  virtual uint8_t get_model() override { return PERF_TRACK_TYPE; }
  virtual uint8_t storage_version() const override {
    return PERF_TRACK_STORAGE_VERSION_PERF_STATES;
  }
  virtual void *get_sound_data_ptr() override { return &encs; }
  virtual size_t get_sound_data_size() override { return sizeof(PerfTrackData); }
};

static_assert(MEMORY_ALIGN(sizeof(PerfTrack) - sizeof(void *)) <= PERF_TRACK_LEN,
              "PerfTrack exceeds storage");
