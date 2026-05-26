/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#include "math.h"
#include "Osc.h"

float Osc::get_sample(uint32_t sample_number, float freq) {
 return 0;
}
void Osc::set_sample_rate(float hz) { sample_rate = hz; }

float Osc::poly_blep(float t, float freq) {

    const float two_pi = 2.0f * (float)PI;
    float phase_inc = (freq * two_pi) / (float) sample_rate;
    float dt = phase_inc / two_pi;

    // 0 <= t < 1
    if (t < dt) {
        t /= dt;
        return t + t - t * t - 1.0f;
    }
    // -1 < t < 0
    else if (t > 1.0f - dt) {
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }
    // 0 otherwise
    else return 0.0f;
}


float SineOsc::get_sample(uint32_t sample_number, float freq) {

  float sample_duration = (float)1 / (float)sample_rate;
  // float sample_duration = (float) 1 / (float) freq;
  return -sinf((2.0f * (float)PI) * freq * sample_number * sample_duration);
  // return sin(radians * sample_time);
}

float PulseOsc::get_sample(uint32_t sample_number, float freq) {

  float n_cycle = (uint16_t)(sample_rate / freq);

  sample_number = sample_number + (n_cycle);
  float cycle_pos = (uint32_t)((float)sample_number / n_cycle);
  float n = (float)sample_number - cycle_pos * n_cycle;
  float n_edge = (uint16_t)(n_cycle * width);

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

  float n_cycle = (uint16_t)(sample_rate / freq);
  sample_number = sample_number + (n_cycle * 0.5f);
  float cycle_pos = (uint32_t)((float)sample_number / n_cycle);
  float n = (float)sample_number - cycle_pos * n_cycle;

  float n_edge = (uint16_t)(n_cycle * (width + 0.5f));
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
  float n_cycle = (uint16_t)(sample_rate / freq);
  sample_number = sample_number + (n_cycle * 0.75f);

  float cycle_pos = (uint32_t)((float)sample_number / n_cycle);
  float n = (float)sample_number - cycle_pos * n_cycle;
  float n_edge = (uint16_t)(n_cycle * width);

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
                         const uint8_t *usr_values) {

  float n_cycle = (uint16_t)(sample_rate / freq);

  float cycle_pos = (uint32_t)((float)sample_number / n_cycle);
  float n = (float)sample_number - cycle_pos * n_cycle;

  float partition_size_n = n_cycle / (float)16;

  uint8_t start = (uint8_t)(n / partition_size_n);
  uint8_t end = (start + 1) & 0x0F;
  float v_start = (float)(64 - usr_values[start]) / (float)64;
  float v_end = (float)(64 - usr_values[end]) / (float)64;
  float phase = (n - start * partition_size_n) / partition_size_n;

  return v_start + (v_end - v_start) * phase;
}

float render_osc_sample(uint8_t osc_type, float width,
                        const uint8_t *sine_levels,
                        const uint8_t *usr_values, uint16_t sine_level_sum,
                        uint32_t sample_number, float freq,
                        SineOsc &sine_osc, TriOsc &tri_osc,
                        PulseOsc &pulse_osc, SawOsc &saw_osc,
                        UsrOsc &usr_osc) {
  float osc_sample = 0;
  switch (osc_type) {
  case SIN_OSC:
    if (sine_level_sum != 0) {
      // The old per-harmonic gain constant cancels with peak normalization, so
      // the normalized sine blend is just each harmonic weighted by level / sum.
      for (uint8_t h = 1; h <= 16; h++) {
        uint8_t sine_level = sine_levels[h - 1];
        if (sine_level != 0) {
          osc_sample += sine_osc.get_sample(sample_number, freq * (float)h) *
                        (float)sine_level;
        }
      }
      osc_sample /= (float)sine_level_sum;
    }
    break;
  case TRI_OSC:
    tri_osc.width = width;
    osc_sample = tri_osc.get_sample(sample_number, freq);
    break;
  case PUL_OSC:
    pulse_osc.width = width;
    osc_sample = pulse_osc.get_sample(sample_number, freq);
    break;
  case SAW_OSC:
    saw_osc.width = width;
    osc_sample = saw_osc.get_sample(sample_number, freq);
    break;
  case USR_OSC:
    osc_sample = usr_osc.get_sample(sample_number, freq, usr_values);
    break;
  }
  return osc_sample;
}
