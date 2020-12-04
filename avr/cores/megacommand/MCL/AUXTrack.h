/* Justin Mammarella jmamma@gmail.com 2018 */

#pragma once

#include "DeviceTrack.h"

class AUXTrack : public DeviceTrack {
public:
  virtual void transition_load(uint8_t tracknumber, SeqTrack* seq_track, uint8_t slotnumber) {
  seq_track->speed = chain.speed;
  seq_track->length = chain.length;
  GridTrack::transition_load(tracknumber, seq_track, slotnumber);
  }

};
