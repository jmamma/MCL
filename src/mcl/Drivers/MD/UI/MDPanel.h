#pragma once

#include "GUI.h"

class MDClass;

class MDPanel {
public:
  explicit MDPanel(MDClass &md) : md_(md) {}

  bool handle_event(gui_event_t *event);

private:
  MDClass &md_;
};
