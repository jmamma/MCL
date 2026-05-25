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
  uint16_t calc_latency(uint8_t tracknumber) override;
  uint16_t send_tempo(bool send = true);
  void transition_send(uint8_t tracknumber, GridSlot slotnumber) override;
  virtual void get_online_data(uint8_t merge) override;

  void load_immediate(uint8_t tracknumber, SeqTrack *seq_track) override;
  void load_immediate_cleared(uint8_t tracknumber, SeqTrack *seq_track) override;

  virtual uint16_t get_track_size() override { return _sizeof(); }
  virtual uintptr_t get_region() override { return BANK1_MDTEMPO_TRACK_START; }

  uint16_t grid_slot_label(const GridSlotLabelContext &ctx) override {
    (void)ctx;
    return make_grid_slot_label('T', 'P');
  }
  virtual uint8_t get_model() override { return MDTEMPO_TRACK_TYPE; }
  virtual void *get_sound_data_ptr() override { return &tempo; }
  virtual size_t get_sound_data_size() override { return sizeof(float); }
};
