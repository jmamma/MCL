/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef DSP_H__
#define DSP_H__

#include "MCL.h"
#include "Math.h"

class DSP {
public:
  float saturate(float sample, float max);
};

extern DSP dsp;

#endif /* DSP_H__ */
