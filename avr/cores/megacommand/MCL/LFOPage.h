/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef LFOPAGE_H__
#define LFOPAGE_H__

#include "GUI.h"
#include "MCLEncoder.h"
#include "LFOSeqTrack.h"

#define NUM_LFO_PAGES 2

#define SIN_WAV 0
#define TRI_WAV 1
#define EXP_WAV 2
//
class LFOPage : public LightPage, MidiCallback {
public:
  LFOPage(LFOSeqTrack *lfo_track_, Encoder *e1 = NULL, Encoder *e2 = NULL,
          Encoder *e3 = NULL, Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {
  lfo_track = lfo_track_;
  }

  bool handleEvent(gui_event_t *event);
  bool midi_state = false;

  bool page_mode;
  uint8_t page_id;

  LFOSeqTrack *lfo_track;

  uint8_t waveform;
  uint8_t depth;
  uint8_t depth2;

  void display();
  void setup();
  void draw_pattern_mask();
  void init();
  void loop();
  void cleanup();
  void update_encoders();
  void load_wavetable(uint8_t waveform, LFOSeqTrack *lfo_track, uint8_t param, uint8_t depth);

  void setup_callbacks();
  void remove_callbacks();

  void onControlChangeCallback_Midi(uint8_t *msg);
};

extern MCLEncoder lfo_page_param1;
extern MCLEncoder lfo_page_param2;
extern MCLEncoder lfo_page_param3;
extern MCLEncoder lfo_page_param4;

#endif /* LFOPAGE_H__ */
