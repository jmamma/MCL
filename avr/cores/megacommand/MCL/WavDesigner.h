/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef WAVDESIGNER_H__
#define WAVDESIGNER_H__

#include "MCL.h"
#include "Math.h"

class WavDesigner {
public:
  OscPage pages[3];
  OscMixerPage mixer;
  uint32_t loop_start;
  uint32_t loop_end;
  WavDesigner() {
    for (uint8_t i = 0; i < 3; i++) {
      pages[i].id = i;
    }
    pages[0].enc1.cur = 1;
  }
  bool render();
  bool send();
};

extern WavDesigner wd;

#endif /* WAVDESIGNER_H__ */
