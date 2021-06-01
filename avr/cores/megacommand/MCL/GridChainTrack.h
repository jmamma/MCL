/* Justin Mammarella jmamma@gmail.com 2018 */

#pragma once

#include "AUXTrack.h"

class GridChainTrack : public AUXTrack {
public:
  GridChain chains;

  GridChainTrack() {
    active = GRIDCHAIN_TRACK_TYPE;
    static_assert(sizeof(GridChainTrack) <= GRIDCHAIN_TRACK_LEN);
  }

  void init() {}

  bool store_in_grid(uint8_t column, uint16_t row,
                     SeqTrack *seq_track = nullptr, uint8_t merge = 0,
                     bool online = false);

  void load_immediate(uint8_t tracknumber, SeqTrack *seq_track);
  void get_chains();
  void place_chains();

  virtual uint16_t get_track_size() { return sizeof(GridChainTrack); }
  virtual uint32_t get_region() { return BANK1_GRIDCHAIN_TRACK_START; }

  virtual uint8_t get_model() { return GRIDCHAIN_TRACK_TYPE; }
  virtual uint8_t get_device_type() { return GRIDCHAIN_TRACK_TYPE; }

  virtual void *get_sound_data_ptr() { return this; }
  virtual size_t get_sound_data_size() { return sizeof(GridChain); }
};
