#include "MCL.h"
#include "OscPage.h"

void osc_mod_handler(Encoder *enc) {}

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
  float n = enc2.cur - 64;
  float fn = fzero * pow(a, n);
  float fout = fn * pow(2, (float)(enc3.cur - 100) / (float)1200);
  return fout;
}
void OscPage::init() {
  DEBUG_PRINTLN("seq extstep init");
  create_chars_mixer();
  md_exploit.on();
  note_interface.state = true;
}

void OscPage::cleanup() {
  md_exploit.off();
  DEBUG_PRINT_FN();
}
bool OscPage::handleEvent(gui_event_t *event) {
  if (BUTTON_PRESSED(Buttons.BUTTON3)) {
    wd.render();
    wd.send();
    return true;
  }

  if (BUTTON_PRESSED(Buttons.ENCODER1)) {
    GUI.setPage(&(wd.pages[0]));

    return true;
  }
  if (BUTTON_PRESSED(Buttons.ENCODER2)) {
    GUI.setPage(&(wd.pages[1]));

    return true;
  }
  if (BUTTON_PRESSED(Buttons.ENCODER3)) {
    GUI.setPage(&(wd.pages[2]));

    return true;
  }
  if (BUTTON_PRESSED(Buttons.ENCODER4)) {

    GUI.setPage(&(wd.mixer));

    return true;
  }

  return false;
}

void OscPage::calc_largest_sine_peak() {
  float max_sine_gain = ((float)1 / (float)16);
  largest_sine_peak = 0;
  for (uint8_t f = 0; f < 16; f++) {
    largest_sine_peak +=
        max_sine_gain * ((float)sine_levels[f - 1] / (float)127);
  }
}
void OscPage::loop() {
  MCLEncoder *enc_ = &enc4;
  int dir = 0;
  uint8_t newval;
  calc_largest_sine_peak();
  for (int i = 0; i < 16; i++) {
    if (note_interface.notes[i] == 1) {
      if ((enc_->getValue() - enc_->old) < 0) {
        dir = -2;
      }
      if ((enc_->getValue() - enc_->old) > 0) {
        dir = 2;
      }
      if (enc1.cur == SIN_OSC) {
        newval = sine_levels[i] + dir;
        if ((newval <= 127) && (newval >= 0)) {
          sine_levels[i] = newval;
        }
      }
      if (enc1.cur == USR_OSC) {
        newval = usr_values[i] + dir;

        if ((newval <= 127) && (newval >= 0)) {
          usr_values[i] = newval;
        }
      }
    }
  }
  enc_->cur = 64 + dir;
  enc_->old = 64;
}
void OscPage::display() {
  oled_display.clearDisplay();
  GUI.setLine(GUI.LINE1);

  MusicalNotes number_to_note;

  GUI.put_string_at(0, "                ");
  switch (enc1.cur) {
  case 0:
    GUI.put_string_at_not(0, "--");
    break;
  case SIN_OSC:
    GUI.put_string_at_not(0, "SIN");
    draw_levels();
    draw_wav(SIN_OSC);
    break;
  case TRI_OSC:
    GUI.put_string_at_not(0, "TRI");
    draw_wav(TRI_OSC);
    break;
  case PUL_OSC:
    GUI.put_string_at_not(0, "PUL");
    draw_wav(PUL_OSC);
    break;
  case SAW_OSC:
    GUI.put_string_at_not(0, "SAW");
    draw_wav(SAW_OSC);
    break;
  case USR_OSC:
    GUI.put_string_at_not(0, "USR");
    draw_wav(USR_OSC);
    draw_usr();
    break;
  }
  uint8_t s = enc2.cur - 7;
  uint8_t note = s - (floor(s / 12) * 12);

  GUI.setLine(GUI.LINE1);
  GUI.put_string_at_not(4, number_to_note.notes_upper[note]);
  GUI.put_value_at1(6, floor(s / 12));

  if (enc3.cur < 0) {
    GUI.put_string_at(7, "-");
  } else {
    GUI.put_string_at(7, "+");
  }
  GUI.put_value_at2(8, enc3.cur);
  GUI.put_string_at(10, "\0");
  //  GUI.put_string_at(0, my_str);

  LCD.goLine(0);
  LCD.puts(GUI.lines[0].data);
  GUI.lines[0].changed = false;
  oled_display.display();
}
void OscPage::draw_wav(uint8_t wav_type) {
  uint8_t x = 64;
  uint8_t y = 0;
  uint8_t h = 30;
  uint8_t w = 128 - x;
  TriOsc tri_osc(w);
  PulseOsc pul_osc(w);
  SawOsc saw_osc(w);
  SineOsc sine_osc(w);
  UsrOsc usr_osc(w);
  float sample;
  float max_sine_gain = (float)1 / (float)16;

  for (uint8_t n = 0; n < 128 - x; n++) {
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
    oled_display.drawPixel(x + n, pixel_y, WHITE);
  }
  uint8_t i = 0;
  for (i = x; i < 128; i++) {
    if (i % 2 == 0) {
      oled_display.drawPixel(i, (h / 2) + y, WHITE);
    }
  }
}

void OscPage::draw_usr() {
  uint8_t scaled_val1;
  uint8_t h = 30;
  float sample;
  uint8_t w = 128 - 64;
  uint8_t y = 0;
  UsrOsc usr_osc(w);

  for (uint8_t i = 0; i < 16; i++) {

    sample = usr_osc.get_sample((uint32_t)i * 4, 1, 0, usr_values);

    uint8_t pixel_y = (uint8_t)((sample * ((float)h / 2.00)) + (h / 2) + y);
    if (note_interface.notes[i] == 1) {
      oled_display.drawRect(63 + i * 4, pixel_y - 1, 3, 3, WHITE);
    }
  }
  //  GUI.put_string_at(0, str);
}

void OscPage::draw_levels() {
  GUI.setLine(GUI.LINE2);
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

    } else if (scaled_xlevel > 0) {
      str[i] = (char)(scaled_level + 2);
    }

#endif
  }
  GUI.put_string_at(0, str);
}

float OscPage::get_phase() { return enc3.cur; }
uint8_t OscPage::get_osc_type() { return enc1.cur; }
