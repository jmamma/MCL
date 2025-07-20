/* Justin Mammarella jmamma@gmail.com 2018 */
#pragma once

#include "AUXTrack.h"
#include "PerfEncoder.h"
#include "MixerPage.h"

class PerfTrackEncoderData {
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

class PerfTrackData {
public:
  PerfTrackEncoderData encs[4];
  PerfScene scenes[NUM_SCENES];
  //Don't change order
  MuteSet mute_sets[2];
  uint8_t perf_locks[4][4];
  //
};

class PerfTrack : public AUXTrack, public PerfTrackData {
public:

  PerfTrack() {
    active = PERF_TRACK_TYPE;
    static_assert(sizeof(PerfTrack) <= PERF_TRACK_LEN);
  }

  virtual void init(uint8_t tracknumber, SeqTrack *seq_track) {
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
  }

  void load_perf(bool immediate, SeqTrack *seq_track);
  void get_perf();

  uint16_t calc_latency(uint8_t tracknumber);

  void transition_send(uint8_t tracknumber, uint8_t slotnumber);
  void transition_load(uint8_t tracknumber, SeqTrack *seq_track, uint8_t slotnumber);
  bool store_in_grid(uint8_t column, uint16_t row,
                     SeqTrack *seq_track = nullptr, uint8_t merge = 0,
                     bool online = false, Grid *grid = nullptr);

  void load_immediate(uint8_t tracknumber, SeqTrack *seq_track);

  virtual uint16_t get_track_size() { return sizeof(PerfTrack); }
  virtual uint8_t *get_region() { return BANK1_PERF_TRACK_START; }

  virtual uint8_t get_model() { return PERF_TRACK_TYPE; }
  virtual uint8_t get_device_type() { return PERF_TRACK_TYPE; }

  virtual void *get_sound_data_ptr() { return &encs; }
  virtual size_t get_sound_data_size() { return sizeof(PerfTrackData); }
};
