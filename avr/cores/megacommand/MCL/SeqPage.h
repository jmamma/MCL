/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SEQPAGE_H__
#define SEQPAGE_H__
#include "GUI.h"

class SeqPage : LightPage {
 public:
 static uint8_t page_select;
 
 static uint8_t last_md_track;
 static uint8_t last_ext_track;

SeqPageMidiEvents midi_events;
 SeqPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL, Encoder *e4 = NULL) : LightPage( e1, e2, e3 ,e4) {
 midi_events.setup_callbacks();
 }
 virtual void pattern_len_handler(Encoder *enc);
 virtual bool handleEvent(gui_event_t *event);
 virtual void create_char_seq();
  void draw_lockmask(uint8_t offset);
  void draw_patternmask(uint8_t offset, uint8_t device);
 virtual void setup() { }
 virtual void init() { }
};

class SeqPageMidiEvents : public MidiCallback {
  public:
  void setup_callbacks();
  void remove_callbacks();
  virtual void onControlChangeCallback_Midi(uint8_t *msg);
}


#endif /* SEQPAGE_H__ */
