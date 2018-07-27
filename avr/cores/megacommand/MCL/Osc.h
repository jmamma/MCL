/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef OSC_H__
#define OSC_H__

#include "MCL.h"
#include "Math.h"

#define SIN_OSC 1
#define TRI_OSC 2
#define PUL_OSC 3
#define SAW_OSC 4
#define USR_OSC 5

class Osc {
public:
  float sample_rate;
  uint8_t phase = 0;
  Osc(float sample_rate_ = 44100) { sample_rate = sample_rate_; }
  virtual float get_sample(uint32_t sample_number, float freq, float phase);

  void set_sample_rate(float hz);
};

class SineOsc : public Osc {

public:
  SineOsc(float sample_rate_ = 44100) { set_sample_rate(sample_rate_); }
  float get_sample(uint32_t sample_number, float freq, float phase);
};

class PulseOsc : public Osc {

public:
  float width; // from 0 -> 1.0
  float skew;
  float vmin;
  float vmax;
  PulseOsc(float sample_rate_ = 44100) {
    set_sample_rate(sample_rate_);
    set_width(0.5);
    skew = 0.05;
    vmax = 1;
    vmin = -1;
  }
  float get_sample(uint32_t sample_number, float freq, float phase);
  void set_width(float width_);
  void set_skew(float skew_);
};

class SawOsc : public Osc {

public:
  float skew;
  float vmin;
  float vmax;
  SawOsc(float sample_rate_ = 44100) {
    set_sample_rate(sample_rate_);
    skew = 2;
    vmax = 1;
    vmin = -1;
  }
  float get_sample(uint32_t sample_number, float freq, float phase);
};

class TriOsc : public Osc {

public:
  float width;
  float vmin;
  float vmax;
  TriOsc(float sample_rate_ = 44100) {
    set_sample_rate(sample_rate_);
    vmax = 1;
    vmin = -1;
    width = 0.5;
  }
  float get_sample(uint32_t sample_number, float freq, float phase);
};

class UsrOsc : public Osc {

public:
  UsrOsc(float sample_rate_ = 44100) { set_sample_rate(sample_rate_); }
  float get_sample(uint32_t sample_number, float freq, float phase,
                   uint8_t *usr_values);

};

#endif /* OSC_H__ */
