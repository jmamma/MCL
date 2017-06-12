#include "WProgram.h"
#include "helpers.h"
#include "DrumTrack.hh"

bool DrumTrack::isHit(uint8_t pos) {
  uint8_t idx = (pos + offset) % len;
  return IS_BIT_SET64(pattern, idx);
}

EuclidDrumTrack::EuclidDrumTrack(uint8_t pulses, uint8_t _len, uint8_t _offset)
  : DrumTrack(0, _len, _offset) {
  setEuclid(pulses, _len, _offset);
}

void EuclidDrumTrack::setEuclid(uint8_t pulses, uint8_t _len, uint8_t _offset) {
  offset = _offset;
  len = _len;
  pattern = 0;
  if (pulses == 0)
    return;

  uint8_t cnt = len;
  for (uint8_t i = 0; i < len; i++) {
    if (i < (len - 1)) {
			pattern <<= 1;
		}
		
    if (cnt >= len) {
      SET_BIT64(pattern, 0);
      cnt -= len;
    }
    cnt += pulses;
  }
}

void PitchTrack::playHit(uint8_t pos) {
  if (track->isHit(pos)) {
    MidiUart.sendNoteOn(pitches[idx], velocity);
    MidiUart.sendNoteOff(pitches[idx]);
    idx = (idx + 1) % len;
  }
}
