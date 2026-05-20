/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SEQPAGE_H__
#define SEQPAGE_H__

#include "GUI.h"
#include "midi-common.h"
#include "Elektron.h"
#include "Shared.h"
#include "MidiClock.h"
#include "Midi.h"
#include "../Drivers/DeviceContext.h"

#define REC_EVENT_TRIG 0
#define REC_EVENT_CC 1
#define PAGE_UNDO 64
#define STEP_UNDO 65

class SeqPageMidiEvents : public MidiCallback, public ClockCallback {
public:
  void setup_callbacks();
  void remove_callbacks();
  void onMidiStartCallback();
};

extern MidiDevice *opt_midi_device_capture;
extern void pattern_len_handler(EncoderParent *enc);

extern uint8_t opt_trackid;
extern uint8_t opt_speed;
extern uint8_t opt_copy;
extern uint8_t opt_paste;
extern uint8_t opt_clear;
extern uint8_t opt_shift;
extern uint8_t opt_reverse;
extern uint8_t opt_transpose;
extern uint8_t opt_lfo_mult;
extern uint8_t opt_clear_step;
extern uint8_t opt_length;
extern uint8_t opt_channel;
extern uint8_t opt_undo;
extern uint8_t opt_undo_track;

extern uint16_t trigled_mask;
extern uint16_t locks_on_step_mask;

extern void opt_trackid_handler();
extern void opt_speed_handler();
extern void opt_clear_track_handler();
extern void opt_clear_locks_handler();
extern void opt_copy_track_handler_cb();
extern void opt_copy_track_handler(uint8_t op = 255);
extern void opt_paste_track_handler();
extern void opt_shift_track_handler();
extern void opt_reverse_track_handler();
extern void opt_transpose_track_handler();
extern void opt_paste_step_handler();
extern void opt_copy_step_handler_cb();
extern void opt_copy_step_handler(uint8_t op = 255);
extern void opt_mute_step_handler();
extern void opt_clear_step_locks_handler();
extern void opt_mask_handler();
extern void opt_length_handler();
extern void opt_channel_handler();
extern void opt_clear_page_handler();
extern void opt_copy_page_handler_cb();
extern void opt_copy_page_handler(uint8_t op = 255);
extern void opt_paste_page_handler();
extern void opt_clear_step_handler();

extern void seq_menu_handler();
extern uint8_t copy_mask;

extern void seq_copy_note_label(uint8_t note_num, char *out);
extern void seq_copy_note_name(uint8_t note, char *out);

class MidiDevice;

extern inline void reset_undo() {
  opt_undo = 255;
  opt_undo_track = 255;
}

class SeqPage : public LightPage {
public:
  // Static variables shared amongst derived objects
  static uint8_t page_select;
  static uint8_t page_count;
  static uint8_t step_select;
  static uint8_t mask_type;
  static uint8_t param_select;
  static uint8_t last_pianoroll_mode;
  static uint8_t pianoroll_mode;
  static uint8_t velocity;
  static uint8_t cond;
  static uint8_t slide;
  static uint8_t md_micro;

  static MidiDevice* midi_device;

  static bool show_seq_menu;
  static bool toggle_device;

  static uint8_t last_midi_state;
  static uint8_t last_param_id;
  static uint8_t last_rec_event;
  static uint8_t last_step;
  static uint32_t last_md_model;
  static bool is_midi_model;

  static bool recording;
  static uint16_t mute_mask;

  static bool microtiming_overlay_active() { return md_micro; }
  static void set_microtiming_overlay_active(bool active) {
    md_micro = active;
  }

  bool display_page_index = true;
  char info1[8];
  char info2[8];
  uint8_t timeout_values[4]; // 255 == highlight

  SeqPageMidiEvents seqpage_midi_events;
  SeqPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
          Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {
  }

  void toggle_record();
  void disable_record();
  void enable_record();

  void check_and_set_page_select();
  void config_as_trackedit();
  void config_as_lockedit();
  void config_mask_info(bool silent = true);
  void shed_mask(uint64_t &mask, uint8_t length, uint8_t offset);
  void draw_lock_mask(const uint8_t offset, const uint64_t &lock_mask, const uint8_t step_count, const uint8_t length, const bool show_current_step = true);
  void draw_lock_mask(const uint8_t offset, const bool show_current_step = true);
  void draw_mask(const uint8_t offset, const uint64_t &pattern_mask,
                 const uint8_t step_count, const uint8_t length,
                 const uint64_t &mute_mask, const uint64_t &slide_mask);
  void draw_mask(const uint8_t offset, const uint8_t device, const bool show_current_step = true);
  void draw_knob_frame();
  void draw_knob(uint8_t i, const char* title, const char* text);
  void draw_knob(uint8_t i, Encoder* enc, const char* name);
  void conditional_str(char *str, uint8_t cond, bool is_md = false);
  void draw_knob_conditional(uint8_t cond);
  void draw_knob_timing(uint8_t timing, uint8_t timing_mid);

  static MidiDevice *device_for_seq_idx(DeviceIdx device_idx);
  static bool devices_share_physical();
  static DeviceIdx current_device_idx();
  static bool idx_is_md_device(DeviceIdx device_idx);
  static bool device_is_md(MidiDevice *device);
  static bool active_device_is_md();
  static void select_device_idx(DeviceIdx device_idx);

  void draw_page_index(bool show_page_index = true, uint8_t _playing_idx = 255);
  void select_track(MidiDevice* device, uint8_t track, bool send = true);
  bool md_track_change_check();

  void bootstrap_record();
  uint8_t translate_to_step_conditional(uint8_t condition, /*OUT*/ bool* plock);
  uint8_t translate_to_knob_conditional(uint8_t condition, /*IN*/ bool plock);

  uint64_t *get_mask();

  void toggle_ext_mask(uint8_t track);

  void params_reset();
  void length_handler(uint8_t length, bool multi = false);

  bool display_mute_mask(MidiDevice *device, uint8_t offset = 0);

  virtual bool handleEvent(gui_event_t *event);
  virtual void loop();
  virtual void display();
  virtual void setup();
  virtual void init();
  virtual void cleanup();
  virtual void config_encoders() = 0;
  static constexpr uint8_t pidx_x0 = 0;
  static constexpr uint8_t pidx_y = 15;
  static constexpr uint8_t pidx_w = 6;
  static constexpr uint8_t pidx_h = 3;

protected:
  virtual void capture_seq_menu_values(bool is_md_device);
  virtual void apply_seq_menu_values(bool same_slot);
  virtual bool apply_seq_menu_row(uint8_t row_entry, void (*row_func)());

};

#endif /* SEQPAGE_H__ */
