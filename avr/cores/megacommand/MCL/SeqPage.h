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
  static uint8_t length;
  static uint8_t resolution;
  static uint8_t apply;
  static uint8_t ignore_button_release;
  static bool show_track_menu;

  bool recording = false;

  SeqPageMidiEvents seqpage_midi_events;
  SeqPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
          Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {
  }
  virtual bool handleEvent(gui_event_t *event);
  void create_chars_seq();
  void draw_lock_mask(uint8_t offset, bool show_current_step = true);
  void draw_pattern_mask(uint8_t offset, uint8_t device, bool show_current_step = true);
  void loop();
  void display();
  void setup();
  void select_track(uint8_t device, uint8_t track);
  void init();
  void cleanup();

  static constexpr uint8_t pane_x1 = 0;
  static constexpr uint8_t pane_x2 = 30;
  static constexpr uint8_t pane_w = pane_x2 - pane_x1;
  static constexpr uint8_t label_x = 0;
  static constexpr uint8_t label_md_y = 0;
  static constexpr uint8_t label_ex_y = 7;
  static constexpr uint8_t label_w = 13;
  static constexpr uint8_t label_h = 7;

  static constexpr uint8_t trackid_x = 15;
  static constexpr uint8_t trackid_y = 8;

  static constexpr uint8_t cir_x1 = 22;
  static constexpr uint8_t cir_x2 = 25;
  static constexpr uint8_t tri_x = 16;
  static constexpr uint8_t tri_y = 9;

  static constexpr uint8_t pidx_x0 = 1;
  static constexpr uint8_t pidx_y = 15;
  static constexpr uint8_t pidx_w = 6;
  static constexpr uint8_t pidx_h = 3;
};

#endif /* SEQPAGE_H__ */
