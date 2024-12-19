/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef OSCMIXERPAGE_H__
#define OSCMIXERPAGE_H__

#include "GUI.h"
#include "WavDesignerPage.h"

class OscMixerPage : public WavDesignerPage {
public:
  // Static variables shared amongst derived objects
  float num_of_channels = 3;
  uint8_t sample_number;
  uint8_t scanline_width;
  MCLEncoder enc1;
  MCLEncoder enc2;
  MCLEncoder enc3;
  MCLEncoder enc4;

  OscMixerPage() {
    enc1.initMCLEncoder(0, 127, 0, ENCODER_RES_SEQ);
    enc2.initMCLEncoder(0, 127, 0, ENCODER_RES_SEQ);
    enc3.initMCLEncoder(0, 127, 0, ENCODER_RES_SEQ);
    enc4.initMCLEncoder(0, 999, 0, ENCODER_RES_SEQ);

    encoders[0] = (Encoder *)&enc1;
    encoders[1] = (Encoder *)&enc2;
    encoders[2] = (Encoder *)&enc3;
    encoders[3] = (Encoder *)&enc4;

    for (uint8_t i = 0; i < 3; i++) {
      encoders[i]->cur = 64;
    }
  }
  virtual bool handleEvent(gui_event_t *event);
  void display();
  void setup();
  void init();
  void loop();
  void cleanup();
  float get_max_gain();
  float get_gain(uint8_t channel);
  void draw_levels();
  void draw_wav();
};

#endif /* OSCMIXERPAGE_H__ */
