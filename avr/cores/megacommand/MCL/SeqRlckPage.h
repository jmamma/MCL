/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SEQRLCKPAGE_H__
#define SEQRLCKPAGE_H__
#include "GUI.h"

class SeqRlckPage : public SeqPage {

public:
  SeqRlckPageMidiEvents;
  SeqRlckPage(Encoder *e1 = NULL,
               Encoder *e2 = NULL, Encoder *e3 = NULL, Encoder *e4 = NULL)
      : SeqPage(e1, e2, e3, e4) {}
  bool handleEvent(gui_event_t *event);
  bool display();
  void setup();
  void init();
};

class SeqRlckPageMidiEvents : public MidiCallBack {
  bool state;

  void setup_callbacks();
  void remove_callbacks();

  void onControlChangeCallback_Midi(uint8_t *msg);
  void onControlChangeCallback_Midi2(uint8_t *msg);

}
#endif /* SEQRLCKPAGE_H__ */
