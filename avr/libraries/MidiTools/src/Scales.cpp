#include "WProgram.h"
#include "helpers.h"
#include "Scales.h"

uint8_t randomScalePitch(const scale_t *scale, uint8_t octaves) {
  uint8_t pitch = scale->pitches[random(scale->size)];
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
  return octave * 12 + root + scale[scaleRoot];
}

uint8_t invMajorScale[12] = {
  0, 0, 2, 4, 4, 5, 7, 7, 7, 9, 9, 11
};


/* greek modes */
scale_t ionianScale = {
  7,
  { 0, 2, 4, 5, 7, 9, 11 }
};

scale_t dorianScale = {
  7,
  { 0, 2, 3, 5, 7, 9, 10 }
};

scale_t phrygianScale = {
  7,
  { 0, 1, 3, 5, 7, 8, 10 }
};

scale_t lydianScale = {
  7,
  { 0, 2, 4, 6, 7, 9, 11 }
};

scale_t mixolydianScale = {
  7,
  { 0, 2, 4, 5, 7, 9, 10 }
};

scale_t aeolianScale = {
  7,
  { 0, 2, 3, 5, 7, 8, 10 }
};

scale_t locrianScale = {
  7,
  { 0, 1, 3, 5, 6, 8, 10 }
};

/* harmonic minor modes */
scale_t harmonicMinorScale = {
  7,
  { 0, 2, 3, 5, 7, 8, 11 }
};

/* melodic minor modes */

scale_t melodicMinorScale = {
  7,
  { 0, 2, 3, 5, 7, 9, 11 }
};

scale_t lydianDominantScale = {
  7,
  { 0, 2, 4, 6, 7, 9, 10 }
};

/* symmetric scales */

scale_t wholeToneScale = {
  6,
  { 0, 2, 4, 6, 8, 10 }
};

scale_t wholeHalfStepScale = {
  8,
  { 0, 2, 3, 5, 6, 8, 9, 11 }
};

scale_t halfWholeStepScale = {
  8,
  { 0, 1, 3, 4, 6, 7, 9, 10 }
};

/* pentatonic scales */

scale_t majorPentatonicScale = {
  5,
  { 0, 2, 4, 7, 9 }
};

scale_t minorPentatonicScale = {
  5,
  { 0, 3, 5, 7, 10 }
};

scale_t suspendedPentatonicScale = {
  5,
  { 0, 2, 5, 7, 10 }
};

scale_t inSenScale = {
  5,
  { 0, 1, 5, 7, 10 }
};

/* derived scales */

scale_t bluesScale = {
  6,
  { 0, 3, 5, 6, 7, 10 }
};

scale_t majorBebopScale = {
  8,
  { 0, 2, 4, 5, 7, 8, 9, 11 }
};

scale_t dominantBebopScale = {
  8,
  { 0, 2, 4, 5, 7, 9, 10, 11 }
};

scale_t minorBebopScale = {
  8,
  { 0, 2, 3, 4, 5, 7, 9, 10 }
};

/* arpeggios */

scale_t majorArp = {
  3,
  { 0, 4, 7 }
};

scale_t minorArp = {
  3,
  { 0, 3, 7 }
};

scale_t majorMaj7Arp = {
  4,
  { 0, 4, 7, 11 }
};

scale_t majorMin7Arp = {
  4,
  { 0, 4, 7, 10 }
};

scale_t minorMin7Arp = {
  4,
  { 0, 3, 7, 10 }
};

scale_t minorMaj7Arp = {
  4,
  { 0, 3, 7, 11 }
};

scale_t majorMaj7Arp9 = {
  5,
  { 0, 2, 4, 7, 11 }
};

scale_t majorMaj7ArpMin9 = {
  5,
  { 0, 1, 4, 7, 11 }
};

scale_t majorMin7Arp9 = {
  5,
  { 0, 2, 4, 7, 10 }
};

scale_t majorMin7ArpMin9 = {
  5,
  { 0, 1, 4, 7, 10 }
};

scale_t minorMin7Arp9 = {
  5,
  { 0, 2, 3, 7, 10 }
};

scale_t minorMin7ArpMin9 = {
  5,
  { 0, 1, 3, 7, 10 }
};

scale_t minorMaj7Arp9 = {
  5,
  { 0, 2, 3, 7, 11 }
};

scale_t minorMaj7ArpMin9 = {
  5,
  { 0, 1, 3, 7, 11 }
};
