#include "MCL_impl.h"

float DSP::saturate(float sample, float max) {
  float percent_max = 100 - (((float)abs(sample) / (float)max) * 100);
  // y = -ax + n, y = - e^-cx + n
  //@ x = 10, y = 0.9n
  // a = .01 * n
  // c = ln(0.1 * n) / 10

  // Percent at which saturation occurs.
  return sample;
  float skew = .07;
  
  float sat_percent = 20;
  //  DEBUG_DUMP(abs(sample));
  //  DEBUG_DUMP(max);
  //  DEBUG_DUMP(percent_max);
  float c = log((float)skew * (float)max) / sat_percent;

  if (abs(sample) < ((1.00 - (sat_percent / 100)) * max)) {
    return sample;
  } else {

    float y = -1 * exp(percent_max * .80) + max;

    //  DEBUG_DUMP(y);
    if (sample < 0) {
      y = y * -1.00;
    }
    return (float)y;
  }
}

DSP dsp;
