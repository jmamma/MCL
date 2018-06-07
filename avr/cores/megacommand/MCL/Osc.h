/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef OSC_H__
#define OSC_H__

#include "MCL.h"
#include "Math.h"

class Osc {
public:
  uint16_t sample_rate;
  float sample_duration;
  uint8_t phase = 0;
  uint16_t freq = 220;
  int16_t amplitude = 0xFFFE; // Full scale;

  virtual float get_sample(uint32_t sample_number);

  void set_sample_rate(uint16_t hz);
};

class SineOsc : public Osc {

public:
  SineOsc() { set_sample_rate(44100); }
  float get_sample(uint32_t sample_number);
};

class PulseOsc : public Osc {

public:
  float width; //from 0 -> 1.0
  float skew;
  float vmin;
  float vmax;
  PulseOsc() {
    set_sample_rate(44100);
    set_width(0.5);
    skew = 0.05;
    vmax = 1;
    vmin = -1;
  }
  float get_sample(uint32_t sample_number);
  void set_width(float width_);
  void set_skew(float skew_);
};

class SawOsc : public Osc {

public:
  float skew;
  float vmin;
  float vmax;
  SawOsc() {
    set_sample_rate(44100);
    skew = 2;
    vmax = 1;
    vmin = -1;
  }
  float get_sample(uint32_t sample_number);
};

class TriOsc : public Osc {

public:
  float width;
  float vmin;
  float vmax;
  TriOsc() {
    set_sample_rate(44100);
    vmax = 1;
    vmin = -1;
    width = 0.5;
  }
  float get_sample(uint32_t sample_number);
};



#endif /* OSC_H__ */
