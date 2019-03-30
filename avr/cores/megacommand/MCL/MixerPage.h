/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MIXERPAGE_H__
#define MIXERPAGE_H__

//#include "Pages.hh"
#include "GUI.h"


class MixerMidiEvents : public MidiCallback {
public:
  bool state;

  void setup_callbacks();
  void remove_callbacks();
  uint8_t note_to_trig(uint8_t note_num);
  void onNoteOnCallback_Midi(uint8_t *msg);
  void onNoteOffCallback_Midi(uint8_t *msg);
  void onControlChangeCallback_Midi(uint8_t *msg);
};

void encoder_level_handle(Encoder *enc);
void encoder_filtf_handle(Encoder *enc);
void encoder_filtw_handle(Encoder *enc);
void encoder_lastparam_handle(Encoder *enc);

class MixerPage : public LightPage {
public:
  MixerMidiEvents midi_events;
  uint8_t level_pressmode = 0;
  int8_t disp_levels[16];
  MixerPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
            Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {
      }
  bool handleEvent(gui_event_t *event);
  void adjust_param(Encoder *enc, uint8_t param);
  void draw_levels();
  void display();
  void loop();
  void set_level(int curtrack, int value);
  void setup();
  void init();
  void cleanup();
};

#endif /* MIXERPAGE_H__ */
