#include "SeqPtcPage.h"

void SeqPtcPage::setup() {
  SeqPage::setup();
  collect_trigs = false;

  encoders[1]->max = 8;
  encoders[2]->max = 64;
  encoders[3]->max = 64;
  encoders[4]->max = 15;
  encoders[2]->cur = 32;
  encoders[1]->cur = 1;

  encoders[3]->cur = PatternLengths[last_md_track];

  curpage = SEQ_PTC_PAGE;
}

bool SeqPtcPage::display() {}
bool SeqPtcPage::handleEvent(gui_event_t *event) {

  if (note_interface.is_event(event)) {
    return true;
  }

 if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
    curpage = SEQ_RPTC_PAGE;
    GUI.setPage(&seq_rptc_page);
    return true;
 }

  if (EVENT_PRESSED(event, Buttons.ENCODER4)) {
    md_exploit.off();
    GUI.setPage(&grid_page);
    curpage = GRID_PAGE;
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.ENCODER1)) {

    load_seq_page(SEQ_STEP_PAGE);
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.ENCODER2)) {

    load_seq_page(SEQ_RTRK_PAGE);

    return true;
  }
  if (EVENT_PRESSED(event, Buttons.ENCODER3)) {
    load_seq_page(SEQ_PARAM_A_PAGE);

    return true;
  }
  if (EVENT_PRESSED(event, Buttons.ENCODER4)) {

    load_seq_page(SEQ_PTC_PAGE);

    return true;
  }
  if ((EVENT_PRESSED(event, Buttons.BUTTON1) && BUTTON_DOWN(Buttons.BUTTON4)) ||
      (EVENT_PRESSED(event, Buttons.BUTTON4) && BUTTON_DOWN(Buttons.BUTTON3))) {

    for (uint8_t n = 0; n < 6; n++) {
      clear_Ext_track(n);
    }

    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON2) && BUTTON_DOWN(Buttons.BUTTON3)) {
    if (ExtPatternResolution[last_ext_track] == 1) {
      ExtPatternResolution[last_ext_track] = 2;
      if (curpage == SEQ_EXTSTEP_PAGE) {
        load_seq_extstep_page(last_ext_track);
      }

    } else {
      ExtPatternResolution[last_ext_track] = 1;
      if (curpage == SEQ_EXTSTEP_PAGE) {
        load_seq_extstep_page(last_ext_track);
      }
    }

    return true;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON4)) {

    if (cur_col < 16) {
      clear_seq_track(last_md_track);
    } else {
      clear_Ext_track(last_Ext_track);
    }
    return true;
  }

  if (SeqPage::handleEvent(event)) {
    return true;
  }

  return false;
}
