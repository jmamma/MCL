/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#include "Math.h"
#include "Osc.h"

float Osc::sample_rate;

void Osc::set_sample_rate(float hz) { sample_rate = hz; }

float SineOsc::get_sample(uint32_t sample_number, float freq, float phase) {

  float sample_duration = (float)1 / (float)sample_rate;
  // float sample_duration = (float) 1 / (float) freq;
  float sample_time = sample_duration * sample_number;
  float radians = 2 * PI * (freq);
  return sin(2 * PI * freq * sample_number * sample_duration);
  // return sin(radians * sample_time);
}

float PulseOsc::get_sample(uint32_t sample_number, float freq, float phase) {

  float n_cycle = floor(sample_rate / freq);

  float n =
      (float)sample_number - (float)floor(sample_number / n_cycle) * n_cycle;
  float n_edge = floor(n_cycle * width);
  if (n < n_edge) {
    return vmin;
  } else {
    return vmax;
  }
}

void PulseOsc::set_width(float width_) { width = width_; }
void PulseOsc::set_skew(float skew_) { skew = skew_; }

float SawOsc::get_sample(uint32_t sample_number, float freq, float phase) {

  float n_cycle = floor(sample_rate / freq);

  float n =
      (float)sample_number - (float)floor(sample_number / n_cycle) * n_cycle;

  float a = ((vmax - vmin) / n_cycle);
  float b = vmin;
  float y = a * n + b;
  return y;
}
float TriOsc::get_sample(uint32_t sample_number, float freq, float phase) {
  float n_cycle = floor(sample_rate / freq);

  float n = sample_number - floor(sample_number / n_cycle) * n_cycle;
  float n_edge = floor(n_cycle * width);

  if (n < n_edge) {
    float b = vmin;
    float a = (vmax - vmin) / n_edge;
    float y = a * n + b;
    return y;
  } else {
    float a = (vmin - vmax) / (float)(n_cycle - n_edge);
    float b = vmax - a * n_edge;
    float y = a * n + b;
    return y;
  }
}
