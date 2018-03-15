#include "SeqParamPage.h"

void SeqParamPage::setup() {
  SeqPage::setup();
  collect_trigs = true;

  encoders[1]->max = 23;
  encoders[2]->max = 127;
  encoders[3]->max = 23;
  encoders[4]->max = 127;

  encoders[1]->cur = PatternLocksParams[last_md_track][0];
  encoders[3]->cur = PatternLocksParams[last_md_track][1];
  encoders[2]->cur =
      MD.kit.params[last_md_track][PatternLocksParams[last_md_track][0]];
  encoders[4]->cur =
      MD.kit.params[last_md_track][PatternLocksParams[last_md_track][1]];

  curpage = SEQ_PARAMA_PAGE;
}
void SeqParamPage::init(uint8_t p1, uint8_t p2) {
  param1 = p1;
  param2 = p2;
}
bool SeqParamPage::display() {}
bool SeqParamPage::handleEvent(gui_event_t *event) {

  if (note_interface.is_event(event)) {
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.ENCODER3)) {
    md_exploit.off();
    GUI.setPage(&grid_page);
    curpage = GRID_PAGE;
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON4)) {
    clear_seq_track(cur_col);
    return true;
  }

  if ((EVENT_PRESSED(evnt, Buttons.BUTTON1) && BUTTON_DOWN(Buttons.BUTTON4)) ||
      (EVENT_PRESSED(evnt, Buttons.BUTTON4) && BUTTON_DOWN(Buttons.BUTTON3))) {

    for (uint8_t n = 0; n < 16; n++) {
      clear_seq_locks(n);
    }
    return true;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
    encoders[1]->cur = PatternLocksParams[last_md_track][p1];
    encoders[3]->cur = PatternLocksParams[last_md_track][p2];
    encoders[2]->cur =
        MD.kit.params[last_md_track][PatternLocksParams[last_md_track][p1]];
    encoders[4]->cur =
        MD.kit.params[last_md_track][PatternLocksParams[last_md_track][p2]];
    curpage = SEQ_PARAM_B_PAGE;
    return true;
  }
  if (EVENT_RELEASED(event, Buttons.BUTTON4)) {
    clear_seq_locks(cur_col);
    return true;
  }

  if (SeqPage::handleEvent(event)) {
    return true;
  }

  return false;
}
