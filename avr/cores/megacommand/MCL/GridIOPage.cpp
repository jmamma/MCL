#include "GridIOPage.h"
#include "MCL.h"

bool GridIOPage::handleEvent(gui_event_t *event) {
  if (EVENT_PRESSED(event, Buttons.ENCODER1)) {
    note_interface.notes[param1.getValue()] = 3;
    note_interface.draw_notes(0);
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.ENCODER2)) {
    note_interface.notes[param1.getValue() + 1] = 3;
    note_interface.draw_notes(0);
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.ENCODER3)) {
    note_interface.notes[param1.getValue() + 2] = 3;
    note_interface.draw_notes(0);
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.ENCODER4)) {
    note_interface.notes[param1.getValue() + 3] = 3;
    note_interface.draw_notes(0);
    return true;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON1) ||
      EVENT_RELEASED(event, Buttons.BUTTON4)) {
    GUI.setPage(&grid_page);
    curpage = 0;
    return true;
  }
  return false;
}
