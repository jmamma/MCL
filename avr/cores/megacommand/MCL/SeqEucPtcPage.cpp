#include "SeqEucPtcPage.h"

void SeqEucPtcPage::setup() {
  SeqPage::setup();
  collect_trigs = true;
  encoders[1]->max = 8;
  encoders[2]->max = 64;
  encoders[3]->max = 64;
  encoders[4]->max = 15;
  encoders[2]->cur = 32;
  encoders[1]->cur = 1;

  encoders[3]->cur = PatternLengths[last_md_track];
  curpage = SEQ_EUCPTC_PAGE;
}

bool SeqEucPtcPage::displayPage() {}
bool SeqEucPtcPage::handleEvent(gui_event_t *event) {

  if (note_interface.is_event(event)) {
    return true;
  }
  if ((EVENT_PRESSED(event, Buttons.BUTTON1) && BUTTON_DOWN(Buttons.BUTTON4)) ||
      (EVENT_PRESSED(event, Buttons.BUTTON4) && BUTTON_DOWN(Buttons.BUTTON3))) {

    for (uint8_t n = 0; n < 16; n++) {
      clear_seq_track(n);
    }
  }
  if (SeqPage::handleEvent(event)) {
    return true;
  }

  return false;
}
