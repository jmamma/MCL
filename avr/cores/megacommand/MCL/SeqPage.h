/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SEQPAGE_H__
#define SEQPAGE_H__
#include "GUI.h"

class SeqPage : LightPage {
 public:
 SeqPageMidiEvents midi_events;
 SeqPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL, Encoder *e4 = NULL) : LightPage( e1, e2, e3 ,e4) {
 midi_events.setup_callbacks();
 }
 virtual bool handleEvent(gui_event_t *event);
 virtual void setup();
};

class SeqPageMidiEvents : public MidiCallback {
  public:
  void setup_callbacks();
  void remove_callbacks();
  virtual void onControlChangeCallback_Midi(uint8_t *msg);
}


#endif /* SEQPAGE_H__ */
