#include "OscMixerPage.h"
#include "WavDesigner.h"
#include "MCLGUI.h"
#include "Osc.h"
#include "DSP.h"

#ifdef WAV_DESIGNER

void OscMixerPage::init() {
  WavDesignerPage::init();
  key_interface.off();
  wavdesign_menu_page.menu.enable_entry(1, false);
  wavdesign_menu_page.menu.enable_entry(2, true);
}
void OscMixerPage::cleanup() {}

bool OscMixerPage::handleEvent(gui_event_t *event) {
  if (WavDesignerPage::handleEvent(event)) {
    return true;
  }
  /*
  if (EVENT_PRESSED(event, Buttons.BUTTON4)) {
    MD.preview_sample(encoders[3]->cur + 1);
    return true;
  }
  */

  return false;
}

void OscMixerPage::loop() { WavDesignerPage::loop(); }
void OscMixerPage::display() {
  if (show_menu) {
    WavDesignerPage::display();
  } else {
    oled_display.setFont();
    oled_display.setCursor(0, 0);
    oled_display.fillRect(0, 0, 64, 32, BLACK);

    mcl_print_P(mclstr_osc_mixer);
    draw_levels();
    scanline_width = 4;
    draw_wav();
  }
}

float OscMixerPage::get_gain(uint8_t channel) {
  MCLEncoder *enc_ = (MCLEncoder *)(encoders[channel]);
  float max_gain = (float)MAX_HEADROOM / (float)NUM_CHANNELS;
  return ((float)enc_->cur / (float)127) * max_gain;
}

void OscMixerPage::draw_wav() {
  uint8_t x = 64;
  uint8_t y = 0;
  uint8_t h = 30;
  uint8_t w = 128 - x;
  TriOsc tri_osc(w);
  PulseOsc pul_osc(w);
  SawOsc saw_osc(w);
  SineOsc sine_osc(w);
  UsrOsc usr_osc(w);

  // Work out lowest base frequency.
  float fund_freq = 20000;
  uint8_t lowest_osc_freq = 0;
  uint8_t i = 0;
  for (i = 0; i < 3; i++) {
    float freq = wd.pages[i].get_freq();
    if ((wd.pages[i].get_osc_type() > 0) && (freq < fund_freq)) {
      fund_freq = freq;
      lowest_osc_freq = i;
    }
  }
  float freqs[3];
  for (i = 0; i < 3; i++) {
    if (lowest_osc_freq == i) {
      freqs[i] = 1;
    } else {
      freqs[i] = wd.pages[i].get_freq() / fund_freq;
    }
  }
  // float buffer[w];
  oled_display.fillRect(sample_number + x, 0, scanline_width, 32, BLACK);
  uint8_t n_end = sample_number + scanline_width;
  for (uint8_t n = sample_number; n < n_end; n++) {
    float sample = 0;
    // Render each oscillator
    for (i = 0; i < 3; i++) {
      float osc_sample =
          render_osc_sample(wd.pages[i].get_osc_type(), wd.pages[i].get_width(),
                            wd.pages[i].sine_levels, wd.pages[i].usr_values,
                            wd.pages[i].sine_level_sum, n, freqs[i],
                            sine_osc, tri_osc, pul_osc, saw_osc, usr_osc);
      // Sum oscillator samples together
      sample += osc_sample * wd.mixer.get_gain(i);
      // DEBUG_PRINTLN(mixer.get_gain(i));
    }
    // Check for overflow outside of int16_t ranges.
    //dsp.saturate(sample, (float)MAX_HEADROOM);
    sample = sample / MAX_HEADROOM;

    //  buffer[n] = sample;
    //  if (abs(buffer[n]) > largest_sample_so_far) {
    //  largest_sample_so_far = abs(buffer[n]);
    //  }
    uint8_t pixel_y = (uint8_t)(((sample) * (float)(h / 2)) + (h / 2));
    oled_display.drawPixel(n + x, pixel_y + y, WHITE);
    // oled_display.drawPixel(i + x, buffer[i] + normalize_inc + y, WHITE);
    if (n % 2 == 0) {
      oled_display.drawPixel(n + x, (h / 2) + y, WHITE);
    }
  }

  sample_number += scanline_width;

  if (sample_number > 128 - x) {
    sample_number = 0;
  }

  // for (i = 0; i < w; i++) {

  // }
}

void OscMixerPage::draw_levels() {
  uint8_t scaled_level;
  for (uint8_t i = 0; i < 3; i++) {

    scaled_level = ((uint16_t)encoders[i]->cur * 15) / 127;
    oled_display.fillRect(0 + i * 6, 12 + (15 - scaled_level), 4,
                          scaled_level + 1, WHITE);
  }
}
#endif
