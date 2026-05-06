#pragma once

#ifdef PLATFORM_TBD

#include "GUI.h"

class TbdPanel {
public:
  bool handleEvent(gui_event_t *event);

private:
  bool open_bank_popup();
  bool suppress_sps_key_release_ = false;
};

extern TbdPanel tbd_panel;
bool tbd_handleEvent(gui_event_t *event);

#endif // PLATFORM_TBD
