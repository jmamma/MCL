#pragma once

#ifdef PLATFORM_TBD

#include "GUI.h"

class TbdPanel {
public:
  void loop();
  bool handleEvent(gui_event_t *event);

private:
  bool top_left_button_consumed_page(uint8_t pg) const;
  void open_page_select();
  bool handle_primary_ui_button(gui_event_t *event);
  bool handle_secondary_ui_button(gui_event_t *event,
                                  bool allow_toggle = true);
  bool open_secondary_ui_from_tap(gui_event_t *event);
  bool handle_active_ui_button(gui_event_t *event, uint8_t orig_src);
  bool handle_grid_trig_preview(gui_event_t *event, uint8_t trig_idx);
  bool handle_mixer_mute_yes_no(gui_event_t *event, uint8_t orig_src);
  bool open_bank_popup();
  bool suppress_sps_key_release_ = false;
  bool ui_b_button_held_ = false;
  bool mixer_yes_button_down_ = false;
  bool mixer_no_button_down_ = false;
  bool active_ui_button_pressed_ = false;
  bool active_ui_button_chorded_ = false;
  bool active_ui_button_exit_on_tap_ = false;
  uint8_t active_ui_button_source_ = 255;
  uint16_t active_ui_button_press_ms_ = 0;
  uint8_t ui_display_chord_source_ = 255;
  uint8_t ui_display_chord_modifier_ = 255;
  bool grid_select_button_down_ = false;
  bool grid_select_button_chorded_ = false;
  bool tempo_track_select_down_ = false;
  uint8_t top_left_page_select_base_page_ = 255;
  bool suppress_top_left_release_ = false;
};

extern TbdPanel tbd_panel;
bool tbd_handleEvent(gui_event_t *event);

#endif // PLATFORM_TBD
