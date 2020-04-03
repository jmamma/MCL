#include "MCL.h"



void WavEditPage::setup() {
#ifdef OLED_DISPLAY
  classic_display = false;
#endif
}


void WavEditPage::init() {
  DEBUG_PRINTLN("seq extstep init");
  wd.last_page = this;
  create_chars_mixer();
  // md_exploit.on();
  note_interface.state = true;
#ifdef OLED_DISPLAY
  oled_display.clearDisplay();
#endif
}

void WavEditPage::cleanup() { DEBUG_PRINT_FN(); }
bool WavEditPage::handleEvent(gui_event_t *event) {
  if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
    return true;
  }

  return false;
}

void WavEditPage::loop() {
}
void WavEditPage::display() {
  // oled_display.clearDisplay();
  if (!classic_display) {
#ifdef OLED_DISPLAY
    oled_display.fillRect(0, 0, 64, 32, BLACK);
#endif
  }
   draw_wav();
   oled_display.display();
}

void WavEditPage::draw_wav(uint8_t wav_type) {
#ifdef OLED_DISPLAY
  uint8_t x = 64;
  uint8_t y = 0;
  uint8_t h = 30;
  uint8_t w = 128 - x;

  float sample;
  float max_sine_gain = (float)1 / (float)16;
  uint8_t n = sample_number;
  // for (uint8_t n = 0; n < 128 - x; n++) {

  oled_display.fillRect(n + x, 0, scanline_width, 32, BLACK);
 
 for (uint8_t n = sample_number; n < sample_number + scanline_width; n++) {
    //  if ((scanline_width < w) && (wav_type > 0)) {
    //  oled_display.drawLine(n + x, 0, n + x, 32, BLACK);
    // }
    switch (wav_type) {
    case SIN_OSC:
      sample = 0;
      for (uint8_t f = 1; f <= 16; f++) {
        if (sine_levels[f - 1] != 0) {
          float sine_gain = ((float)sine_levels[f - 1] / (float)127);
          sample += sine_osc.get_sample((uint32_t)n, 1 * (float)f, 0) *
                    sine_gain * max_sine_gain;
        }
      }
      sample = (1.00 / largest_sine_peak) * sample;
      break;
    case USR_OSC:
      sample = usr_osc.get_sample((uint32_t)n, 1, 0, usr_values);
      break;
    case TRI_OSC:
      sample = tri_osc.get_sample((uint32_t)n, 1, 0);
      break;
    case PUL_OSC:
      sample = pul_osc.get_sample((uint32_t)n, 1, 0);
      break;
    case SAW_OSC:
      sample = saw_osc.get_sample((uint32_t)n, 1, 0);
      break;
    }
    uint8_t pixel_y = (uint8_t)((sample * ((float)h / 2.00)) + (h / 2) + y);

    if (wav_type != 0) {
      oled_display.drawPixel(x + n, pixel_y, WHITE);
    }
    // }
    // uint8_t i = n;
    // for (i = x; i < 128; i++) {
    if (n % 2 == 0) {
      oled_display.drawPixel(n + x, (h / 2) + y, WHITE);
    }
    // }
  }
  sample_number += scanline_width;

  if (sample_number > 128 - x) {
    sample_number = 0;
  }
#endif
}

void WavEditPage::draw_usr() {
  uint8_t scaled_val1;
  uint8_t h = 30;
  float sample;
  uint8_t w = 128 - 64;
  uint8_t y = 0;
  UsrWavEdit usr_osc(w);

#ifndef OLED_DISPLAY
  GUI.setLine(GUI.LINE2);
  char str[17] = "                ";
#endif
  for (uint8_t i = 0; i < 16; i++) {
    sample = usr_osc.get_sample((uint32_t)i * 4, 1, 0, usr_values);

#ifdef OLED_DISPLAY

    uint8_t pixel_y = (uint8_t)((sample * ((float)h / 2.00)) + (h / 2) + y);
    if (note_interface.notes[i] == 1) {

      // oled_display.fillRect(63 + i * 4, 0, 3, 32, BLACK);
      oled_display.drawRect(63 + i * 4, pixel_y - 1, 3, 3, WHITE);
    }
#else
    int scaled_level = (int)((float)(sample + 1.0) * .5 * 7.00);
    if (scaled_level == 7) {
      str[i] = (char)(255);
    } else if (scaled_level > 0) {
      str[i] = (char)(scaled_level + 2);
    }
#endif
  }
#ifndef OLED_DISPLAY
  GUI.put_string_at(0, str);
#endif
}

void WavEditPage::draw_levels() {
#ifndef OLED_DISPLAY
  GUI.setLine(GUI.LINE2);
#endif
  uint8_t scaled_level;
  char str[17] = "                ";
  uint8_t x = 64;
  uint8_t w = 128 - x;
  UsrWavEdit usr_osc(w);

  for (int i = 0; i < 16; i++) {
#ifdef OLED_DISPLAY

    scaled_level = (uint8_t)(((float)sine_levels[i] / (float)127) * 15);
    if (note_interface.notes[i] == 1) {
      oled_display.fillRect(0 + i * 4, 12 + (15 - scaled_level), 3,
                            scaled_level + 1, WHITE);
    } else {
      oled_display.drawRect(0 + i * 4, 12 + (15 - scaled_level), 3,
                            scaled_level + 1, WHITE);
    }

#else

    scaled_level = (int)(((float)sine_levels[i] / (float)127) * 7);
    if (scaled_level == 7) {
      str[i] = (char)(255);
    } else if (scaled_level > 0) {
      str[i] = (char)(scaled_level + 2);
    }

#endif
  }
#ifndef OLED_DISPLAY
  GUI.put_string_at(0, str);
#endif
}

float WavEditPage::get_phase() { return 0; }
float WavEditPage::get_width() { return enc3.cur / 128.00; }
uint8_t WavEditPage::get_osc_type() { return osc_waveform; }
