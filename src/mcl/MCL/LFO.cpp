/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#include "LFO.h"
#include "math.h"
#define DIV_1_127 (1.00f / 127.0f)
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
                                    (float) powf(M_E, (float)-1 * (float)sample_number *
                                                  (float)time_constant));
  return y;
}


uint8_t IExpLFO::get_sample(uint8_t sample_number) {
  ExpLFO e;
  e.amplitude = amplitude;
  uint8_t y = amplitude - e.get_sample(sample_number);
  return y;
}


uint8_t RampLFO::get_sample(uint8_t sample_number) {
  return ((uint16_t)amplitude * sample_number) / LFO_LENGTH;
}

uint8_t IRampLFO::get_sample(uint8_t sample_number) {
  return amplitude - (((uint16_t)amplitude * sample_number) / LFO_LENGTH);

}


uint8_t TriLFO::get_sample(uint8_t sample_number) {
  uint8_t half_length = LFO_LENGTH / 2;
  if (sample_number > half_length) {
    // Descending part: amplitude - slope * (sample - half_length)
    uint8_t offset = sample_number - half_length;
    return amplitude - (((uint16_t)amplitude * offset) / half_length);
  } else {
    // Ascending part: slope * sample
    return ((uint16_t)amplitude * sample_number) / half_length;
  }
}

uint8_t SinLFO::get_sample(uint8_t sample_number) {
  float sample_duration = (float)1.0f / (float)LFO_LENGTH;

  uint8_t y = (float)(amplitude / 2.0f) *
             (float) sin(2.0f * (float)M_PI * (float) sample_number * sample_duration - (0.5f * (float)M_PI)) +
         (float)(amplitude / 2.0f);
  return y;

}


uint8_t LFO::get_sample(uint8_t sample_number) { return 0; }
