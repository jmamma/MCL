#include "MCL_impl.h"

#ifdef WAV_DESIGNER
#define WAV_NAME "WAVE.wav"

void WavDesigner::prompt_send() {
  //  if (mcl_gui.wait_for_confirm("Send Sample", "Overwrite sample slot?")) {
  oled_display.textbox("Render", "");
  oled_display.display();
  //Order of statements important for directory switching.
  mcl.pushPage(SAMPLE_BROWSER);
  sample_browser.show_samplemgr = true;
  sample_browser.pending_action = PA_SELECT;
  sample_browser.setup();
  wd.render();
  sample_browser.init(true);
  if (sample_browser.file.open(WAV_NAME, O_READ)) {
    while (mcl.currentPage() == SAMPLE_BROWSER &&
           sample_browser.pending_action == PA_SELECT && sample_browser.show_samplemgr) {
      GUI.loop();
    }
  }
  DEBUG_PRINTLN("cleaning up");
  sample_browser.file.close();
  mcl.setPage(WD_MIXER_PAGE);
  // oled_display.textbox("Sending..","");
  //
  // oled_display.display();
  // wd.send();
  // }
}

bool WavDesigner::render() {
  DEBUG_PRINT_FN();
  float sample_rate = 44100;
  Wav wav_file;

  if (!wav_file.open(WAV_NAME, true, 1, sample_rate, 16, true)) {
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

  uint32_t n_cycle = ((round(sample_rate / fund_freq)));
  DEBUG_PRINTLN("n_cycle 1");
  DEBUG_PRINTLN(n_cycle);
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

  float max_sine_gain = 0.0004921259843f; ; //(float)1 / (float)16) / 127;

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
                ((float)pages[i].sine_levels[h - 1]) * max_sine_gain;

            osc_sample +=
                sine_osc.get_sample(n, pages[i].get_freq() * (float)h) *
                sine_gain;
          }
        }
        if (wd.pages[i].largest_sine_peak == 0) {
          osc_sample = 0;
        } else {
          osc_sample = (1.00 / wd.pages[i].largest_sine_peak) * osc_sample;
        }
        // DEBUG_PRINTLN(osc_sample);
        break;
      case 2:
        tri_osc.width = pages[i].get_width();
        osc_sample += tri_osc.get_sample(n, pages[i].get_freq());
        break;
      case 3:
        pulse_osc.width = pages[i].get_width();
        osc_sample += pulse_osc.get_sample(n, pages[i].get_freq());
        break;
      case 4:
        saw_osc.width = pages[i].get_width();
        osc_sample += saw_osc.get_sample(n, pages[i].get_freq());
        break;
      case 5:
        osc_sample +=
            usr_osc.get_sample(n, pages[i].get_freq(), pages[i].usr_values);
        break;
      }
      // Sum oscillator samples together
      sample +=
          dsp.saturate((osc_sample * mixer.get_gain(i)), mixer.get_max_gain());
      // DEBUG_PRINTLN(mixer.get_gain(i));
    }
    // Check for overflow outside of int16_t ranges.
    DEBUG_PRINTLN(sample);
    //dsp.saturate(sample, (float)MAX_HEADROOM);

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

  DEBUG_PRINTLN(F("wave stats:"));
  DEBUG_PRINTLN(n_cycle);
  DEBUG_PRINTLN(pos);
  DEBUG_PRINTLN(wav_file.header.data.chunk_size);
  DEBUG_PRINTLN(F("zero crossings"));
  DEBUG_PRINTLN(first_zero_crossing);
  DEBUG_PRINTLN(last_zero_crossing);
  DEBUG_PRINTLN(n_cycle - 3);
  //  last_zero_crossing = n_cycle - 3;
  DEBUG_PRINTLN(n_cycle);
  loop_start = first_zero_crossing;
  loop_end = last_zero_crossing;

  // first_zero_crossing = 0;
  // Normalise wav

  float normalize_gain = ((float)(MAX_HEADROOM / (float)largest_sample_so_far));
  DEBUG_PRINTLN("gain:");
  DEBUG_PRINTLN(largest_sample_so_far);
  DEBUG_PRINTLN(normalize_gain);
  wav_file.header.smpl.init(wav_file.header.fmt, SDS_LOOP_FORWARD, loop_start,
                            loop_end);
  wav_file.file.sync();
  wav_file.apply_gain(normalize_gain);
  write_header = true;
  if (!wav_file.close(write_header)) {
    DEBUG_PRINTLN(F("could not close"));
  }
}

bool WavDesigner::send() {
  return midi_sds.sendWav(WAV_NAME, WAV_NAME, mixer.enc4.cur, false);
}

WavDesigner wd;

#endif
