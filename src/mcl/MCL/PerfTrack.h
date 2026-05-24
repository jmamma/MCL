/* Justin Mammarella jmamma@gmail.com 2018 */

#pragma once

#include "AUXTrack.h"
#include "PerfEncoder.h"
#include "MixerPage.h"

#define PERF_TRACK_STORAGE_VERSION_CLEAN_LAYOUT 1

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
    uint8_t c = n * 2;
    active_scene_a = 0 + c;
    active_scene_b = 1 + c;
    const char *str = "CONTROL";
    strcpy(name, str);
  }

};

class ATTR_PACKED() PerfTrackData {
public:
  PerfTrackEncoderData encs[4];
  PerfScene scenes[NUM_SCENES];
  MuteSet mute_sets[2];
  uint8_t perf_locks[4][4];
  uint8_t load_mute_set;
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
    for (uint8_t n = 0; n < NUM_SCENES; n++) {
      if (n < 4) {
        encs[n].init(n);
      }
      scenes[n].init();
    }
    //memset(mute_sets, 0xFF, sizeof(mute_sets));
    //memset(perf_locks, 255, sizeof(perf_locks));
    memset(mute_sets, 0xFF, sizeof(mute_sets) + sizeof(perf_locks));
    load_mute_set = 255;
    load_type_mask = 0xFF;
  }
  void init_defaults() override { init(); }

  void load_perf(bool immediate, SeqTrack *seq_track);
  void get_perf();
  void convert_legacy_load_settings();

  uint16_t calc_latency(uint8_t tracknumber) override;

  void transition_load(uint8_t tracknumber, SeqTrack *seq_track, GridSlot slotnumber) override;
  virtual void get_online_data(uint8_t merge) override;

  void load_immediate(uint8_t tracknumber, SeqTrack *seq_track) override;

  virtual uint16_t get_track_size() override { return _sizeof(); }
  virtual uintptr_t get_region() override { return BANK1_PERF_TRACK_START; }

  bool copy_grid_slot_label(uint8_t model, GridColumn column, GridSlot slot,
                            GridRow row, char label[3]) override {
    (void)model;
    (void)column;
    (void)slot;
    (void)row;
    label[0] = 'P';
    label[1] = 'F';
    label[2] = '\0';
    return true;
  }
  virtual uint8_t get_model() override { return PERF_TRACK_TYPE; }
  virtual uint8_t storage_version() const override {
    return PERF_TRACK_STORAGE_VERSION_CLEAN_LAYOUT;
  }
  virtual void *get_sound_data_ptr() override { return &encs; }
  virtual size_t get_sound_data_size() override { return sizeof(PerfTrackData); }
};

static_assert(MEMORY_ALIGN(sizeof(PerfTrack) - sizeof(void *)) <= PERF_TRACK_LEN,
              "PerfTrack exceeds storage");
