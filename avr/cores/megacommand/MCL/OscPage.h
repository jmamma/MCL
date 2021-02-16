/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef OSCPAGE_H__
#define OSCPAGE_H__

#include "GUI.h"
#include "WavDesignerPage.h"

// Encoder 1: Oscillator Type
// Encoder 2: Pitch
// Encoder 3: Phase
// Encoder 4: Mod (Width, Skew, Paramter Level etc)

void osc_mod_handler(EncoderParent *enc);



class OscPage : public WavDesignerPage {
public:
  MCLEncoder enc1;
  MCLEncoder enc2;
  MCLEncoder enc3;
  MCLEncoder enc4;
  bool show_freq = false;
  uint8_t osc_waveform;

  uint8_t sample_number = 0;
  uint8_t scanline_width;
  uint8_t sine_levels[16];
  uint8_t usr_values[16];
  float largest_sine_peak;
  static uint32_t exploit_delay_clock;
  OscPage() {
    enc1.initMCLEncoder(8,118, 0, ENCODER_RES_SEQ);
    enc2.initMCLEncoder(-99, 99, 0, ENCODER_RES_SEQ);
    enc3.initMCLEncoder(0, 127, 0, ENCODER_RES_SEQ);
    enc4.initMCLEncoder(0, 127, 0, ENCODER_RES_SEQ);

    encoders[0] = (Encoder *)&enc1;
    encoders[1] = (Encoder *)&enc2;
    encoders[2] = (Encoder *)&enc3;
    encoders[3] = (Encoder *)&enc4;
    enc4.handler = osc_mod_handler;
    sine_levels[0] = 127;
    enc1.cur = 65;
    enc2.cur = 0;
    enc3.cur = 64;
  }
  virtual bool handleEvent(gui_event_t *event);
  void display();
  void setup();
  void init();
  void loop();
  void cleanup();
  void draw_levels();
  void draw_usr();
  void draw_tri();
  void draw_saw();
  void draw_pul();
  void draw_wav(uint8_t wav_type);
  void calc_largest_sine_peak();
  float get_freq();
  float get_width();
  float get_phase();
  uint8_t get_osc_type();
};

#endif /* OSCPAGE_H__ */
