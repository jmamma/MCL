#include "MCL.h"
bool WavDesigner::render() {
  DEBUG_PRINT_FN();
  SineOsc sine_osc;
  TriOsc tri_osc;
  PulseOsc pulse_osc;
  SawOsc saw_osc;
 
  //  sine_osc.sample_rate = 88200;
  //  tri_osc.sample_rate = 88200;
  //  pulse_osc.sample_rate = 88200;
  //  saw_osc.sample_rate = 88200;
   // float sample_rate = 88200;
  float sample_rate = 44100;
  Wav wav_file;
  bool overwrite = true;
  if (!wav_file.open("render.wav", overwrite, 1, sample_rate, 16)) {
    return false;
  }
  // Work out lowest base frequency.
  float fund_freq = 20000;
  for (uint8_t i = 0; i < 3; i++) {
    DEBUG_PRINTLN(pages[i].get_freq());
    if (pages[i].get_freq() < fund_freq) {
      fund_freq = pages[i].get_freq();
    }
  }
  // Determine sample lenght for 1 cycle.
  DEBUG_PRINTLN("fund: ");
  DEBUG_PRINTLN(fund_freq);
 // uint32_t n_cycle = floor(sample_rate / fund_freq);
  //fund_freq = sample_rate / n_cycle;
  uint32_t n_cycle = ((floor(sample_rate / fund_freq)));
  sample_rate = n_cycle * fund_freq;

  Osc::sample_rate = sample_rate;

  //while (n_cycle < 152) {
  n_cycle = n_cycle * 2;
  while (n_cycle < 256) {
  n_cycle += n_cycle;
  }
  n_cycle += 2;
//  n_cycle = 8096;   
  DEBUG_PRINTLN("samples");
  DEBUG_PRINTLN(n_cycle);
  // 512B worth of buffer
  uint16_t buffer[256];
  uint16_t samples_so_far = 0;

  int16_t largest_sample_so_far;
  // Render each sample
  uint32_t pos = 0;
  bool write_header = false;
  float max_sine_gain = (float)1 / (float)16;
   for (uint32_t b = 0; b < 500; b++) {
     Serial.println(0);
     Serial.println(" ");
   }
  int32_t last_zero_sample = 0;
  int32_t first_zero_sample;
  int32_t lowest_sample_so_far;
  int32_t lowest_sample_found;

  int32_t first_zero_crossing = 0;
  bool zero_crossing_found = false;
  int32_t last_zero_crossing = 0;
  int32_t last_sample = 0;
  uint8_t zero_crossing_count = 0;
  bool zero_sample_found = false;
  for (uint32_t n = 0; n < n_cycle; n++) {
    float sample = 0;

    // Render each oscillator
    for (uint8_t i = 0; i < 3; i++) {
      float osc_sample = 0;
      switch (pages[i].get_osc_type()) {
      case 0:
        osc_sample += 0;
        break;
      // Sine wave with 16 overtones.
      case 1:
        for (uint8_t h = 1; h <= 1; h++) {
          float sine_gain =
              ((float)pages[i].sine_levels[h - 1] / (float)127) * max_sine_gain;
          // osc_sample += sine_gain * sine_osc.get_sample(n,
          // pages[i].get_freq() * (float) h, 0);

          osc_sample =
              sine_osc.get_sample(n, pages[i].get_freq() * (float)h, 0);

          // DEBUG_PRINTLN(osc_sample);
        }
        break;
      case 2:
        osc_sample +=
            tri_osc.get_sample(n, pages[i].get_freq(), pages[i].get_phase());
        break;
      case 3:
        osc_sample +=
            pulse_osc.get_sample(n, pages[i].get_freq(), pages[i].get_phase());
        break;
      case 4:
        osc_sample +=
            saw_osc.get_sample(n, pages[i].get_freq(), pages[i].get_phase());
        break;
      }
      // Sum oscillator samples together
      sample +=
          dsp.saturate((osc_sample * mixer.get_gain(i)), mixer.get_max_gain());
      // DEBUG_PRINTLN(mixer.get_gain(i));
    }
    // Check for overflow outside of int16_t ranges.
    dsp.saturate(sample, (float)0x7FFE);
   // sample = 30000 * sample;
    if (sample > 0x7FFF) {
      sample = 0x7FFF;
    }
    if (sample < -0x7FFF) {
      sample = -0x7FFF;
    }
    DEBUG_PRINTLN((int16_t)sample);
   // DEBUG_PRINTLN(" ");
    
    //Need to correctly convert from float to int
    int16_t out_sample;
    if(sample > 0) { out_sample = (int16_t)(sample + 0.5); }
    else { out_sample = (int16_t)(sample - 0.5); }

    buffer[samples_so_far] = out_sample;

    samples_so_far++;

    if ((abs(out_sample) > largest_sample_so_far)) {
      largest_sample_so_far = abs(out_sample);
    }
    if ((last_sample * out_sample < 0) || (out_sample == 0)) {
      if (!zero_crossing_found) {
            first_zero_crossing = n;
            zero_crossing_found = true;
      }
      zero_crossing_count++;
      if (zero_crossing_count % 2 > 0) {
      last_zero_crossing = n - 1;
      }
    }
 
    last_sample = out_sample;

    // if (n == 0) {
    //  start_sample = (int16_t)sample;
    // }
    //    if ((int16_t)abs(sample) < 5) {
    //      if (!zero_sample_found) {
    //      zero_sample_found = true;
    //     first_zero_sample = n;
    //   }
    // last_zero_sample = n;
    //  }

    if ((samples_so_far >= 256) || (n == n_cycle - 1)) {
      DEBUG_PRINTLN("let's write");
      DEBUG_PRINTLN(samples_so_far);
      if (!wav_file.write_samples(buffer, samples_so_far, pos, 0,
                                  write_header)) {
        return false;
      }

      pos += samples_so_far;
      samples_so_far = 0;
    }
  }
  // Normalise wav

  float normalize_gain = 0.9 + ((float)(0x7FEE / largest_sample_so_far));
  DEBUG_PRINTLN("gain:");
  DEBUG_PRINTLN(normalize_gain);
  wav_file.file.sync();
  wav_file.apply_gain(normalize_gain);
  write_header = true;
  if (!wav_file.close(write_header)) {
    DEBUG_PRINTLN("could not close");
  }
  DEBUG_PRINTLN("wave stats:");
  DEBUG_PRINTLN(n_cycle);
  DEBUG_PRINTLN(pos);
  DEBUG_PRINTLN(wav_file.header.subchunk2Size);
  DEBUG_PRINTLN("zero crossings");
  DEBUG_PRINTLN(first_zero_crossing);
  DEBUG_PRINTLN(last_zero_crossing);
  DEBUG_PRINTLN(n_cycle - 3); 
//  last_zero_crossing = n_cycle - 3;
  DEBUG_PRINTLN(n_cycle); 
 // first_zero_crossing = 0;
  midi_sds.sendWav(wav_file.filename, 0, SDS_LOOP_FORWARD, first_zero_crossing,
                   last_zero_crossing);
}
WavDesigner wav_designer;
