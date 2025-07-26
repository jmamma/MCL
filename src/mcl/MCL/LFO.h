/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef LFOOSC_H__
#define LFOOSC_H__

#include "mcl.h"
#include "math.h"

#define EXP_LFO 1
#define LFO_LENGTH 96

#define SIN_WAV 0
#define TRI_WAV 1
#define RAMP_WAV 2
#define IEXP_WAV 3
#define IRAMP_WAV 4
#define EXP_WAV 5

class LFO {
public:
  uint8_t amplitude;
  virtual uint8_t get_sample(uint8_t sample_number);
};

class ExpLFO : public LFO {

  float time_constant;
public:
  ExpLFO(float time_constant_ = 40) { time_constant = 1.00f / time_constant_; }
  uint8_t get_sample(uint8_t sample_number);
};


class IExpLFO : public LFO {

  float time_constant;
public:
  IExpLFO(float time_constant_ = 40) { time_constant = 1.00f / time_constant_; }
  uint8_t get_sample(uint8_t sample_number);
};

class RampLFO : public LFO {

public:
  RampLFO() { }
  uint8_t get_sample(uint8_t sample_number);
};

class IRampLFO : public LFO {

public:
  IRampLFO() { }
  uint8_t get_sample(uint8_t sample_number);
};


class TriLFO : public LFO {

public:
  TriLFO() { }
  uint8_t get_sample(uint8_t sample_number);
};

class SinLFO : public LFO {

public:
  SinLFO() { }
  uint8_t get_sample(uint8_t sample_number);
};


#endif /* LFOOSC_H__ */
