/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef WAVEDITPAGE_H__
#define WAVEDITPAGE_H__

#include "GUI.h"
#include "Wav.h"

#define WAV_SECONDS 1.0 // maximmum sample length to display on screen

#define WAV_DRAW_STEREO 2
#define WAV_DRAW_LEFT 0
#define WAV_DRAW_RIGHT 1

class WavEditPage : public LightPage {
public:
  Wav wav_file;
  uint32_t selection_start;
  uint32_t selection_end;

  uint32_t start;
  uint32_t end;
  int32_t offset;

  uint32_t samples_per_pixel;

  static constexpr uint8_t draw_w = 96;
  static constexpr uint8_t draw_h = 32;

  int8_t wav_buf[2][draw_w][2];

  uint8_t draw_mode = WAV_DRAW_STEREO;

  WavEditPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
              Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {}
  virtual bool handleEvent(gui_event_t *event);
  void display();
  void setup();
  void open(char *file);
  void init();
  void loop();
  void render(uint32_t sample_start, uint32_t sample_end, int32_t sample_offset,
              uint32_t samples_per_pixel);
  void cleanup();
  void draw_wav();

  uint8_t get_selection_start();
  uint8_t get_selection_end();
  uint8_t get_selection_width();

  wav_sample_t get_selection_sample_start();
  wav_sample_t get_selection_sample_end();

};

extern WavEditPage wav_edit_page;
#endif /* WAVEDITPAGE_H__ */
