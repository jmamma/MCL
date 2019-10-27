/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SEQPTCPAGE_H__
#define SEQPTCPAGE_H__

#include "MidiActivePeering.h"
#include "Scales.h"
#include "SeqPage.h"

#define MAX_POLY_NOTES 16

extern scale_t *scales[16];

void ptc_pattern_len_handler(Encoder *enc);

class SeqPtcMidiEvents : public MidiCallback {
public:
  bool state;

  void setup_callbacks();
  void remove_callbacks();

  void onNoteOnCallback_Midi2(uint8_t *msg);
  void onNoteOffCallback_Midi2(uint8_t *msg);
  void onControlChangeCallback_Midi(uint8_t *msg);
};

class SeqPtcPage : public SeqPage {

public:
  uint8_t poly_count = 0;
  uint8_t poly_max = 0;
  uint8_t last_midi_state = 0;
  int8_t poly_notes[MAX_POLY_NOTES];
  uint64_t note_mask = 0;
  uint8_t deferred_timer = 0;
  const uint8_t render_defer_time = 200;

  SeqPtcMidiEvents midi_events;
  SeqPtcPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
             Encoder *e4 = NULL)
      : SeqPage(e1, e2, e3, e4) {}
  uint8_t calc_poly_count();
  uint8_t seq_ext_pitch(uint8_t note_num);
  uint8_t get_machine_pitch(uint8_t track, uint8_t pitch);
  uint8_t get_next_voice(uint8_t pitch);
  uint8_t calc_pitch(uint8_t note_num);

  void trig_md(uint8_t note_num, uint8_t pitch);
  void trig_md_fromext(uint8_t note_num);
  void clear_trig_fromext(uint8_t note_num);

  void config_encoders();
  void init_poly();

  virtual bool handleEvent(gui_event_t *event);
  virtual void display();
  virtual void setup();
  virtual void cleanup();
  virtual void loop();
  virtual void init();
  virtual void config();
};

#endif /* SEQPTCPAGE_H__ */
