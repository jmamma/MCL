/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MUTEPAGE_H__
#define MUTEPAGE_H__

#include "GUI.h"

class MuteMidiEvents : public MidiCallback {
public:
  bool state;

  void setup_callbacks();
  void remove_callbacks();
  uint8_t note_to_trig(uint8_t note_num);
  void onNoteOnCallback_Midi(uint8_t *msg);
  void onNoteOffCallback_Midi(uint8_t *msg);
};

class MutePage : public LightPage {
 public:
 MuteMidiEvents midi_events;
 uint32_t mutes;
 uint16_t detect_time;
 MutePage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL, Encoder *e4 = NULL) : LightPage( e1, e2, e3 ,e4) {

 }
 void toggle_mute(int i);
 void toggle_mutes_batch();
 void draw_mutes(uint8_t line_number);
 bool handleEvent(gui_event_t *event);
 void display();
 void setup();
 void init();
 void cleanup();
};
#endif /* MUTEPAGE_H__ */
