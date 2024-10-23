#include "MCL_impl.h"

#ifdef WAV_DESIGNER

void osc_mod_handler(EncoderParent *enc) {}

uint32_t OscPage::exploit_delay_clock = 0;

void OscPage::setup() {
  for (uint8_t i = 0; i < 16; i++) {
    usr_values[i] = get_random(127);
  }
}

float OscPage::get_freq() {
  float fzero = 440;
  float a = pow(2.00, 1.00 / 12.00);
  float n = enc1.cur - 64;
  float fn = fzero * pow(a, n);
  float fout = fn * pow(2, (float)(100 - enc2.cur) / (float)1200);
  return fout;
}
void OscPage::init() {
  WavDesignerPage::init();
  wavdesign_menu_page.menu.enable_entry(1, true);
  wavdesign_menu_page.menu.enable_entry(2, false);
  oled_display.clearDisplay();
}

void OscPage::cleanup() { }
bool OscPage::handleEvent(gui_event_t *event) {
  if (WavDesignerPage::handleEvent(event)) {
    return true;
  }
  if (note_interface.is_event(event)) {
    uint8_t mask = event->mask;
    uint8_t port = event->port;
    MidiDevice *device = midi_active_peering.get_device(port);

    uint8_t track = event->source - 128;
    if (device != &MD) {
      return true;
    }
    trig_interface.send_md_leds(TRIGLED_OVERLAY);
  }
  if (EVENT_CMD(event)) {
    uint8_t key = event->source - 64;
    if (event->mask == EVENT_BUTTON_PRESSED) {
        switch (key) {
        case MDX_KEY_NO:
          //  trig_interface.ignoreNextEvent(MDX_KEY_NO);
            show_freq = !show_freq;
          return true;
        }
    }
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
    show_freq = !show_freq;
  }
  return false;
}

void OscPage::calc_largest_sine_peak() {
  float max_sine_gain = 0.0004921259843f; // ((float)1 / (float)16) / 127;
  largest_sine_peak = 0;
  for (uint8_t f = 0; f < 16; f++) {
    largest_sine_peak += max_sine_gain * (float)sine_levels[f];
  }
}

void OscPage::loop() {
  WavDesignerPage::loop();
  MCLEncoder *enc_ = &enc4;
  // largest_sine_peak = 1.0 / 16.00;
  calc_largest_sine_peak();

  if (show_menu == false) {
    if (encoders[0]->hasChanged()) {
      encoders[1]->cur = 0;
      encoders[1]->old = 0;
    }
  }

  int dir = 0;
  int16_t newval;
  int8_t diff = enc_->cur - enc_->old;
  for (uint8_t i = 0; i < 16; i++) {
    if (note_interface.is_note_on(i)) {
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
    if (!trig_interface.state) {
      trig_interface.on();
    }
  }

  else {
    trig_interface.off();
  }
}

const char wave_names[][4] PROGMEM = {"--", "SIN", "TRI", "PUL", "SAW", "USR"};

void OscPage::display() {
  // oled_display.clearDisplay();
  oled_display.fillRect(0, 0, 64, 32, BLACK);

  scanline_width = 64;

  uint8_t c = 1;
  uint8_t i = 0;
  oled_display.setCursor(0, 0);

  switch (osc_waveform) {
  default:
    sample_number = 0;
    break;
  case SIN_OSC:
    draw_levels();
    for (i = 0; i < 16; i++) {
      if (sine_levels[i] > 0) {
        c++;
      }
    }
    scanline_width = 64 / c;
    break;
  }
  char buf1[4];
  draw_wav(osc_waveform);
  strncpy_P(buf1, wave_names[osc_waveform], 4);
  oled_display.print(buf1);
  oled_display.print(F(" "));

  char str[] = "    ";
  if (show_freq) {
    float freq = get_freq();
    oled_display.print((int)freq);
    oled_display.print(F("Hz"));
    // GUI.printf_at(6, "%f", freq);
  } else {
    uint8_t s = enc1.cur - 8;
    uint8_t note = s - (floor(s / 12) * 12);
    oled_display.print(number_to_note.notes_upper[note]);
    oled_display.print((uint8_t)floor(s / 12));
    if (enc2.cur < 0) {
      oled_display.print(F("+"));
    }
    if (enc2.cur != 0) {
      oled_display.print(-1 * enc2.cur);
    }
  }
  //  GUI.put_string_at(0, my_str);
  WavDesignerPage::display();
}
void OscPage::draw_wav(uint8_t wav_type) {
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
  float sample = 0;
  float max_sine_gain = 0.0004921259843f; // (float)1 / (float)16 / 127;
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
          float sine_gain = (float)sine_levels[f - 1] * max_sine_gain;
          sample += sine_osc.get_sample((uint32_t)n, 1 * (float)f) * sine_gain;
        }
      }
      if (largest_sine_peak == 0) {
        sample = 0;
      } else {
        sample = (1.00 / largest_sine_peak) * sample;
      }
      break;
    case USR_OSC:
      sample = usr_osc.get_sample((uint32_t)n, 1, usr_values);
      break;
    case TRI_OSC:
      sample = tri_osc.get_sample((uint32_t)n, 1);
      break;
    case PUL_OSC:
      sample = pul_osc.get_sample((uint32_t)n, 1);
      break;
    case SAW_OSC:
      sample = saw_osc.get_sample((uint32_t)n, 1);
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

  if (sample_number > 127 - x) {
    sample_number = 0;
  }
  if (wav_type == USR_OSC) {
    draw_usr();
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
    sample = usr_osc.get_sample((uint32_t)i * 4, 1, usr_values);

    uint8_t pixel_y = (uint8_t)((sample * ((float)h / 2.00)) + (h / 2) + y);
    if (note_interface.is_note_on(i)) {

      // oled_display.fillRect(63 + i * 4, 0, 3, 32, BLACK);
      oled_display.drawRect(63 + i * 4, pixel_y - 1, 3, 3, WHITE);
    }
  }
}

void OscPage::draw_levels() {
  uint8_t scaled_level;
  uint8_t x = 64;
  uint8_t w = 128 - x;
  UsrOsc usr_osc(w);

  for (uint8_t i = 0; i < 16; i++) {

    scaled_level = (uint8_t)(((float)sine_levels[i] / (float)127) * 15);
    if (note_interface.is_note_on(i)) {
      oled_display.fillRect(0 + i * 4, 12 + (15 - scaled_level), 3,
                            scaled_level + 1, WHITE);
    } else {
      oled_display.drawRect(0 + i * 4, 12 + (15 - scaled_level), 3,
                            scaled_level + 1, WHITE);
    }
  }
}

float OscPage::get_phase() { return 0; }
float OscPage::get_width() { return enc3.cur / 128.00; }
uint8_t OscPage::get_osc_type() { return osc_waveform; }

#endif
