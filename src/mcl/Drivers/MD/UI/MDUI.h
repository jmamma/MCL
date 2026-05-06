#pragma once

#include "GUI.h"
#include "MDPanel.h"
#include "SpsMode.h"

class MDClass;

class MDUI {
public:
  explicit MDUI(MDClass &md);

  void loop();
  bool handle_event(gui_event_t *event);
  bool enter(gui_event_t *event);
  bool is_active() const;
  void exit();
  void handle_ui_slot_button(bool pressed);

  SpsMode sps_mode;

private:
  MDPanel panel_;
};
