/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SEQPAGE_H__
#define SEQPAGE_H__

#include "GUI.h"
#include "midi-common.hh"

class SeqPageMidiEvents : public MidiCallback {
public:
  void setup_callbacks();
  void remove_callbacks();
  virtual void onControlChangeCallback_Midi(uint8_t *msg);
};


void pattern_len_handler(Encoder *enc);

class SeqPage : public LightPage {
public:
  // Static variables shared amongst derived objects
  static uint8_t page_select;


  SeqPageMidiEvents midi_events;
  SeqPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
          Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {
    midi_events.setup_callbacks();
  }
  virtual bool handleEvent(gui_event_t *event);
  void create_chars_seq();
  void draw_lock_mask(uint8_t offset);
  void draw_pattern_mask(uint8_t offset, uint8_t device);
  void display();
  void setup();
  void init();
};

#endif /* SEQPAGE_H__ */
