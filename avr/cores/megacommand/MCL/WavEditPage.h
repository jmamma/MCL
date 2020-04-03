/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef WAVEDITPAGE_H__
#define WAVEDITPAGE_H__

#include "GUI.h"

#define WAV_DRAW_WIDTH 128

class WavEditPage : public LightPage {
public:
  uint8_t wav_buf[WAV_DRAW_WIDTH];
  WavEditPage() {
 }
  virtual bool handleEvent(gui_event_t *event);
  void display();
  void setup();
  void init();
  void loop();
  void cleanup();
  void draw_wav();
  uint8_t get_osc_type();
};

#endif /* WAVEDITPAGE_H__ */
