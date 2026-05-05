#pragma once

#ifdef PLATFORM_TBD

#include "GUI.h"

class TbdPanel {
public:
  bool handleEvent(gui_event_t *event);

private:
  bool open_bank_popup();
  bool handle_bank_arrow_cycle(gui_event_t *event);
  bool handle_driver_ui_event(gui_event_t *event);
};

extern TbdPanel tbd_panel;
bool tbd_handleEvent(gui_event_t *event);

#endif // PLATFORM_TBD
