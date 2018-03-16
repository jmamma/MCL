#include "SeqRptcPage.h"

bool SeqRptcPage::display() {
 SeqPtcPage::display();
      GUI.put_string_at(0, "RPTC");

}
bool SeqRptcPage::handleEvent(gui_event_t *event) {

  if (note_interface.is_event(event)) {
    return true;
  }

 if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
    curpage = SEQ_PTC_PAGE;
    GUI.setPage(&seq_ptc_page);
    return true;

  }
  if (SeqPtcPage::handleEvent(event)) {
    return true;
  }

  return false;
}
