#include "MCL_impl.h"

uint16_t GridIOPage::track_type_select = 0xF;
bool GridIOPage::show_track_type_select = false;

bool GridIOPage::handleEvent(gui_event_t *event) {
  if (EVENT_PRESSED(event, Buttons.ENCODER1) ||
      EVENT_PRESSED(event, Buttons.ENCODER2) ||
      EVENT_PRESSED(event, Buttons.ENCODER3) ||
      EVENT_PRESSED(event, Buttons.ENCODER4) ||
      EVENT_RELEASED(event, Buttons.BUTTON1) ||
      EVENT_RELEASED(event, Buttons.BUTTON4)) {
    GUI.setPage(&grid_page);
    curpage = 0;
    return true;
  }
  return false;
}
