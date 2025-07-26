/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef DSP_H__
#define DSP_H__

#include "mcl.h"
#include "math.h"

#define FULL_SCALE 0xFFFF
#define HALF_FULL_SCALE 0x8000
#define MAX_HEADROOM 0x7210 //approx 1db off full scale

class DSP {
public:
  float saturate(float sample, float max);
};

extern DSP dsp;

#endif /* DSP_H__ */
