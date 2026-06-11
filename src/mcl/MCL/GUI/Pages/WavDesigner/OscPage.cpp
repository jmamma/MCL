#include "OscPage.h"
#include "DeviceManager.h"
#include "../../../../Drivers/MD/MD.h"
#include "Osc.h"
#include "DSP.h"
#include "MidiNotes.h"
#include "SeqPage.h"
#include "WavDesigner.h"

#ifdef WAV_DESIGNER

void osc_mod_handler(EncoderParent *enc) {}

uint32_t OscPage::exploit_delay_clock = 0;

static constexpr int16_t WD_OSC_NOTE_OFFSET = 8;
static constexpr uint8_t WD_OSC_FREQ_FRAC_BITS = 3;

// Add a signed encoder delta to a 0..127 level and clamp back into range.
// Shared by the sine-level and usr-value edit paths in loop() so the clamp
// is emitted once instead of inline at both sites.
static uint8_t osc_clamp_level(uint8_t cur, int8_t diff) NOINLINE();
static uint8_t osc_clamp_level(uint8_t cur, int8_t diff) {
  int16_t newval = (int16_t)cur + diff;
  if (newval < 0) {
    newval = 0;
  }
  if (newval > 127) {
    newval = 127;
  }
  return (uint8_t)newval;
}

// Map a [-1,1] oscillator sample to a y pixel for the 30px-high preview area
// (h=30, y=0): centre at 15, +/-15 swing. Shared by draw_wav() and draw_usr()
// so the float multiply-add + cast is emitted once.
static uint8_t osc_sample_to_y(float sample) NOINLINE();
static uint8_t osc_sample_to_y(float sample) {
  return (uint8_t)((sample * ((float)30 / 2.00f)) + (30 / 2));
}

void OscPage::setup() {
  for (uint8_t i = 0; i < 16; i++) {
    usr_values[i] = get_random(127);
  }
}

float OscPage::get_freq() {
  int16_t coarse = (int16_t)enc1.cur - (WD_OSC_NOTE_OFFSET + MIDI_NOTE_A4);
#if defined(__AVR__)
  static const uint16_t semitone_q15[12] PROGMEM = {
      32768, 34716, 36781, 38968, 41285, 43740,
      46341, 49097, 52016, 55109, 58386, 61858,
  };

  int8_t octave = coarse / 12;
  int8_t semitone = coarse % 12;
  if (semitone < 0) {
    semitone += 12;
    octave--;
  }

  uint32_t freq_q3 =
      (440UL * pgm_read_word(semitone_q15 + semitone) +
       (1UL << (15 - WD_OSC_FREQ_FRAC_BITS - 1))) >>
      (15 - WD_OSC_FREQ_FRAC_BITS);
  if (octave > 0) {
    freq_q3 <<= octave;
  } else if (octave < 0) {
    uint8_t shift = -octave;
    freq_q3 = (freq_q3 + (1UL << (shift - 1))) >> shift;
  }

  int16_t fine = -enc2.cur;
  // Q15 approximation of 2^(fine_cents / 1200). The quadratic term keeps
  // the AVR path within about 0.3 cents over the encoder range.
  int32_t fine_q15 = 32768 + fine * 19;
  fine_q15 += ((int32_t)fine * fine * 11 + 1024) >> 11;
  freq_q3 = (freq_q3 * (uint32_t)fine_q15 + 16384UL) >> 15;
  return (float)freq_q3 * (1.0f / (1 << WD_OSC_FREQ_FRAC_BITS));
#else
  float cents = (float)coarse * 100.0f - (float)enc2.cur;
  float scale = (float)powf(2.0f, cents / 1200.0f);
  return 440.0f * scale;
#endif
}
void OscPage::init() {
  WavDesignerPage::init();
  wavdesign_menu_page.menu.enable_entry(1, true);
  wavdesign_menu_page.menu.enable_entry(2, false);
  oled_display.clearDisplay();
}

void OscPage::cleanup() {}

