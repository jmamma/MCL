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


extern uint8_t opt_trackid;
extern uint8_t opt_speed;
extern uint8_t opt_copy;
extern uint8_t opt_paste;
extern uint8_t opt_clear;
extern uint8_t opt_shift;
extern uint8_t opt_reverse;
extern uint8_t opt_clear_step;

extern void opt_trackid_handler();
extern void opt_speed_handler();
extern void opt_clear_track_handler();
extern void opt_clear_locks_handler();
extern void opt_copy_track_handler();
extern void opt_paste_track_handler();
extern void opt_shift_track_handler();
extern void opt_reverse_track_handler();
extern void opt_paste_step_handler();
extern void opt_copy_step_handler();
extern void opt_mute_step_handler();
extern void opt_clear_step_locks_handler();
extern void opt_mask_handler();

extern void seq_menu_handler();
extern void step_menu_handler();

#define MASK_PATTERN 0
#define MASK_SLIDE 1
#define MASK_LOCK 2
#define MASK_MUTE 3

class SeqPage : public LightPage {
public:
  // Static variables shared amongst derived objects
  static uint8_t page_select;
  static uint8_t page_count;
  static uint8_t midi_device;
  static uint8_t step_select;
  static uint8_t mask_type;

  static bool show_seq_menu;
  static bool show_step_menu;
  static bool toggle_device;

  bool recording = false;
  bool display_page_index = true;
  char info1[8] = { '\0' };
  char info2[8] = { '\0' };
  uint8_t timeout_values[4] = { 0 }; // 255 == highlight

  SeqPageMidiEvents seqpage_midi_events;
  SeqPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
          Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {
  }
  void config_as_trackedit();
  void config_as_lockedit();
  void config_mask_info();
  void create_chars_seq();
  void draw_lock_mask(uint8_t offset, uint64_t lock_mask, uint8_t step_count, uint8_t length, bool show_current_step = true);
  void draw_lock_mask(uint8_t offset, bool show_current_step = true);
  void draw_mask(uint8_t offset, uint64_t pattern_mask, uint8_t step_count, uint8_t length, bool show_current_step = true, uint64_t mute_mask = 0);
  void draw_mask(uint8_t offset, uint8_t device, bool show_current_step = true);
  void draw_knob_frame();
  void draw_knob(uint8_t i, const char* title, const char* text);
  void draw_knob(uint8_t i, Encoder* enc, const char* name);
  void draw_page_index(bool show_page_index = true, uint8_t _playing_idx = 255);
  void select_track(uint8_t device, uint8_t track);

  uint8_t get_md_speed(uint8_t speed_id);
  uint8_t get_ext_speed(uint8_t speed_id);

  virtual bool handleEvent(gui_event_t *event);
  virtual void loop();
  virtual void display();
  virtual void setup();
  virtual void init();
  virtual void cleanup();

  static constexpr uint8_t pidx_x0 = 1;
  static constexpr uint8_t pidx_y = 15;
  static constexpr uint8_t pidx_w = 6;
  static constexpr uint8_t pidx_h = 3;

};

#endif /* SEQPAGE_H__ */
