/* Justin Mammarella jmamma@gmail.com 2018 */

#pragma once

#include "DeviceTrack.h"

class ATTR_PACKED() AUXTrack : public DeviceTrack {
public:
  virtual void transition_load(uint8_t tracknumber, SeqTrack* seq_track, uint8_t slotnumber) {
    load_link_data(seq_track);
    GridTrack::transition_load(tracknumber, seq_track, slotnumber);
  }

  // Subclass captures current device state when saving online.
  virtual void get_online_data(uint8_t merge) = 0;

  virtual bool store_in_grid(uint8_t column, uint16_t row,
                              SeqTrack *seq_track = nullptr, uint8_t merge = 0,
                              bool online = false, Grid *grid = nullptr) override;
};
