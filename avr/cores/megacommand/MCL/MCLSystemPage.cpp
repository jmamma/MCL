#include "MCLSystemPage.h"

bool MCLSystemPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {

    return true;
  }
  if (EVENT_RELEASED(evt, Buttons.ENCODER1) ||
      EVENT_RELEASED(evt, Buttons.ENCODER2) ||
      EVENT_RELEASED(evt, Buttons.ENCODER3) ||
      EVENT_RELEASED(evt, Buttons.ENCODER1)) {
    if (options_param1.getValue() == 0) {
      load_project_page();
      return true;
    } else if (options_param1.getValue() == 1) {
      new_project_page();
      return true;
    }
    cfg.write_cfg();
    midi_setup.cfg_ports();

    GUI.setPage(&grid_page);
    curpage = 0;
    return true;
  }

  return false;
}

bool MCLSystemPage::setup() {}
