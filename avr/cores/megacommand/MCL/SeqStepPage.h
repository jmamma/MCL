/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SEQSTEPPAGE_H__
#define SEQSTEPPAGE_H__

#include "SeqPage.h"

class SeqStepMidiEvents : public MidiCallback {
public:
  bool state;
  void onNoteOnCallback_Midi2(uint8_t *msg);
  void setup_callbacks();
  void remove_callbacks();

};

class SeqStepPage : public SeqPage {

public:
  bool show_pitch = false;
  SeqStepMidiEvents midi_events;
  SeqStepPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
              Encoder *e4 = NULL)
      : SeqPage(e1, e2, e3, e4) {
      }
  virtual bool handleEvent(gui_event_t *event);
  virtual void display();
  virtual void setup();
  virtual void init();
  virtual void config();
  virtual void loop();
  virtual void cleanup();
};

#endif /* SEQSTEPPAGE_H__ */
