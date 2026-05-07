#pragma once

#ifdef PLATFORM_TBD

#include "GUI.h"

class TbdPanel {
public:
  void loop();
  bool handleEvent(gui_event_t *event);

private:
  bool top_left_reserved_page() const;
  bool handle_menu_no_hold(gui_event_t *event, bool is_press,
                           bool is_release);
  void reset_menu_no_hold();
  bool handle_primary_ui_button(gui_event_t *event);
  bool handle_secondary_ui_button(gui_event_t *event,
                                  bool allow_toggle = true);
  bool handle_grid_trig_preview(gui_event_t *event, uint8_t trig_idx);
  bool open_bank_popup();
  bool suppress_sps_key_release_ = false;
  bool ui_b_button_held_ = false;
  uint16_t menu_no_hold_start_ms_ = 0;
  bool menu_no_hold_tracking_ = false;
  bool menu_no_hold_opened_ = false;
};

extern TbdPanel tbd_panel;
bool tbd_handleEvent(gui_event_t *event);

#endif // PLATFORM_TBD
