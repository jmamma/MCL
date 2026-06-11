/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SEQSTEPPAGE_H__
#define SEQSTEPPAGE_H__

#include "MCLDefines.h"
#include "SeqPage.h"

class SeqStepMidiEvents : public MidiCallback {
public:
  bool state;
  void onControlChangeCallback_Midi(uint8_t *msg);
  void setup_callbacks();
  void remove_callbacks();
};

class SeqStepPage : public SeqPage {

public:
  bool show_pitch;
  bool reset_on_release;
  bool update_params_queue;
  bool prepare;
  bool page_copy;
  bool return_to_grid_on_mask_close;
  uint8_t suppress_mask_shortcut_mask;

  uint8_t pitch_param;
  uint16_t ignore_release;
#ifdef MCL_HAS_EXTENDED_PANEL_INPUT
  uint16_t shift_select_latch;
#endif
  uint16_t update_params_clock;
  uint8_t last_param_id;
  uint8_t last_rec_event;
  PageIndex last_page = NULL_PAGE;
  SeqStepMidiEvents midi_events;
  SeqStepPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
              Encoder *e4 = NULL)
      : SeqPage(e1, e2, e3, e4),
        return_to_grid_on_mask_close(false),
        suppress_mask_shortcut_mask(255) {
      }
  void disable_microtiming_overlay();
  void enable_paramupdate_events();
  void disable_paramupdate_events();
  virtual bool handleEvent(gui_event_t *event);
  virtual void display();
  virtual void init();
  virtual void config();
  virtual void config_encoders();
  virtual void loop();
  virtual void cleanup();
  void send_locks(uint8_t step);
  bool close_mask_mode();
  bool toggle_mask(uint8_t mask, bool func_down);
  bool should_suppress_mask_shortcut(uint8_t mask);
  void clear_mask_shortcut_suppress();
};

#endif /* SEQSTEPPAGE_H__ */
