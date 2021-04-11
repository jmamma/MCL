/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef WAVDESIGNER_H__
#define WAVDESIGNER_H__

#include "MCL.h"
#include "math.h"
#include "OscPage.h"
#include "OscMixerPage.h"

#define MIXER_ID 4
#define NUM_OSC 3
class WavDesigner {
public:
  OscPage pages[3];
  OscMixerPage mixer;
  uint32_t loop_start;
  uint32_t loop_end;
  LightPage *last_page;
  WavDesigner() {
    for (uint8_t i = 0; i < 3; i++) {
      pages[i].id = i;
    }
    mixer.id = MIXER_ID;
    pages[0].osc_waveform = 1;
    mixer.enc4.cur = 0;
    last_page = &(pages[0]);
  }
  void prompt_send();
  bool render();
  bool send();
};

extern WavDesigner wd;

#endif /* WAVDESIGNER_H__ */
