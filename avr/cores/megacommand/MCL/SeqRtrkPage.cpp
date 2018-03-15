#include "SeqRtrkPage.h"

void SeqRtrkPage::setup() {
  SeqPage::setup();
  collect_trigs = false;

  trackinfo_param1.max = 4;
  trackinfo_param2.max = 64;
  trackinfo_param3.max = 64;
  trackinfo_param4.max = 11;
  trackinfo_param3.cur = PatternLengths[last_md_track];

  curpage = SEQ_RTRK_PAGE;
}

bool SeqRtrkPage::displayPage() {}
bool SeqRtrkPage::handleEvent(gui_event_t *event) {

  if (note_interface.is_event(event)) {
    return true;
  }
  if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
    md_exploit.off();
    curpage = SEQ_RLCK_PAGE;
    GUI.setPage(&seq_rlck_page);
    return true;

  }

  if (EVENT_PRESSED(event, Buttons.ENCODER2)) {
    md_exploit.off();
    GUI.setPage(&grid_page);
    curpage = GRID_PAGE;
    return true;
  }

  if ((EVENT_PRESSED(event, Buttons.BUTTON1) && BUTTON_DOWN(Buttons.BUTTON4)) ||
      (EVENT_PRESSED(event, Buttons.BUTTON4) && BUTTON_DOWN(Buttons.BUTTON3))) {

    for (uint8_t n = 0; n < 16; n++) {
      clear_seq_track(n);
    }
  return true;
  }


   if (EVENT_RELEASED(Event, Buttons.BUTTON4)) {
    if (cur_col < 16) {
      clear_seq_track(cur_col);
    }
    else {
      clear_Ext_track(cur_col - 16);
    }
    return true;

  }

  if (SeqPage::handleEvent(event)) {
    return true;
  }


  return false;
}
SeqRtrkPage seq_rtrk_page;
