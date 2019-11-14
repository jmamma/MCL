#include "MCL.h"
#include "Osc.h"
#include "OscPage.h"
#include "WavDesigner.h"

void osc_mod_handler(Encoder *enc) {}

uint32_t OscPage::exploit_delay_clock = 0;

void OscPage::setup() {
#ifdef OLED_DISPLAY
  classic_display = false;
#endif
  for (uint8_t i = 0; i < 16; i++) {
    usr_values[i] = random(127);
  }
}

float OscPage::get_freq() {
  float fzero = 440;
  float a = pow(2.00, 1.00 / 12.00);
  float n = enc1.cur - 64;
  float fn = fzero * pow(a, n);
  float fout = fn * pow(2, (float)(enc2.cur - 100) / (float)1200);
  return fout;
}
void OscPage::init() {
  DEBUG_PRINTLN("seq extstep init");
  wd.last_page = this;
  create_chars_mixer();
  // md_exploit.on();
  note_interface.state = true;
#ifdef OLED_DISPLAY
  oled_display.clearDisplay();
#endif
}

void OscPage::cleanup() { DEBUG_PRINT_FN(); }
bool OscPage::handleEvent(gui_event_t *event) {
  if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
    wd.load_next_page(id);
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
    GUI.setPage(&page_select_page);
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
    osc_waveform++;
    if (osc_waveform > 5) {
      osc_waveform = 0;
    }
    return true;
  }

  return false;
}

void OscPage::calc_largest_sine_peak() {
  float max_sine_gain = ((float)1 / (float)16);
  largest_sine_peak = 0;
  for (uint8_t f = 0; f < 16; f++) {
    largest_sine_peak += max_sine_gain * ((float)sine_levels[f] / (float)127);
  }
}
void OscPage::loop() {
  MCLEncoder *enc_ = &enc4;
  // largest_sine_peak = 1.0 / 16.00;
  if (encoders[0]->hasChanged()) {
    show_freq = false;
  }
  if (encoders[1]->hasChanged()) {
    show_freq = true;
  }

  calc_largest_sine_peak();
  int dir = 0;
  int16_t newval;
  int8_t diff = enc_->cur - enc_->old;
  for (int i = 0; i < 16; i++) {
    if (note_interface.notes[i] == 1) {
      if (osc_waveform == SIN_OSC) {
        newval = sine_levels[i] + diff;

        if (newval < 0) {
          newval = 0;
        }
        if (newval > 127) {
          newval = 127;
        }

        sine_levels[i] = newval;
      }
      if (osc_waveform == USR_OSC) {
        newval = usr_values[i] + diff;
        if (newval < 0) {
          newval = 0;
        }
        if (newval > 127) {
          newval = 127;
        }
        usr_values[i] = newval;
      }
    }
  }
  enc_->cur = 64 + diff;
  enc_->old = 64;
  if ((osc_waveform == SIN_OSC) || (osc_waveform == USR_OSC)) {
    if (!md_exploit.state) {
      md_exploit.on();
      note_interface.state = true;
    }
  }

  else {
    if (md_exploit.state) {
      md_exploit.off();
    }
  }
}
void OscPage::display() {
  // oled_display.clearDisplay();
  if (!classic_display) {
#ifdef OLED_DISPLAY
    oled_display.fillRect(0, 0, 64, 32, BLACK);
#endif
  } else {
    GUI.setLine(GUI.LINE2);
    GUI.put_string_at(0, "                ");
  }
  GUI.setLine(GUI.LINE1);

  MusicalNotes number_to_note;

  GUI.put_string_at(0, "                ");
  scanline_width = 64;

  uint8_t c = 0;
  uint8_t i = 0;
  switch (osc_waveform) {
  case 0:
    draw_wav(0);
    GUI.put_string_at_not(0, "--");
    break;
  case SIN_OSC:
    GUI.put_string_at_not(0, "SIN");
    draw_levels();
    for (i = 0; i < 16; i++) {
      if (sine_levels[i] > 0) {
        c++;
      }
    }
    scanline_width = 64 / c;
    draw_wav(SIN_OSC);
    break;
  case TRI_OSC:
    GUI.put_string_at_not(0, "TRI");
    sample_number = 0;
    draw_wav(TRI_OSC);
    break;
  case PUL_OSC:
    GUI.put_string_at_not(0, "PUL");
    sample_number = 0;
    draw_wav(PUL_OSC);
    break;
  case SAW_OSC:
    GUI.put_string_at_not(0, "SAW");
    sample_number = 0;
    draw_wav(SAW_OSC);
    break;
  case USR_OSC:
    GUI.put_string_at_not(0, "USR");
    draw_wav(USR_OSC);
    sample_number = 0;
    draw_usr();
    break;
  }

  GUI.setLine(GUI.LINE1);
  if (show_freq) {
    float freq = get_freq();
    float upper = floor(freq / 1000);
    float lower = floor(freq - (upper * 1000));
    GUI.put_value_at1(4, (uint8_t)upper);
    GUI.put_value_at(5, (int16_t)lower);
    GUI.put_string_at(8, "Hz");
    // GUI.printf_at(6, "%f", freq);
  } else {
    uint8_t s = enc1.cur - 8;
    uint8_t note = s - (floor(s / 12) * 12);
#ifndef OLED_DISPLAY
    GUI.put_value_at2(5, enc3.cur);
    GUI.put_string_at(7, "%");
#endif
    GUI.put_string_at_not(4, number_to_note.notes_upper[note]);
    GUI.put_value_at1(6, floor(s / 12));
    if (enc3.cur < 0) {
      GUI.put_string_at(7, "-");
    } else {
      GUI.put_string_at(7, "+");
    }
    GUI.put_value_at2(8, enc2.cur);
  }
  GUI.put_string_at(10, "\0");
  //  GUI.put_string_at(0, my_str);
#ifdef OLED_DISPLAY
  if (!classic_display) {
    LCD.goLine(0);
    LCD.puts(GUI.lines[0].data);
    GUI.lines[0].changed = false;
  }
#endif
#ifdef OLED_DISPLAY
  oled_display.display();
#endif
}
void OscPage::draw_wav(uint8_t wav_type) {
#ifdef OLED_DISPLAY
  uint8_t x = 64;
  uint8_t y = 0;
  uint8_t h = 30;
  uint8_t w = 128 - x;
  float osc_width = get_width();
  TriOsc tri_osc(w, osc_width);
  PulseOsc pul_osc(w, osc_width);
  SawOsc saw_osc(w, osc_width);
  SineOsc sine_osc(w);
  UsrOsc usr_osc(w);
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

void OscPage::draw_usr() {
  uint8_t scaled_val1;
  uint8_t h = 30;
  float sample;
  uint8_t w = 128 - 64;
  uint8_t y = 0;
  UsrOsc usr_osc(w);

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

void OscPage::draw_levels() {
#ifndef OLED_DISPLAY
  GUI.setLine(GUI.LINE2);
#endif
  uint8_t scaled_level;
  char str[17] = "                ";
  uint8_t x = 64;
  uint8_t w = 128 - x;
  UsrOsc usr_osc(w);

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

float OscPage::get_phase() { return 0; }
float OscPage::get_width() { return enc3.cur / 128.00; }
uint8_t OscPage::get_osc_type() { return osc_waveform; }
