/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef WAVDESIGNER_H__
#define WAVDESIGNER_H__

#include "MCL.h"
#include "Math.h"

class WavDesigner {
public:
  OscPage pages[3];
  OscMixerPage mixer;
  WavDesigner() {
    for (uint8_t i = 0; i < 3; i++) {
      pages[i].id = i;
    }
  }
  bool render();
};

extern WavDesigner wav_designer;

#endif /* WAVDESIGNER_H__ */
