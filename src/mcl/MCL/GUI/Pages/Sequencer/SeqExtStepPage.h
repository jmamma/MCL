/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SEQEXTSTEPPAGE_H__
#define SEQEXTSTEPPAGE_H__

#include "GUI/Pages/Sequencer/SeqPage.h"
#include "Sequencer/SeqExtStepTypes.h"
#include "GUI/Pages/Sequencer/SeqStepPage.h"
#ifdef PLATFORM_TBD
#include "Sequencer/SeqExtMidiControl.h"
#endif

class MidiClass;

#ifdef PLATFORM_TBD
void ext_pattern_len_handler(Encoder *enc);
class SeqExtStepMidiEvents : public MidiCallback {
public:
  bool state;
  MidiClass *bound_midi;
  SeqExtMidiControlState control_state;

  void setup_callbacks();
  void remove_callbacks();

  void onNoteOnCallback_Midi2(uint8_t *msg);
  void onNoteOffCallback_Midi2(uint8_t *msg);
  void onControlChangeCallback_Midi2(uint8_t *msg);
  void onPitchWheelCallback_Midi2(uint8_t *msg);
  void onAfterTouchCallback_Midi2(uint8_t *msg);
  void onChannelPressureCallback_Midi2(uint8_t *msg);
};
#endif

class SeqExtStepPage : public SeqPage {

public:
  static constexpr uint8_t fov_w = 96;
  static constexpr uint8_t fov_notes = 7;
  static constexpr uint8_t fov_h = 28;
  int16_t fov_y;

  seq_extstep_tick_t fov_offset;
  seq_extstep_tick_t fov_length;

  static constexpr uint8_t draw_y = 2;
  static constexpr uint8_t draw_x = 128 - fov_w;
  static constexpr uint8_t keyboard_w = 3;

  static constexpr uint8_t zoom_max = 32;

  uint16_t fov_pixels_per_tick; // Q8: actual_value × 256

  seq_extstep_tick_t cur_x;
  int16_t cur_y;
  seq_extstep_tick_t cur_w;

  seq_extstep_tick_t last_cur_x;

  int8_t lock_cur_y = 64;

  static constexpr seq_extstep_tick_t cur_w_min = 2;

  seq_extstep_tick_t roll_length;

  bool scroll_dir;

  bool encoder_init;

  bool note_selection_active;
  bool note_selection_editing;
  bool note_selection_width_saved;
  uint8_t note_selection_track;
  seq_extstep_tick_t note_selection_anchor_x;
  seq_extstep_tick_t note_selection_x;
  int16_t note_selection_anchor_y;
  int16_t note_selection_y;
  seq_extstep_tick_t note_selection_saved_w;
  seq_extstep_tick_t note_selection_saved_fov_offset;
  int16_t note_selection_saved_fov_y;

#ifdef PLATFORM_TBD
  SeqExtStepMidiEvents midi_events;
#endif
  SeqExtStepPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
                 Encoder *e4 = NULL)
      : SeqPage(e1, e2, e3, e4) {}
  virtual void config_encoders();

  void draw_thick_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
  void draw_note(uint8_t x, uint8_t y, uint8_t w, bool note_beyond_fov);
  void draw_pianoroll() NOINLINE();
  void draw_lockeditor() NOINLINE();
  void draw_note_selection();
  void draw_viewport_minimap();
  void draw_seq_pos();
  void draw_grid();
  uint8_t fov_x_for_tick(seq_extstep_tick_t tick) const NOINLINE();
  uint8_t lock_y_for_value(uint8_t value) const NOINLINE();
  uint8_t note_y_for_value(int16_t value) const NOINLINE();
  void set_cur_y(uint8_t cur_y_);
  void pos_cur_x(seq_extstep_tick_t diff);
  void pos_cur_y(int16_t diff);
  void pos_cur_w(seq_extstep_tick_t diff);
  void save_note_selection_view() NOINLINE();
  void clamp_fov_offset() NOINLINE();
  void begin_note_selection();
  void finish_note_selection();
  void move_note_selection(seq_extstep_tick_t x_diff, int16_t y_diff);
  void clear_note_selection();
  bool copy_note_selection();
  bool copy_note_page();
  bool copy_lock_page() NOINLINE();
  bool paste_note_clip();
  bool paste_lock_clip() NOINLINE();
  bool clear_lock_page() NOINLINE();
  bool clear_note_selection_notes();
  bool clear_note_page();

  bool is_within_fov(seq_extstep_tick_t x) NOINLINE();

  bool is_within_fov(seq_extstep_tick_t start_x, seq_extstep_tick_t end_x) NOINLINE();

  seq_extstep_tick_t clamp_width_at(seq_extstep_tick_t x) NOINLINE();
  void param_select_update();
  void enter_notes();
  void config_menu_entries();
  bool handle_cc_lock_learn(uint8_t track, uint8_t param, uint8_t value);
  virtual bool handleEvent(gui_event_t *event);
  virtual void display();
  virtual void loop();
  virtual void setup();
  virtual void init();
  virtual void config();
  virtual void cleanup();
};

#endif /* SEQEXTSTEPPAGE_H__ */
