#include "SeqStepPage.h"

void SeqStepPage::setup() {
  SeqPage::setup();
  collect_trigs = true;

  trackinfo_param1.max = 13;
  trackinfo_param2.max = 23;
  trackinfo_param2.min = 1;
  trackinfo_param2.cur = 12;
  trackinfo_param3.max = 64;
  trackinfo_param4.max = 16;
  trackinfo_param3.cur = PatternLengths[last_md_track];

  curpage = SEQ_STEP_PAGE;
}

bool SeqStepPage::displayPage() {}

bool SeqStepPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.ENCODER1) {
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
  if (EVENT_RELEASED(event, Buttons.BUTTON4)) {
    clear_seq_track(last_md_track);
    return true;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON1))  {
    load_seq_extstep_page(last_Ext_track);

    return true;
    /*

      return true;
    */


  }
  if (SeqStep::handleEvent(event)) {
    return true;
  }


return false;
}
