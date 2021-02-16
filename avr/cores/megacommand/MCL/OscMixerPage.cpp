#include "MCL_impl.h"

void OscMixerPage::setup() {}

void OscMixerPage::init() {
  WavDesignerPage::init();
  trig_interface.off();
  wd.last_page = this;
  create_chars_mixer();
  classic_display = false;
}
void OscMixerPage::cleanup() {}
bool OscMixerPage::handleEvent(gui_event_t *event) {
  if (WavDesignerPage::handleEvent(event)) {
  return true;
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON4)) {
    MD.preview_sample(encoders[3]->cur + 1);
    return true;
  }

  return false;
}

void OscMixerPage::loop() { WavDesignerPage::loop(); }
void OscMixerPage::display() {
// oled_display.clearDisplay();
  auto oldfont = oled_display.getFont();
  if (show_menu) {
    WavDesignerPage::display();
  }
  else {
  oled_display.setFont();
  oled_display.setCursor(0, 0);
  oled_display.fillRect(0, 0, 64, 32, BLACK);
  GUI.setLine(GUI.LINE1);

  oled_display.print("SLOT: ");

  oled_display.print(enc4.cur);
  draw_levels();
  scanline_width = 4;
  draw_wav();
  }
  oled_display.display();
  oled_display.setFont(oldfont);
}

float OscMixerPage::get_max_gain() {
  float max_gain = (float)MAX_HEADROOM / (num_of_channels);
  return max_gain;
}
float OscMixerPage::get_gain(uint8_t channel) {
  MCLEncoder *enc_ = (MCLEncoder *)(encoders[channel]);
  float max_gain = (float)MAX_HEADROOM / (num_of_channels);
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
  float max_sine_gain = (float)1 / (float)16;

  // Work out lowest base frequency.
  float fund_freq = 20000;
  uint8_t lowest_osc_freq = 0;
  uint8_t i = 0;
  for (i = 0; i < 3; i++) {
    if ((wd.pages[i].get_osc_type() > 0) &&
        (wd.pages[i].get_freq() < fund_freq)) {
      fund_freq = wd.pages[i].get_freq();
      lowest_osc_freq = i;
    }
  }
  float freqs[3];
  for (i = 0; i < 3; i++) {
    if (lowest_osc_freq == i) {
      freqs[i] = 1;
    } else {
      freqs[i] = wd.pages[i].get_freq() / wd.pages[lowest_osc_freq].get_freq();
    }
  }
  // float buffer[w];
#ifdef OLED_DISPLAY
  oled_display.fillRect(sample_number + x, 0, scanline_width, 32, BLACK);
#endif
  float largest_sample_so_far;
  for (uint32_t n = sample_number; n < scanline_width + sample_number; n++) {
    float sample = 0;
    // Render each oscillator
    for (i = 0; i < 3; i++) {
      float osc_sample = 0;
      switch (wd.pages[i].get_osc_type()) {
      case 0:
        osc_sample += 0;
        break;
      // Sine wave with 16 overtones.
      case 1:
        for (uint8_t f = 1; f <= 16; f++) {
          // osc_sample += sine_gain * sine_osc.get_sample(n,
          // wd.pages[i].get_freq() * (float) h, 0);
          if (wd.pages[i].sine_levels[f - 1] != 0) {

            float sine_gain =
                ((float)wd.pages[i].sine_levels[f - 1] / (float)127) *
                max_sine_gain;

            osc_sample +=
                sine_osc.get_sample(n, freqs[i] * (float)f, 0) * sine_gain;
          }
        }
        osc_sample = (1.00 / wd.pages[i].largest_sine_peak) * osc_sample;
        break;
      case 2:
        tri_osc.width = wd.pages[i].get_width();
        osc_sample += tri_osc.get_sample(n, freqs[i], wd.pages[i].get_phase());
        break;
      case 3:
        pul_osc.width = wd.pages[i].get_width();
        osc_sample += pul_osc.get_sample(n, freqs[i], wd.pages[i].get_phase());
        break;
      case 4:
        saw_osc.width = wd.pages[i].get_width();
        osc_sample += saw_osc.get_sample(n, freqs[i], wd.pages[i].get_phase());
        break;
      case 5:
        osc_sample += usr_osc.get_sample(n, freqs[i], wd.pages[i].get_phase(),
                                         wd.pages[i].usr_values);
        break;
      }
      // Sum oscillator samples together
      sample += dsp.saturate((osc_sample * wd.mixer.get_gain(i)),
                             wd.mixer.get_max_gain());
      // DEBUG_PRINTLN(mixer.get_gain(i));
    }
    // Check for overflow outside of int16_t ranges.
    dsp.saturate(sample, (float)MAX_HEADROOM);
    sample = sample / MAX_HEADROOM;

    //  buffer[n] = sample;
    //  if (abs(buffer[n]) > largest_sample_so_far) {
    //  largest_sample_so_far = abs(buffer[n]);
    //  }
    uint8_t pixel_y = (uint8_t)(((sample) * (float)(h / 2)) + (h / 2));
#ifdef OLED_DISPLAY
    oled_display.drawPixel(n + x, pixel_y + y, WHITE);
    // oled_display.drawPixel(i + x, buffer[i] + normalize_inc + y, WHITE);
    if (n % 2 == 0) {
      oled_display.drawPixel(n + x, (h / 2) + y, WHITE);
    }
#endif
  }

  sample_number += scanline_width;

  if (sample_number > 128 - x) {
    sample_number = 0;
  }

  // for (i = 0; i < w; i++) {

  // }
}

void OscMixerPage::draw_levels() {
  GUI.setLine(GUI.LINE2);
  uint8_t scaled_level;
  char str[17] = "                ";
  for (int i = 0; i < 3; i++) {
#ifdef OLED_DISPLAY

    scaled_level = (uint8_t)(((float)encoders[i]->cur / (float)127) * 15);
    oled_display.fillRect(0 + i * 6, 12 + (15 - scaled_level), 4,
                          scaled_level + 1, WHITE);
#else

    scaled_level = (int)(((float)encoders[i]->cur / (float)127) * 7);
    if (scaled_level == 7) {
      str[i] = (char)(255);

    } else if (scaled_level > 0) {
      str[i] = (char)(scaled_level + 2);
    }

#endif
  }
  GUI.put_string_at(0, str);
}
