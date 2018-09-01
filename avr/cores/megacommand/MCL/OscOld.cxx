/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#include "Math.h"
#include "Osc.h"

uint16_t Osc::sample_rate;

void Osc::set_sample_rate(uint16_t hz) {
  sample_rate = hz;
}


float SineOsc::get_sample(uint32_t sample_number, float freq, float phase) {
 
   float sample_duration = (float)1 / (float)sample_rate;
   DEBUG_PRINTLN(sample_number);
   // float sample_duration = (float) 1 / (float) freq;
   float sample_time = sample_duration * sample_number;
   float radians = 2 * PI * (freq); 
 // float sample_time =  (float) sample_number / (float) sample_rate;
 // float radians = (1.00 / (float) (2.00 * M_PI)) * (freq);
   DEBUG_PRINTLN(sin(radians * sample_time));
  // DEBUG_PRINTLN("k");
   return sin(2 * PI * freq * sample_number * sample_duration);
   //return sin(radians * sample_time);
}

float PulseOsc::get_sample(uint32_t sample_number, float freq, float phase) {


  float n_cycle = floor(sample_rate / freq);

  float n = (float) sample_number - (float) floor(sample_number / n_cycle) * n_cycle;
  float n_edge = floor(n_cycle * width);
  skew = skew * .0001;  
  n = n * .0001;
  n_edge = n_edge * .0001;
  n_cycle = n_cycle * .0001;
  if (n < n_edge) {
    float c = skew;
    // y = cln(at + b);
    //
    // t = 0, y = vmin
    // vmin = c * ln ( 0 + b )
    // b = e ^(vmin/c)
    //
    // t = half_cycle_time, y = vmax;
    // vmax = c * ln ( a * half_cycle_time + b)

    // a = (e^(vmax/c) - e^(vmin/c)) / half_cyle_time
    //   DEBUG_PRINTLN(full_cycle_time);
    //   DEBUG_PRINTLN(edge_start_time);
    //   DEBUG_PRINTLN(time_within_cycle);
    //   DEBUG_PRINTLN(skew);
    float b = exp(vmin / c);
    float a = (exp(vmax / c) - b) / n_edge;
    float y = c * log(a * n + b);
    // DEBUG_PRINTLN(y);
    return y;
  } else {
    float c = skew * 5;
    // t = edge_start_time, y = vmax
    // t = full_cycle_time, y = vmin
    // y = ae^(-cx) + b
    // vmax = ae^(-ct1) + b
    // b = vmax - ae^(-ct1)
    // vmin = ae^(-ct2) + b
    // a= (vmin - b) * e^(ct2)
    // a= (vmin - vmax + ae^(-ct1))  * e ^(ct2)
    // a = (e^(ct2) (vmin - vmax) + ae^(-ct1)e^(ct2)
    // a (1 - e^(-ct1)e^(ct2) = e ^(ct2)(vmin - vmax)
    // a = (e^(ct2)(vmin-vmax) / (1-e^(-ct1)e^(ct2))

    double k = exp(-1 * c * n_edge);
    double m = exp(c * n_cycle);
    double a = (m * (vmin - vmax)) / (1 - (k * m));
    double b = vmax - (a * k);
    double y = a * exp(-1 * c * n) + b;

    return y;
  }
}

void PulseOsc::set_width(float width_) { width = width_; }
void PulseOsc::set_skew(float skew_) { skew = skew_; }

float SawOsc::get_sample(uint32_t sample_number, float freq, float phase) {



  float n_cycle = floor(sample_rate / freq);

  float n = (float) sample_number - (float) floor(sample_number / n_cycle) * n_cycle;
  float n_edge = n_cycle * .90;



  if (n < n_edge) {
    float c = skew;
    float b = exp(vmin / c);
    float a = (exp(vmax / c) - b) / n_edge;
    float y = c * log(a * n + b);
    // DEBUG_PRINTLN(y);
    return y;
  } else {
    float c = (float)1/skew;
    // c = .001;
   // n = n / (float)1000;
    //n_edge = n_edge / (float) 1000;
   // n_cycle = n_cycle / (float) 1000;
    // t = edge_start_time, y = vmax
    // t = full_cycle_time, y = vmin
    // y = ae^(-cx) + b
    // vmax = ae^(-ct1) + b
    // b = vmax - ae^(-ct1)
    // vmin = ae^(-ct2) + b
    // a= (vmin - b) * e^(ct2)
    // a= (vmin - vmax + ae^(-ct1))  * e ^(ct2)
    // a = (e^(ct2) (vmin - vmax) + ae^(-ct1)e^(ct2)
    // a (1 - e^(-ct1)e^(ct2) = e ^(ct2)(vmin - vmax)
    // a = (e^(ct2)(vmin-vmax) / (1-e^(-ct1)e^(ct2))
//    n = n / (float) 1000;
  //  n_edge = n_edge / (float) 1000;

    double k = exp(-1 * c * n_edge);
    double m = exp(c * n_cycle);
    double a = (m * (vmin - vmax)) / (1 - (k * m));
    double b = vmax - (a * k);
    float y = a * exp(-1 * c * n) + b;
   return y;
  }
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
