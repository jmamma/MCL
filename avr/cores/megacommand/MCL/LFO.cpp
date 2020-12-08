/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#include "LFO.h"
#include "math.h"
#define DIV_1_127 (1.00 / 127.0)
// Exponential Rise Formua:
// y = M * (1-e^(-x/a));
// M = Maximum
// a = time constant.
// 0.63M = M * (1 - e^(-x/a))
// 0.63 = (1 - e^(-x/a))
// e^(-x/a) = ( 1 - 0.63)
// e^(-x/a) = 0.37
// -x/a = ln(0.37) [1]
//
// at t = M, y = M - 1
//
//(M - 1 ) = M * (1 - ^e(-M/a))
// M - 1 = M - Me^(-M/a)
// e^(-M/a) = 1/M
// ln(1/M) = -M/a
// a = - M / (ln(1/M) [2]
//
// For M = 127. a = 26
//
//

uint8_t ExpLFO::get_sample(uint8_t sample_number) {
  uint8_t y = (uint8_t)((float)amplitude *
                                    powf(M_E, (float)-1 * (float)sample_number *
                                                  (float)time_constant));
  return y;
}


uint8_t IExpLFO::get_sample(uint8_t sample_number) {
  uint8_t y = amplitude - (uint8_t)((float)amplitude *
                                    powf(M_E, (float)-1 * (float)sample_number *
                                                  (float)time_constant));
  return y;
}


uint8_t RampLFO::get_sample(uint8_t sample_number) {
  uint8_t y = ((float)amplitude / (float)(LFO_LENGTH)) * (sample_number);

  return y;
}

uint8_t IRampLFO::get_sample(uint8_t sample_number) {
  uint8_t y = amplitude - ((float)amplitude / (float)(LFO_LENGTH)) * (sample_number);

  return y;
}


uint8_t TriLFO::get_sample(uint8_t sample_number) {
  uint8_t y;
  if (sample_number > LFO_LENGTH / 2) {
    y = amplitude -  1 * ( (float) amplitude / (float) (LFO_LENGTH / 2)) * (sample_number - (LFO_LENGTH / 2));
  } else {
    y = ((float)amplitude / (float)(LFO_LENGTH / 2)) * (sample_number);
  }
  return y;
}

uint8_t SinLFO::get_sample(uint8_t sample_number) {
  float sample_duration = (float)1.0 / (float)LFO_LENGTH;

  uint8_t y = (float)(amplitude / 2.0) *
             sin(2.0 * PI * (float) sample_number * sample_duration - (0.5 * PI)) +
         (float)(amplitude / 2.0);
  return y;

}


uint8_t LFO::get_sample(uint8_t sample_number) { return 0; }
