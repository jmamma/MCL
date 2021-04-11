#include "MCL_impl.h"

void WavDesigner::prompt_send() {
    if (mcl_gui.wait_for_confirm("Send Sample", "Overwrite sample slot?")) {

#ifdef OLED_DISPLAY
      oled_display.clearDisplay();
#endif
      GUI.setLine(GUI.LINE1);
      GUI.put_string_at(0, "Render..");
      LCD.goLine(0);
      LCD.puts(GUI.lines[0].data);
#ifdef OLED_DISPLAY
      oled_display.display();
      oled_display.clearDisplay();
#endif
      wd.render();
      GUI.put_string_at(0, "Sending..");
      LCD.goLine(0);
      LCD.puts(GUI.lines[0].data);
#ifdef OLED_DISPLAY
      oled_display.display();
#endif
     wd.send();
   }
}

bool WavDesigner::render() {
  DEBUG_PRINT_FN();
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
    if ((pages[i].get_osc_type() > 0) && (pages[i].get_freq() < fund_freq)) {
      fund_freq = pages[i].get_freq();
    }
  }
  // Determine sample lenght for 1 cycle.
  DEBUG_PRINTLN(F("fund: "));
  DEBUG_PRINTLN(fund_freq);

  // Recalculate sample rate using base frequency, for sample alignment.

  uint32_t n_cycle = ((floor(sample_rate / fund_freq)));

  sample_rate = n_cycle * fund_freq;

  SineOsc sine_osc(sample_rate);
  TriOsc tri_osc(sample_rate);
  PulseOsc pulse_osc(sample_rate);
  SawOsc saw_osc(sample_rate);
  UsrOsc usr_osc(sample_rate);

  // We need at least 2 cycles of the waveform
  // MD is finicky about minimum sample length.
  // 256 to be safe

  n_cycle = n_cycle * 2;
  while (n_cycle < 256) {
    n_cycle += n_cycle;
  }
  n_cycle += 2;
  //  n_cycle = 8096;
  DEBUG_PRINTLN(F("samples"));
  DEBUG_PRINTLN(n_cycle);
  // 512B worth of buffer
  uint16_t buffer[256];
  uint16_t samples_so_far = 0;

  int16_t largest_sample_so_far = 0;
  // Render each sample
  uint32_t pos = 0;
  bool write_header = false;

  float max_sine_gain = (float)1 / (float)16;

  // Zero crossing detection vars
  bool zero_crossing_found = false;
  int32_t first_zero_crossing = 0;
  int32_t last_zero_crossing = 0;
  int32_t last_sample = 0;
  uint8_t zero_crossing_count = 0;

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
        for (uint8_t h = 1; h <= 16; h++) {
          // osc_sample += sine_gain * sine_osc.get_sample(n,
          // pages[i].get_freq() * (float) h, 0);
          if (pages[i].sine_levels[h - 1] != 0) {
            float sine_gain =
                ((float)pages[i].sine_levels[h - 1] / (float)127) *
                max_sine_gain;

            osc_sample +=
                sine_osc.get_sample(n, pages[i].get_freq() * (float)h, 0) *
                sine_gain;
          }
        }
        osc_sample = (1.00 / wd.pages[i].largest_sine_peak) * osc_sample;
        // DEBUG_PRINTLN(osc_sample);
        break;
      case 2:
       tri_osc.width = pages[i].get_width();
        osc_sample +=
            tri_osc.get_sample(n, pages[i].get_freq(), pages[i].get_phase());
        break;
      case 3:
        pulse_osc.width = pages[i].get_width();
        osc_sample +=
            pulse_osc.get_sample(n, pages[i].get_freq(), pages[i].get_phase());
        break;
      case 4:
        saw_osc.width = pages[i].get_width();
        osc_sample +=
            saw_osc.get_sample(n, pages[i].get_freq(), pages[i].get_phase());
        break;
      case 5:
        osc_sample += usr_osc.get_sample(
            n, pages[i].get_freq(), pages[i].get_phase(), pages[i].usr_values);
        break;
      }
      // Sum oscillator samples together
      sample +=
          dsp.saturate((osc_sample * mixer.get_gain(i)), mixer.get_max_gain());
      // DEBUG_PRINTLN(mixer.get_gain(i));
    }
    // Check for overflow outside of int16_t ranges.
    DEBUG_PRINTLN(sample);
    dsp.saturate(sample, (float)MAX_HEADROOM);

    if (sample > MAX_HEADROOM) {
      sample = MAX_HEADROOM;
    }
    if (sample < -1 * MAX_HEADROOM) {
      sample = -1 * MAX_HEADROOM;
    }
    // DEBUG_PRINTLN(F(" "));
   
    // Need to correctly convert from float to int
    int16_t out_sample;
    if (sample > 0) {
      out_sample = (int16_t)(sample + 0.5);
    } else {
      out_sample = (int16_t)(sample - 0.5);
    }
    DEBUG_PRINTLN(out_sample); 
    buffer[samples_so_far] = out_sample;

    samples_so_far++;

    // Detect largest sample in render

    if ((abs(out_sample) > largest_sample_so_far)) {
      largest_sample_so_far = abs(out_sample);
      DEBUG_PRINTLN(F("large sample found"));
      DEBUG_PRINTLN(largest_sample_so_far);
    }

    // Detect zero crossings

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

    // If buffer overflow approaching write to flash.

    if ((samples_so_far > 255) || (n == n_cycle - 1)) {
      DEBUG_PRINTLN(F("let's write"));
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

  float normalize_gain = ((float)(MAX_HEADROOM / (float)largest_sample_so_far));
  DEBUG_PRINTLN(F("gain:"));
  DEBUG_PRINTLN(largest_sample_so_far);
  DEBUG_PRINTLN(normalize_gain);
  wav_file.file.sync();
  wav_file.apply_gain(normalize_gain);
  write_header = true;
  if (!wav_file.close(write_header)) {
    DEBUG_PRINTLN(F("could not close"));
  }
  DEBUG_PRINTLN(F("wave stats:"));
  DEBUG_PRINTLN(n_cycle);
  DEBUG_PRINTLN(pos);
  DEBUG_PRINTLN(wav_file.header.subchunk2Size);
  DEBUG_PRINTLN(F("zero crossings"));
  DEBUG_PRINTLN(first_zero_crossing);
  DEBUG_PRINTLN(last_zero_crossing);
  DEBUG_PRINTLN(n_cycle - 3);
  //  last_zero_crossing = n_cycle - 3;
  DEBUG_PRINTLN(n_cycle);
  loop_start = first_zero_crossing;
  loop_end = last_zero_crossing;
  // first_zero_crossing = 0;
}
bool WavDesigner::send() {
    return midi_sds.sendWav("render.wav", mixer.enc4.cur, SDS_LOOP_FORWARD, loop_start,
                          loop_end, true);
}

WavDesigner wd;
