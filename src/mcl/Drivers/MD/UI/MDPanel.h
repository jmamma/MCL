#pragma once

#include "GUI.h"

class MDClass;

class MDPanel {
public:
  explicit MDPanel(MDClass &md) : md_(md) {}

  bool handle_event(gui_event_t *event);

private:
  bool handle_bank_arrow_cycle(gui_event_t *event);
  void handle_grid_trig_preview(gui_event_t *event, uint8_t trig_idx);

  MDClass &md_;
};
