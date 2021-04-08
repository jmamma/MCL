/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SEQSTEPPAGE_H__
#define SEQSTEPPAGE_H__

#include "SeqPage.h"

class SeqStepMidiEvents : public MidiCallback, public ClockCallback {
public:
  bool state;
  void onNoteOnCallback_Midi2(uint8_t *msg);
  void onControlChangeCallback_Midi(uint8_t *msg);
  void setup_callbacks();
  void remove_callbacks();
  void onMidiStartCallback();

};

class SeqStepPage : public SeqPage {

public:
  bool show_pitch = false;
  bool reset_on_release = false;
  bool update_params_queue;
  uint16_t update_params_clock;
  uint8_t last_param_id;
  uint8_t last_rec_event;
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
  virtual void config_encoders();
  virtual void loop();
  virtual void cleanup();
  void send_locks(uint8_t step);
};

#endif /* SEQSTEPPAGE_H__ */
