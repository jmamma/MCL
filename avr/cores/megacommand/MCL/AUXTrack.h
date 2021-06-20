/* Justin Mammarella jmamma@gmail.com 2018 */

#pragma once

#include "DeviceTrack.h"

class AUXTrack : public DeviceTrack {
public:
  virtual void transition_load(uint8_t tracknumber, SeqTrack* seq_track, uint8_t slotnumber) {
  load_link_data(seq_track);
  GridTrack::transition_load(tracknumber, seq_track, slotnumber);
  }

};
