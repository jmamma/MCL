/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SEQEXTSTEPPAGE_H__
#define SEQEXTSTEPPAGE_H__

#include "SeqPage.h"
#include "SeqStepPage.h"

void ext_pattern_len_handler(Encoder *enc);
class SeqExtStepMidiEvents : public MidiCallback {
public:
  bool state;

  void setup_callbacks();
  void remove_callbacks();

  void onNoteOnCallback_Midi2(uint8_t *msg);
  void onNoteOffCallback_Midi2(uint8_t *msg);
};

class SeqExtStepPage : public SeqPage {

public:
  static constexpr uint8_t fov_w = 96;
  static constexpr uint8_t fov_notes = 7;
  static constexpr uint8_t fov_h = 28;
  int16_t fov_offset = 0;
  uint16_t fov_length;

  static constexpr uint8_t draw_x = 128 - fov_w;
  static constexpr uint8_t keyboard_w = 4;

  float fov_pixels_per_tick;
  SeqExtStepMidiEvents midi_events;
  SeqExtStepPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
                 Encoder *e4 = NULL)
      : SeqPage(e1, e2, e3, e4) {}
  void config_encoders();

  void draw_note(uint8_t note_val, uint16_t note_start, uint16_t note_end);
  void draw_pianoroll();
  uint8_t find_note_off(int8_t note_val, uint8_t step);

  bool is_within_fov(uint16_t x) {
    if ((x >= fov_offset) && (x < fov_offset + fov_length)) { return true; }
    return false;
  }

  virtual bool handleEvent(gui_event_t *event);
  virtual void display();
  virtual void loop();
  virtual void setup();
  virtual void init();
  virtual void config();
  virtual void cleanup();
};

#endif /* SEQEXTSTEPPAGE_H__ */
