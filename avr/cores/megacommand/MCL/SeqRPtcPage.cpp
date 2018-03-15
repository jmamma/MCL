#include "SeqRptcPage.h"

bool SeqRptcPage::displayPage() {}
bool SeqRptcPage::handleEvent(gui_event_t *event) {

  if (note_interface.is_event(event)) {
    return true;
  }

 if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
    curpage = SEQ_PTC_PAGE;
    GUI.setPage(&seq_ptc_page);
    return true;

  }
  if (SeqRptcPage::handleEvent(event)) {
    return true;
  }

  return false;
}
