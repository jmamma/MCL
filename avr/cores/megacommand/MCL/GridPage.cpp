#include "GridPage.h"

bool GridPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {

    return true;
  }

  if ((EVENT_PRESSED(event, Buttons.BUTTON1) && BUTTON_DOWN(Buttons.BUTTON4)) ||
      (EVENT_PRESSED(event, Buttons.BUTTON4) && BUTTON_DOWN(Buttons.BUTTON1))) {

    GUI.setPage(&system_page);

    curpage = SYSTEM_PAGE;

    return true;
  }

  return false;
}

bool GridPage::setup() {}
