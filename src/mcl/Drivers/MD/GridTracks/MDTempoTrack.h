/* Justin Mammarella jmamma@gmail.com 2018 */

#pragma once

#include "AUXTrack.h"

class ATTR_PACKED() TempoData {
public:
  float tempo;
};

class ATTR_PACKED() MDTempoTrack : public AUXTrack, public TempoData {
public:
  MDTempoTrack() {
    active = MDTEMPO_TRACK_TYPE;
  }

  size_t _sizeof() const {
     return sizeof(MDTempoTrack) - sizeof(void*);
  }

  void init() {}

  void get_tempo();
  uint16_t calc_latency(uint8_t tracknumber);
  uint16_t send_tempo(bool send = true);
  void transition_send(uint8_t tracknumber, GridSlot slotnumber);
  virtual void get_online_data(uint8_t merge) override;

  void load_immediate(uint8_t tracknumber, SeqTrack *seq_track);
  void load_immediate_cleared(uint8_t tracknumber, SeqTrack *seq_track);

  virtual uint16_t get_track_size() { return _sizeof(); }
  virtual uintptr_t get_region() { return BANK1_MDTEMPO_TRACK_START; }

  virtual uint8_t get_model() { return MDTEMPO_TRACK_TYPE; }
  virtual uint8_t get_device_type() { return MDTEMPO_TRACK_TYPE; }

  virtual void *get_sound_data_ptr() { return &tempo; }
  virtual size_t get_sound_data_size() { return sizeof(float); }
};
