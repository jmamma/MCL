#pragma once

#include "MCLDefines.h"

#ifdef MCL_HAS_EXTENDED_PANEL_INPUT

#include "GUI.h"

class PlatformPanel {
public:
  void loop();
  bool handleEvent(gui_event_t *event);
};

extern PlatformPanel platform_panel;
bool platform_panel_handleEvent(gui_event_t *event);

#endif // MCL_HAS_EXTENDED_PANEL_INPUT
