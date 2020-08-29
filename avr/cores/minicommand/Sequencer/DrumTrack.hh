#ifndef DRUMTRACK_H__
#define DRUMTRACK_H__

#include "WProgram.h"

class DrumTrack {
public:
  uint8_t len;
  uint64_t pattern;
  uint8_t offset;
  
  DrumTrack(uint32_t _pattern = 0, uint8_t _len = 16, uint8_t _offset = 0) {
    pattern = _pattern;
    len = _len;
    offset = _offset;
  }

  virtual bool isHit(uint8_t pos);
};

class EuclidDrumTrack : public DrumTrack {
 public:
  EuclidDrumTrack(uint8_t pulses, uint8_t len, uint8_t _offset = 0);
  void setEuclid(uint8_t pulses, uint8_t _len, uint8_t _offset = 0);
};

class PitchTrack {
 public:
  uint8_t pitches[32];
  DrumTrack *track;

  uint8_t len;
  uint8_t idx;
  uint8_t velocity;

  PitchTrack(DrumTrack *_track, uint8_t _len, uint8_t _velocity = 100) {
    len = _len;
    track = _track;
    velocity = _velocity;
    idx = 0;
  }

  void playHit(uint8_t pos);

  void reset() {
    idx = 0;
  }
};


#endif /* DRUMTRACK_H__ */
