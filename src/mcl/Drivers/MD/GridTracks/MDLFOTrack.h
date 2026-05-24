/* Justin Mammarella jmamma@gmail.com 2018 */

#pragma once

#include "AUXTrack.h"
#include "LFOSeqTrack.h"

class ATTR_PACKED() MDLFOTrack : public AUXTrack {
public:
  LegacyLFOSeqTrackData lfo_data;
  MDLFOTrack() {
    active = MDLFO_TRACK_TYPE;
  }
  size_t _sizeof() const {
     return sizeof(MDLFOTrack) - sizeof(void*);
  }
  void init() {};

  virtual void init(uint8_t tracknumber, SeqTrack *seq_track) override {
     lfo_data.init();
  }
  void init_defaults() override { lfo_data.init(); }

  void get_lfos();
  uint16_t calc_latency(uint8_t tracknumber) override;
  void transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                       GridSlot slotnumber) override;
  virtual void get_online_data(uint8_t merge) override;

  void load_immediate(uint8_t tracknumber, SeqTrack *seq_track) override;

  virtual uint16_t get_track_size() override { return _sizeof(); }
  virtual uintptr_t get_region() override { return BANK1_MDLFO_TRACK_START; }

  bool copy_grid_slot_label(uint8_t model, GridColumn column, GridSlot slot,
                            GridRow row, char label[3]) override {
    (void)model;
    (void)column;
    (void)slot;
    (void)row;
    label[0] = 'L';
    label[1] = 'F';
    label[2] = '\0';
    return true;
  }
  virtual uint8_t get_model() override { return MDLFO_TRACK_TYPE; }
  virtual void *get_sound_data_ptr() override { return nullptr; }
  virtual size_t get_sound_data_size() override { return 0; }
};
