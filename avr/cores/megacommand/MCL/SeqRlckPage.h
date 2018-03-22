/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SEQRLCKPAGE_H__
#define SEQRLCKPAGE_H__

#include "SeqPage.h"

class SeqRlckPageMidiEvents : public MidiCallback {
  bool state;

  void setup_callbacks();
  void remove_callbacks();

  void onControlChangeCallback_Midi(uint8_t *msg);
  void onControlChangeCallback_Midi2(uint8_t *msg);

};

class SeqRlckPage : public SeqPage {

public:
  SeqRlckPageMidiEvents midi_events;
  SeqRlckPage(Encoder *e1 = NULL,
               Encoder *e2 = NULL, Encoder *e3 = NULL, Encoder *e4 = NULL)
      : SeqPage(e1, e2, e3, e4) {}
  bool handleEvent(gui_event_t *event);
  void display();
  void setup();
  void init();
};

#endif /* SEQRLCKPAGE_H__ */