bool OscPage::handleEvent(gui_event_t *event) {
  if (WavDesignerPage::handleEvent(event)) {
    return true;
  }
  if (EVENT_NOTE(event)) {
    uint8_t port = event->port;

    if (!device_manager.port_supports(
            port, MidiDeviceCapability::MdTrigInterface)) {
      return true;
    }
    key_interface.send_md_leds(TRIGLED_OVERLAY);
  }
  if (EVENT_CMD(event)) {
    uint8_t key = event->source;
    if (event->mask == EVENT_BUTTON_PRESSED) {
      switch (key) {
      case MDX_KEY_NO:
        //  key_interface.ignoreNextEvent(MDX_KEY_NO);
        show_freq = !show_freq;
        return true;
      }
    }
  }
  if (EVENT_BUTTON(event)) {
    if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
      show_freq = !show_freq;
    }
  }
  return false;
}

void OscPage::calc_sine_level_sum() {
  sine_level_sum = 0;
  for (uint8_t f = 0; f < 16; f++) {
    sine_level_sum += sine_levels[f];
  }
}

void OscPage::loop() {
  WavDesignerPage::loop();
  MCLEncoder *enc_ = &enc4;
  // largest_sine_peak = 1.0 / 16.00;
  calc_sine_level_sum();

  if (show_menu == false) {
    if (encoders[0]->hasChanged()) {
      encoders[1]->cur = 0;
      encoders[1]->old = 0;
    }
  }

  int8_t diff = consume_centered_encoder_delta(enc_);
  for (uint8_t i = 0; i < 16; i++) {
    if (note_interface.is_note_on(i)) {
      if (osc_waveform == SIN_OSC) {
        sine_levels[i] = osc_clamp_level(sine_levels[i], diff);
      }
      if (osc_waveform == USR_OSC) {
        usr_values[i] = osc_clamp_level(usr_values[i], diff);
      }
    }
  }
  if ((osc_waveform == SIN_OSC) || (osc_waveform == USR_OSC)) {
    if (!key_interface.state) {
      key_interface.on();
    }
  }

  else {
    key_interface.off();
  }
}



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
  mcl_print_P(mclstr_space);

  if (show_freq) {
    float freq = get_freq();
    oled_display.print((int)freq);
    mcl_print_P(mclstr_hz);
    // GUI.printf_at(6, "%f", freq);
  } else {
    uint8_t s = enc1.cur - 8;
    char note_label[5];
    seq_copy_note_label(s, note_label);
    oled_display.print(note_label);
    if (enc2.cur < 0) {
      mcl_print_P(mclstr_plus);
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
  uint8_t n = sample_number;
  // for (uint8_t n = 0; n < 128 - x; n++) {

  oled_display.fillRect(n + x, 0, scanline_width, 32, BLACK);
  for (uint8_t n = sample_number; n < sample_number + scanline_width; n++) {
    //  if ((scanline_width < w) && (wav_type > 0)) {
    //  oled_display.drawLine(n + x, 0, n + x, 32, BLACK);
    // }
    sample = render_osc_sample(wav_type, osc_width, sine_levels, usr_values,
                               sine_level_sum, n, 1.0f, sine_osc, tri_osc,
                               pul_osc, saw_osc, usr_osc);
    uint8_t pixel_y = osc_sample_to_y(sample);
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
  float sample;
  uint8_t w = 128 - 64;
  UsrOsc usr_osc(w);

  for (uint8_t i = 0; i < 16; i++) {
    sample = usr_osc.get_sample((uint32_t)i * 4, 1, usr_values);

    uint8_t pixel_y = osc_sample_to_y(sample);
    if (note_interface.is_note_on(i)) {

      // oled_display.fillRect(63 + i * 4, 0, 3, 32, BLACK);
      oled_display.drawRect(63 + i * 4, pixel_y - 1, 3, 3, WHITE);
    }
  }
}

void OscPage::draw_levels() {
  for (uint8_t i = 0; i < 16; i++) {
    uint8_t scaled_level = ((uint16_t)sine_levels[i] * 15) / 127;
    uint8_t bx = i * 4;
    uint8_t by = 12 + (15 - scaled_level);
    uint8_t bh = scaled_level + 1;
    if (note_interface.is_note_on(i)) {
      oled_display.fillRect(bx, by, 3, bh, WHITE);
    } else {
      oled_display.drawRect(bx, by, 3, bh, WHITE);
    }
  }
}

float OscPage::get_phase() { return 0; }
float OscPage::get_width() { return enc3.cur / 128.00; }
uint8_t OscPage::get_osc_type() { return osc_waveform; }

#endif
