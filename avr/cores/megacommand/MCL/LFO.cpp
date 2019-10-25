/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#include "Math.h"
#include "LFO.h"
#define DIV_1_127 (1.00 / MAX)
//Exponential Rise Formua:
//y = M * (1-e^(-x/a));
//M = Maximum
//a = time constant.
//0.63M = M * (1 - e^(-x/a))
//0.63 = (1 - e^(-x/a))
//e^(-x/a) = ( 1 - 0.63)
//e^(-x/a) = 0.37
// -x/a = ln(0.37) [1]
//
//at t = M, y = M - 1
//
//(M - 1 ) = M * (1 - ^e(-M/a))
//M - 1 = M - Me^(-M/a)
//e^(-M/a) = 1/M
//ln(1/M) = -M/a
//a = - M / (ln(1/M) [2]
//
//For M = 127. a = 26
//
//

#define MAX 127

uint8_t ExpLFO::get_sample(uint8_t sample_number) {
 uint8_t y = MAX - (uint8_t) ((float) MAX * powf(M_E, (float) -1 * (float) sample_number * (float)time_constant));
 return y;
}

uint8_t TriLFO::get_sample(uint8_t sample_number) {
 uint8_t y;
 if (sample_number > LFO_LENGTH / 2) {
 y = (127 - sample_number) * 2;
 }
 else {
 y = (sample_number * 2);
 }
 return y;
}

uint8_t SinLFO::get_sample(uint8_t sample_number) {
  float sample_duration = (float)1 / (float)LFO_LENGTH;
  // float sample_duration = (float) 1 / (float) freq;
  return (float) (MAX / 2.0) * cos(2 * PI * 1 * sample_number * sample_duration) + (float) (MAX / 2.0);
}


uint8_t LFO::get_sample(uint8_t sample_number) {
 return 0;
}

