#include "WavDesigner.h"
#include "Wav.h"
#include "MCL.h"
#include "Osc.h"
#include "SampleBrowserPage.h"
#include "GUI.h"
#include "MIDISds.h"
#include "DSP.h"
#include "MCLStrings.h"

#ifdef WAV_DESIGNER
#define WAV_NAME "WAVE.wav"

#ifndef __AVR__
extern const char *c_wav_root;
static bool wavdesigner_chdir_wav_root() {
  char path[64];
  return SD.chdir(mcl_sd.full_path(c_wav_root, path, sizeof(path)));
}
#endif

void WavDesigner::prompt_send() {
  //  if (mcl_gui.wait_for_confirm("Send Sample", "Overwrite sample slot?")) {
  oled_display.textbox_P(mclstr_render);
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
      mcl.loop();
    }
  }
  DEBUG_PRINTLN("cleaning up");
  sample_browser.file.close();
  mcl.setPage(WD_MIXER_PAGE);
  oled_display.textbox_P(mclstr_sending);
  oled_display.display();
  wd.send();
}

bool WavDesigner::render() {
  DEBUG_PRINT_FN();
  float sample_rate = 44100;
  Wav wav_file;

#ifndef __AVR__
  if (!wavdesigner_chdir_wav_root()) {
    return false;
  }
#endif

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

  // The OscPage pitch range keeps a rendered waveform well under 65535 samples.
  uint16_t n_cycle = (uint16_t)(sample_rate / fund_freq + 0.5f);
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

  uint16_t largest_sample_so_far = 0;
  // Render each sample
  uint16_t pos = 0;

  // Zero crossing detection vars
  bool zero_crossing_found = false;
  uint16_t first_zero_crossing = 0;
  uint16_t last_zero_crossing = 0;
  int16_t last_sample = 0;
  bool update_loop_end = false;

  for (uint16_t n = 0; n < n_cycle; n++) {
    float sample = 0;

    // Render each oscillator
    for (uint8_t i = 0; i < 3; i++) {
      OscPage* page = &pages[i];  // Cache pointer to current page
      uint8_t osc_type = page->get_osc_type();
      float osc_sample = 0;
      if (osc_type != 0) {
        osc_sample =
            render_osc_sample(osc_type, page->get_width(), page->sine_levels,
                              page->usr_values, page->largest_sine_peak, n,
                              page->get_freq(), sine_osc, tri_osc, pulse_osc,
                              saw_osc, usr_osc);
      }
      // Sum oscillator samples together
      sample += osc_sample * mixer.get_gain(i);
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
      out_sample = (int16_t)(sample + 0.5f);
    } else {
      out_sample = (int16_t)(sample - 0.5f);
    }
    DEBUG_PRINTLN(out_sample);
    buffer[samples_so_far] = out_sample;

    samples_so_far++;

    // Detect largest sample in render

    uint16_t sample_magnitude =
        out_sample < 0 ? (uint16_t)-out_sample : (uint16_t)out_sample;
    if (sample_magnitude > largest_sample_so_far) {
      largest_sample_so_far = sample_magnitude;
      DEBUG_PRINTLN(F("large sample found"));
      DEBUG_PRINTLN(largest_sample_so_far);
    }

    // Detect zero crossings

    if (((last_sample < 0) && (out_sample > 0)) ||
        ((last_sample > 0) && (out_sample < 0)) || (out_sample == 0)) {
      if (!zero_crossing_found) {
        first_zero_crossing = n;
        zero_crossing_found = true;
      }
      update_loop_end = !update_loop_end;
      if (update_loop_end) {
        last_zero_crossing = n - 1;
      }
    }

    last_sample = out_sample;

    // If buffer overflow approaching write to flash.

    if ((samples_so_far > 255) || (n == n_cycle - 1)) {
      DEBUG_PRINTLN(F("let's write"));
      DEBUG_PRINTLN(samples_so_far);
      if (!wav_file.write_samples(buffer, samples_so_far, pos, 0, false)) {
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

  DEBUG_PRINTLN("gain:");
  DEBUG_PRINTLN(largest_sample_so_far);
  wav_file.header.smpl.init(wav_file.header.fmt, SDS_LOOP_FORWARD, loop_start,
                            loop_end);
  wav_file.file.sync();
  if (largest_sample_so_far > 0) {
#if defined(__AVR__)
    if (!wav_file.normalize16_mono(MAX_HEADROOM, largest_sample_so_far)) {
      return false;
    }
#else
    float normalize_gain = ((float)(MAX_HEADROOM / (float)largest_sample_so_far));
    DEBUG_PRINTLN(normalize_gain);
    if (!wav_file.apply_gain(normalize_gain)) {
      return false;
    }
#endif
  }
  if (!wav_file.close(true)) {
    DEBUG_PRINTLN(F("could not close"));
    return false;
  }
  return true;
}

bool WavDesigner::send() {
#ifndef __AVR__
  if (!wavdesigner_chdir_wav_root()) {
    return false;
  }
#endif
  return midi_sds.sendWav(WAV_NAME, WAV_NAME, mixer.enc4.cur, false);
}

WavDesigner wd;

#endif
