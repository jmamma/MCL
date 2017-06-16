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

extern uint8_t invMajorScale[12];

extern scale_t ionianScale;
extern scale_t dorianScale;
extern scale_t phrygianScale;
extern scale_t lydianScale;
extern scale_t mixolydianScale;
extern scale_t aeolianScale;
extern scale_t locrianScale;

extern scale_t harmonicMinorScale;

extern scale_t melodicMinorScale;
extern scale_t lydianDominantScale;

extern scale_t wholeToneScale;
extern scale_t wholeHalfStepScale;
extern scale_t halfWholeStepScale;

extern scale_t majorPentatonicScale;
extern scale_t minorPentatonicScale;
extern scale_t suspendedPentatonicScale;
extern scale_t inSenScale;

extern scale_t bluesScale;
extern scale_t majorBebopScale;
extern scale_t dominantBebopScale;
extern scale_t minorBebopScale;

extern scale_t majorArp;
extern scale_t minorArp;
extern scale_t majorMaj7Arp;
extern scale_t majorMin7Arp;
extern scale_t minorMin7Arp;
extern scale_t minorMaj7Arp;
extern scale_t majorMaj7Arp9;
extern scale_t majorMaj7ArpMin9;
extern scale_t majorMin7Arp9;
extern scale_t majorMin7ArpMin9;
extern scale_t minorMin7Arp9;
extern scale_t minorMin7ArpMin9;
extern scale_t minorMaj7Arp9;
extern scale_t minorMaj7ArpMin9;

uint8_t randomScalePitch(const scale_t *scale, uint8_t octaves = 0);
uint8_t scalePitch(uint8_t pitch, uint8_t root, const uint8_t *scale);

/* @} @} @} */

#endif /* SCALES_H__ */
