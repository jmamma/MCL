#include "platform.h"
#include "helpers.h"
#include "Scales.h"

uint8_t randomScalePitch(const scale_t *scale, uint8_t octaves) {
  uint8_t size = pgm_read_byte(&scale->size);
  uint8_t pitch = pgm_read_byte(&scale->pitches[random(size)]);
  if (octaves == 0) {
    return pitch;
  } else {
    return pitch + 12 * random(octaves);
  }
}

uint8_t scalePitch(uint8_t pitch, uint8_t root, const uint8_t *scale) {
  uint8_t scaleRoot;
  if (pitch < root) {
    scaleRoot = 12 + pitch - root;
  } else {
    scaleRoot = pitch - root;
  }
  uint8_t octave = scaleRoot / 12;
  scaleRoot %= 12;
  return octave * 12 + root + pgm_read_byte(&scale[scaleRoot]);
}

const uint8_t invMajorScale[12] PROGMEM = {
  0, 0, 2, 4, 4, 5, 7, 7, 7, 9, 9, 11
};

const scale_t chromaticScale PROGMEM = {
  12,
  { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 }
};
/* greek modes */
const scale_t ionianScale PROGMEM = {
  7,
  { 0, 2, 4, 5, 7, 9, 11 }
};

const scale_t dorianScale PROGMEM = {
  7,
  { 0, 2, 3, 5, 7, 9, 10 }
};

const scale_t phrygianScale PROGMEM = {
  7,
  { 0, 1, 3, 5, 7, 8, 10 }
};

const scale_t lydianScale PROGMEM = {
  7,
  { 0, 2, 4, 6, 7, 9, 11 }
};

const scale_t mixolydianScale PROGMEM = {
  7,
  { 0, 2, 4, 5, 7, 9, 10 }
};

const scale_t aeolianScale PROGMEM = {
  7,
  { 0, 2, 3, 5, 7, 8, 10 }
};

const scale_t locrianScale PROGMEM = {
  7,
  { 0, 1, 3, 5, 6, 8, 10 }
};

/* harmonic minor modes */
const scale_t harmonicMinorScale PROGMEM = {
  7,
  { 0, 2, 3, 5, 7, 8, 11 }
};

/* melodic minor modes */

const scale_t melodicMinorScale PROGMEM = {
  7,
  { 0, 2, 3, 5, 7, 9, 11 }
};

const scale_t lydianDominantScale PROGMEM = {
  7,
  { 0, 2, 4, 6, 7, 9, 10 }
};

/* symmetric scales */

const scale_t wholeToneScale PROGMEM = {
  6,
  { 0, 2, 4, 6, 8, 10 }
};

const scale_t wholeHalfStepScale PROGMEM = {
  8,
  { 0, 2, 3, 5, 6, 8, 9, 11 }
};

const scale_t halfWholeStepScale PROGMEM = {
  8,
  { 0, 1, 3, 4, 6, 7, 9, 10 }
};

/* pentatonic scales */

const scale_t majorPentatonicScale PROGMEM = {
  5,
  { 0, 2, 4, 7, 9 }
};

const scale_t minorPentatonicScale PROGMEM = {
  5,
  { 0, 3, 5, 7, 10 }
};

const scale_t suspendedPentatonicScale PROGMEM = {
  5,
  { 0, 2, 5, 7, 10 }
};

const scale_t inSenScale PROGMEM = {
  5,
  { 0, 1, 5, 7, 10 }
};

/* derived scales */

const scale_t bluesScale PROGMEM = {
  6,
  { 0, 3, 5, 6, 7, 10 }
};

const scale_t majorBebopScale PROGMEM = {
  8,
  { 0, 2, 4, 5, 7, 8, 9, 11 }
};

const scale_t dominantBebopScale PROGMEM = {
  8,
  { 0, 2, 4, 5, 7, 9, 10, 11 }
};

const scale_t minorBebopScale PROGMEM = {
  8,
  { 0, 2, 3, 4, 5, 7, 9, 10 }
};

/* arpeggios */

const scale_t majorArp PROGMEM = {
  3,
  { 0, 4, 7 }
};

const scale_t minorArp PROGMEM = {
  3,
  { 0, 3, 7 }
};

const scale_t majorMaj7Arp PROGMEM = {
  4,
  { 0, 4, 7, 11 }
};

const scale_t majorMin7Arp PROGMEM = {
  4,
  { 0, 4, 7, 10 }
};

const scale_t minorMin7Arp PROGMEM = {
  4,
  { 0, 3, 7, 10 }
};

const scale_t minorMaj7Arp PROGMEM = {
  4,
  { 0, 3, 7, 11 }
};

const scale_t majorMaj7Arp9 PROGMEM = {
  5,
  { 0, 2, 4, 7, 11 }
};

const scale_t majorMaj7ArpMin9 PROGMEM = {
  5,
  { 0, 1, 4, 7, 11 }
};

const scale_t majorMin7Arp9 PROGMEM = {
  5,
  { 0, 2, 4, 7, 10 }
};

const scale_t majorMin7ArpMin9 PROGMEM = {
  5,
  { 0, 1, 4, 7, 10 }
};

const scale_t minorMin7Arp9 PROGMEM = {
  5,
  { 0, 2, 3, 7, 10 }
};

const scale_t minorMin7ArpMin9 PROGMEM = {
  5,
  { 0, 1, 3, 7, 10 }
};

const scale_t minorMaj7Arp9 PROGMEM = {
  5,
  { 0, 2, 3, 7, 11 }
};

const scale_t minorMaj7ArpMin9 PROGMEM = {
  5,
  { 0, 1, 3, 7, 11 }
};
