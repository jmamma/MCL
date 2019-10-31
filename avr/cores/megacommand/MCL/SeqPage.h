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
  static uint8_t page_count;
  static uint8_t midi_device;
  static uint8_t length;
  static uint8_t resolution;
  static uint8_t apply;
  static uint8_t ignore_button_release;
  static bool show_track_menu;

  bool recording = false;
  char info1[8] = { '\0' };
  char info2[8] = { '\0' };
  uint8_t timeout_values[4] = { 0 }; // 255 == highlight

  SeqPageMidiEvents seqpage_midi_events;
  SeqPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
          Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {
  }
  void create_chars_seq();
  void draw_lock_mask(uint8_t offset, uint64_t lock_mask, uint8_t step_count, uint8_t length, bool show_current_step = true);
  void draw_lock_mask(uint8_t offset, bool show_current_step = true);
  void draw_pattern_mask(uint8_t offset, uint64_t pattern_mask, uint8_t step_count, uint8_t length, bool show_current_step = true);
  void draw_pattern_mask(uint8_t offset, uint8_t device, bool show_current_step = true);
  void draw_knob_frame();
  void draw_knob(uint8_t i, const char* title, const char* text);
  void draw_knob(uint8_t i, Encoder* enc, const char* name);
  void draw_page_index(bool show_page_index = true);
  void select_track(uint8_t device, uint8_t track);

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

  static constexpr uint8_t seq_x0 = 32;
  static constexpr uint8_t led_y = 22;
  static constexpr uint8_t trig_y = 26;

  static constexpr uint8_t knob_x0 = 31;
  static constexpr uint8_t knob_w = 24;
  static constexpr uint8_t knob_xend = 127;
  static constexpr uint8_t knob_y0 = 1;
  static constexpr uint8_t knob_y = 20;
};

#endif /* SEQPAGE_H__ */
