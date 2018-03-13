#include "GridIOPage.h"

bool GridIOPage::handleEvent(gui_event_t *event) {
  if (EVENT_PRESSED(event, Buttons.ENCODER1)) {
    md_exploit.notes[param1.getValue()] = 3;
    md_exploit.draw_notes(0);
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.ENCODER2)) {
    md_exploit.notes[param1.getValue() + 1] = 3;
    md_exploit.draw_notes(0);
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.ENCODER3)) {
    md_exploit.notes[param1.getValue() + 2] = 3;
    md_exploit.draw_notes(0);
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.ENCODER4)) {
    md_exploit.notes[param1.getValue() + 3] = 3;
    md_exploit.draw_notes(0);
    return true;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON1) ||
      EVENT_RELEASED(event, Buttons.BUTTON4)) {
    md_exploit.off();

    GUI.setPage(&page);
    curpage = 0;
    return true;
  }
  return false;
}
