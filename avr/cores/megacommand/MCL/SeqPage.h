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


extern void pattern_len_handler(Encoder *enc);

class SeqPage : public LightPage {
public:
  // Static variables shared amongst derived objects
  static uint8_t page_select;
  static uint8_t midi_device;

  SeqPageMidiEvents seqpage_midi_events;
  SeqPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
          Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {
  }
  virtual bool handleEvent(gui_event_t *event);
  void create_chars_seq();
  void draw_lock_mask(uint8_t offset, bool show_current_step = true);
  void draw_pattern_mask(uint8_t offset, uint8_t device, bool show_current_step = true);
  void cleanup() {
 DEBUG_PRINTLN("clean up");
  seqpage_midi_events.remove_callbacks();
  }
  void display();
  void setup();
  void init();
};

#endif /* SEQPAGE_H__ */
