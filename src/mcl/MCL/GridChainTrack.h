/* Justin Mammarella jmamma@gmail.com 2018 */

#pragma once

#include "AUXTrack.h"
#include "GridChain.h"

class ATTR_PACKED() GridChainTrack : public AUXTrack {
public:
  GridChain chains;

  size_t _sizeof() const {
    return sizeof(GridChainTrack) - sizeof(void*);
  }

  GridChainTrack() {
    active = GRIDCHAIN_TRACK_TYPE;
    static_assert(sizeof(GridChainTrack) <= GRIDCHAIN_TRACK_LEN);
  }

  void init() { chains.init(); }
  void init_defaults() override { init(); }

  virtual void get_online_data(uint8_t merge) override;
  bool store_in_grid(GridSlot column, GridRow row,
                     SeqTrack *seq_track = nullptr, uint8_t merge = 0,
                     bool online = false);

  void load_immediate(uint8_t tracknumber, SeqTrack *seq_track) override;
  void get_chains();
  void place_chains();

  virtual uint16_t get_track_size() override { return _sizeof(); }
  virtual uintptr_t get_region() override { return BANK1_GRIDCHAIN_TRACK_START; }

  uint16_t grid_slot_label(GridSlotLabelContext ctx) override {
    (void)ctx;
    return make_grid_slot_label('C', 'N');
  }
  virtual uint8_t get_model() override { return GRIDCHAIN_TRACK_TYPE; }
  virtual void *get_sound_data_ptr() override { return this; }
  virtual size_t get_sound_data_size() override { return sizeof(GridChain); }
};
