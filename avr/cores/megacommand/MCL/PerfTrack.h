/* Justin Mammarella jmamma@gmail.com 2018 */

#pragma once

#include "AUXTrack.h"

class PerfTrackEncoderData {
public:
  uint8_t src;
  uint8_t param;
  uint8_t min;

  uint8_t active_scene_a;
  uint8_t active_scene_b;

  uint8_t cur;
};

class PerfTrackData {
public:
  PerfTrackEncoderData encs[4];
  PerfScene scenes[NUM_SCENES];
};

class PerfTrack : public AUXTrack, public PerfTrackData {
public:
  PerfTrack() {
    active = PERF_TRACK_TYPE;
    static_assert(sizeof(PerfTrack) <= PERF_TRACK_LEN);
  }

  void init() {}

  void load_perf();
  void get_perf();

  uint16_t calc_latency(uint8_t tracknumber);

  void transition_send(uint8_t tracknumber, uint8_t slotnumber);
  bool store_in_grid(uint8_t column, uint16_t row,
                     SeqTrack *seq_track = nullptr, uint8_t merge = 0,
                     bool online = false, Grid *grid = nullptr);

  void load_immediate(uint8_t tracknumber, SeqTrack *seq_track);

  virtual uint16_t get_track_size() { return sizeof(PerfTrack); }
  virtual uint16_t get_region() { return BANK1_PERF_TRACK_START; }

  virtual uint8_t get_model() { return PERF_TRACK_TYPE; }
  virtual uint8_t get_device_type() { return PERF_TRACK_TYPE; }

  virtual void *get_sound_data_ptr() { return &encs; }
  virtual size_t get_sound_data_size() { return sizeof(PerfTrackData); }
};
