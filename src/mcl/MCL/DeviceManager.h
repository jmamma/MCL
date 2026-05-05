#pragma once

#include "platform.h"

#ifdef PLATFORM_TBD
#include "GUI.h"
#endif

class DeviceManager {
public:
#ifdef PLATFORM_TBD
  void ui_loop();
  bool handle_ui_event(gui_event_t *event);
  bool is_ui_active() const;
  void mark_tr_consumed();
  void mark_b_consumed();
#endif
};

extern DeviceManager device_manager;
