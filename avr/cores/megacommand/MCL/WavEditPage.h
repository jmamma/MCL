/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef WAVEDITPAGE_H__
#define WAVEDITPAGE_H__

#include "GUI.h"
#include "Wav.h"

#define WAV_DRAW_WIDTH 128
#define WAV_DRAW_HEIGHT 32
#define WAV_SECONDS 1.0 //maximmum sample length to display on screen

class WavEditPage : public LightPage {
public:
  Wav wav_file;
  uint32_t start;
  uint32_t end;
  uint32_t offset;
  uint32_t samples_per_pixel;
  int8_t wav_buf[WAV_DRAW_WIDTH][2];
  WavEditPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
          Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {
  }
  virtual bool handleEvent(gui_event_t *event);
  void display();
  void setup();
  void open(char *file);
  void init();
  void loop();
  void render(uint32_t sample_start, uint32_t sample_end, int32_t offset, uint32_t samples_per_pixel);
  void cleanup();
  void draw_wav();
};

extern WavEditPage wav_edit_page;
#endif /* WAVEDITPAGE_H__ */
