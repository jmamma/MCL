/* Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef SCALES_H__
#define SCALES_H__

#include <inttypes.h>

/**
 * \addtogroup Midi
 *
 * @{
 **/

/**
 * \addtogroup midi_tools Midi Tools
 *
 * @{
 **/

/**
 * \addtogroup midi_scales Midi Scales
 *
 * @{
 **/

typedef struct scale_s {
  uint8_t size;
  uint8_t pitches[12];
} scale_t;

#define majorScale ionianScale
#define minorScale aeolianScale

extern const uint8_t invMajorScale[12] PROGMEM;

extern const scale_t chromaticScale PROGMEM;
extern const scale_t ionianScale PROGMEM;
extern const scale_t dorianScale PROGMEM;
extern const scale_t phrygianScale PROGMEM;
extern const scale_t lydianScale PROGMEM;
extern const scale_t mixolydianScale PROGMEM;
extern const scale_t aeolianScale PROGMEM;
extern const scale_t locrianScale PROGMEM;

extern const scale_t harmonicMinorScale PROGMEM;

extern const scale_t melodicMinorScale PROGMEM;
extern const scale_t lydianDominantScale PROGMEM;

extern const scale_t wholeToneScale PROGMEM;
extern const scale_t wholeHalfStepScale PROGMEM;
extern const scale_t halfWholeStepScale PROGMEM;

extern const scale_t majorPentatonicScale PROGMEM;
extern const scale_t minorPentatonicScale PROGMEM;
extern const scale_t suspendedPentatonicScale PROGMEM;
extern const scale_t inSenScale PROGMEM;

extern const scale_t bluesScale PROGMEM;
extern const scale_t majorBebopScale PROGMEM;
extern const scale_t dominantBebopScale PROGMEM;
extern const scale_t minorBebopScale PROGMEM;

extern const scale_t majorArp PROGMEM;
extern const scale_t minorArp PROGMEM;
extern const scale_t majorMaj7Arp PROGMEM;
extern const scale_t majorMin7Arp PROGMEM;
extern const scale_t minorMin7Arp PROGMEM;
extern const scale_t minorMaj7Arp PROGMEM;
extern const scale_t majorMaj7Arp9 PROGMEM;
extern const scale_t majorMaj7ArpMin9 PROGMEM;
extern const scale_t majorMin7Arp9 PROGMEM;
extern const scale_t majorMin7ArpMin9 PROGMEM;
extern const scale_t minorMin7Arp9 PROGMEM;
extern const scale_t minorMin7ArpMin9 PROGMEM;
extern const scale_t minorMaj7Arp9 PROGMEM;
extern const scale_t minorMaj7ArpMin9 PROGMEM;

uint8_t randomScalePitch(const scale_t *scale, uint8_t octaves = 0);
uint8_t scalePitch(uint8_t pitch, uint8_t root, const uint8_t *scale);

/* @} @} @} */

#endif /* SCALES_H__ */
