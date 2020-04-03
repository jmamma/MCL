/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef WAVEDITPAGE_H__
#define WAVEDITPAGE_H__

#include "GUI.h"
#include "Wav.h"

#define WAV_DRAW_WIDTH 128
#define WAV_DRAW_HEIGHT 32

class WavEditPage : public LightPage {
public:
  Wav wav_file;
  int8_t wav_buf[WAV_DRAW_WIDTH][2];
  WavEditPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
          Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {
  }
  virtual bool handleEvent(gui_event_t *event);
  void display();
  void setup();
  void init();
  void loop();
  void cleanup();
  void draw_wav();
};

extern WavEditPage wav_edit_page;
#endif /* WAVEDITPAGE_H__ */
