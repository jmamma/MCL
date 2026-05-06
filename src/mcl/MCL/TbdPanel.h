#pragma once

#ifdef PLATFORM_TBD

#include "GUI.h"

class TbdPanel {
public:
  void loop();
  bool handleEvent(gui_event_t *event);

private:
  bool top_left_reserved_page() const;
  bool enter_primary_ui(gui_event_t *event);
  bool enter_secondary_ui(gui_event_t *event);
  bool handle_grid_trig_preview(gui_event_t *event, uint8_t trig_idx);
  bool open_bank_popup();
  bool suppress_sps_key_release_ = false;
  uint16_t top_left_press_ms_ = 0;
  bool top_left_pressed_ = false;
  bool top_left_page_select_opened_ = false;
  bool top_left_chorded_ = false;
};

extern TbdPanel tbd_panel;
bool tbd_handleEvent(gui_event_t *event);

#endif // PLATFORM_TBD
