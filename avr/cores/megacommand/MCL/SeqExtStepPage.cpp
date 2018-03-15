#include "SeqExtStepPage.h"

void SeqExtStepPage::setup() {
  SeqPage::setup();
  if (ExtPatternResolution[last_ext_track] == 1) {
    encoders[2]->cur = 6;
    encoders[2]->max = 11;
  } else {
    encoders[2]->cur = 12;
    encoders[2]->max = 23;
  }
  encoders[3]->cur = ExtPatternLengths[track];
  cur_col = last_ext_track + 16;
  curpage = SEQ_EXTSTEP_PAGE;
}

bool SeqExtStepPage::display() {}

bool SeqExtStepPage::handleEvent(gui_event_t *event) {
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

if ( EVENT_PRESSED(event, Buttons.BUTTON2) && BUTTON_DOWN(Buttons.BUTTON3)) {
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

  if (EVENT_RELEASED(event, Buttons.BUTTON1))  {
    load_seq_step_page(last_md_track);

    return true;
  }
if (EVENT_RELEASED(event, Buttons.BUTTON4)) {
    clear_Ext_track(last_Ext_track);
    return true;
  }

  if (SeqExtStep::handleEvent(event)) {
    return true;
  }


return false;
}
