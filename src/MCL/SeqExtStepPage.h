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
  void onControlChangeCallback_Midi2(uint8_t *msg);
};

class SeqExtStepPage : public SeqPage {

public:
  static constexpr uint8_t fov_w = 96;
  static constexpr uint8_t fov_notes = 7;
  static constexpr uint8_t fov_h = 28;
  int16_t fov_y;

  int16_t fov_offset = 0;
  uint16_t fov_length;

  static constexpr uint8_t draw_y = 2;
  static constexpr uint8_t draw_x = 128 - fov_w;
  static constexpr uint8_t keyboard_w = 3;

  static constexpr uint8_t zoom_max = 32;

  float fov_pixels_per_tick;

  int16_t cur_x;
  int16_t cur_y;
  int16_t cur_w;

  int16_t last_cur_x;

  int8_t lock_cur_y = 64;

  static constexpr int16_t cur_w_min = 2;

  int16_t roll_length;

  bool scroll_dir;

  bool encoder_init;

  SeqExtStepMidiEvents midi_events;
  SeqExtStepPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
                 Encoder *e4 = NULL)
      : SeqPage(e1, e2, e3, e4) {}
  virtual void config_encoders();

  void draw_thick_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2,
                       uint8_t color = WHITE);
  void draw_note(uint8_t x, uint8_t y, uint8_t w);
  void draw_pianoroll();
  void draw_lockeditor();
  void draw_viewport_minimap();
  void draw_seq_pos();
  void draw_grid();
  void set_cur_y(uint8_t cur_y_);
  void pos_cur_x(int16_t diff);
  void pos_cur_y(int16_t diff);
  void pos_cur_w(int16_t diff);

  inline bool is_within_fov(int16_t x) {
    return (x - fov_offset) < fov_length;
  }

  bool is_within_fov(int16_t start_x, int16_t end_x) {
    int16_t fov_end = fov_offset + fov_length;
    // Handle wrap-around case
    if (end_x < start_x) {
        return (start_x < fov_end) || (end_x >= fov_offset);
    }
    // Normal case
    return (start_x < fov_end) && (end_x >= fov_offset);
  }

  void param_select_update();
  void enter_notes();
  void config_menu_entries();
  virtual bool handleEvent(gui_event_t *event);
  virtual void display();
  virtual void loop();
  virtual void setup();
  virtual void init();
  virtual void config();
  virtual void cleanup();
};

#endif /* SEQEXTSTEPPAGE_H__ */
