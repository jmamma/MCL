/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef OSCPAGE_H__
#define OSCPAGE_H__

#include "GUI.h"

// Encoder 1: Oscillator Type
// Encoder 2: Pitch
// Encoder 3: Phase
// Encoder 4: Mod (Width, Skew, Paramter Level etc)

void osc_mod_handler(Encoder *enc);



class OscPage : public LightPage {
public:
  // Static variables shared amongst derived objects
  uint8_t id;
  MCLEncoder enc1;
  MCLEncoder enc2;
  MCLEncoder enc3;
  MCLEncoder enc4;

  uint8_t sine_levels[16];
  uint8_t custom_values[16];
  OscPage() {
    enc1.initMCLEncoder(0, 4, 0, ENCODER_RES_SEQ);
    enc2.initMCLEncoder(0, 127, 0, ENCODER_RES_SEQ);
    enc3.initMCLEncoder(0, 180, 0, ENCODER_RES_SEQ);
    enc4.initMCLEncoder(0, 127, 0, ENCODER_RES_SEQ);

    encoders[0] = (Encoder *)&enc1;
    encoders[1] = (Encoder *)&enc2;
    encoders[2] = (Encoder *)&enc3;
    encoders[3] = (Encoder *)&enc4;
    enc4.handler = osc_mod_handler;
    sine_levels[0] = 127;
    enc2.cur = 64;
  }
  virtual bool handleEvent(gui_event_t *event);
  void display();
  void setup();
  void init();
  void loop();
  void cleanup();
  void draw_levels();
  float get_freq();
  float get_phase();
  uint8_t get_osc_type();
};

#endif /* OSCPAGE_H__ */
