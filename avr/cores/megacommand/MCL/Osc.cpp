/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#include "math.h"
#include "Osc.h"

float Osc::get_sample(uint32_t sample_number, float freq) {
 return 0;
}
void Osc::set_sample_rate(float hz) { sample_rate = hz; }

float Osc::poly_blep(float t, float freq) {

    float phase_inc = (freq * 2.0 * PI) / (float) sample_rate;
    float dt = phase_inc / (2.0 * PI);

    // 0 <= t < 1
    if (t < dt) {
        t /= dt;
        return t+t - t*t - 1.0;
    }
    // -1 < t < 0
    else if (t > 1.0 - dt) {
        t = (t - 1.0) / dt;
        return t*t + t+t + 1.0;
    }
    // 0 otherwise
    else return 0.0;
}


float SineOsc::get_sample(uint32_t sample_number, float freq) {

  float sample_duration = (float)1 / (float)sample_rate;
  // float sample_duration = (float) 1 / (float) freq;
  return -1 * sin(2 * PI * freq * sample_number * sample_duration);
  // return sin(radians * sample_time);
}

float PulseOsc::get_sample(uint32_t sample_number, float freq) {

  float n_cycle = floor(sample_rate / freq);

  sample_number = sample_number + (n_cycle);
  float n =
      (float)sample_number - (float)floor(sample_number / n_cycle) * n_cycle;
  float n_edge = floor(n_cycle * width);

  float out = 0.0;

  if (n < n_edge) {
    out = vmin;
  } else {
    out = vmax;
  }

  /*
  float phase_inc = (freq * 2.0 * PI) / (float) sample_rate;
  float phase = n * phase_inc;

  float t = phase / (2.0 * PI);
  out -= poly_blep(t, freq);
  out += poly_blep(fmod(t + (1.0 - width), 1.0), freq);
  */
  return out;
}

void PulseOsc::set_width(float width_) { width = width_; }
void PulseOsc::set_skew(float skew_) { skew = skew_; }

float SawOsc::get_sample(uint32_t sample_number, float freq) {

  float n_cycle = floor(sample_rate / freq);
  sample_number = sample_number + (n_cycle * .5);
  float n =
      (float)sample_number - (float)floor(sample_number / n_cycle) * n_cycle;

  float n_edge = floor(n_cycle * (width + .5));
  float a = ((vmin - vmax) / n_edge);
  float b = vmax;
  float y = a * n + b;

  float out = 0.0;
  if (n < n_edge) {
   out = y;
  }
  else {
   out = vmax;
  }

  return out;
}

float TriOsc::get_sample(uint32_t sample_number, float freq) {
  float n_cycle = floor(sample_rate / freq);
  sample_number = sample_number + (n_cycle * .75);

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

float UsrOsc::get_sample(uint32_t sample_number, float freq,
                         uint8_t *usr_values) {

  float n_cycle = floor(sample_rate / freq);

  float n = sample_number - floor(sample_number / n_cycle) * n_cycle;

  float partition_size_n = n_cycle / (float)16;

  int start = floor(n / partition_size_n);
  float n_start = start * partition_size_n;
  int end;
  float n_end;
  if (start < 15) {
    end = start + 1;
    n_end = end * partition_size_n;

  } else {
    end = 0;
    n_end = n_cycle;
  }
  float v_start = -1 * (float)(usr_values[start] - 64) / (float)64;
  float v_end = -1 * (float)(usr_values[end] - 64) / (float)64;

  float m = ((v_end - v_start) / (n_end - n_start));
  float b = v_start - m * n_start;
  float y = m * n + b;

  return y;
}
