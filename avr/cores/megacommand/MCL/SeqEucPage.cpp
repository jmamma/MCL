#include "SeqEucPage.h"

void SeqEucPage::setup() {
  SeqPage::setup();
  collect_trigs = true;

  trackinfo_param1.max = 64;
  trackinfo_param1.cur = 8;
  trackinfo_param2.max = 64;
  trackinfo_param2.min = 0;
  trackinfo_param2.cur = 0;
  trackinfo_param3.max = 12;
  trackinfo_param3.cur = PatternLengths[last_md_track];
  trackinfo_param4.max = 36;
  curpage = SEQ_EUC_PAGE;
}

bool SeqEucPage::displayPage() {}
bool SeqEucPage::handleEvent(gui_event_t *event) {

  if (note_interface.is_event(event)) {
    return true;
  }


  if ((curpage == SEQ_EUC_PAGE) && EVENT_RELEASED(event, Buttons.BUTTON1))  {
  /*  trackinfo_param1.max = 13;
    trackinfo_param2.max = 23;
    trackinfo_param2.min = 1;
    trackinfo_param2.cur = 12;
    trackinfo_param3.max = 64;
    trackinfo_param4.max = 16;*/
    curpage = SEQ_STEP_PAGE;
    GUI.setPage(&seq_step_page);
    return true;

  }
  if (EVENT_PRESSED(event, Buttons.BUTTON4)) {
    uint8_t notescount = 0;
    for (uint8_t i = 0; i < 16; i++) {
      if (notes[i] == 1) {
        notescount++;
      }
    }
    for (uint8_t i = 0; i < 16; i++) {
      if (notes[i] > 0) {
        if (notescount == 1) {
          setEuclid(i, trackinfo_param1.cur, trackinfo_param3.cur,
                    trackinfo_param2.cur, trackinfo_param4.cur,
                    trackinfo_param3.cur);
        } else {
          random_track(i, trackinfo_param1.cur, PatternLengths[i],
                       trackinfo_param2.cur, trackinfo_param4.cur,
                       trackinfo_param3.cur);
        }
      }
    }

    // setEuclid(last_md_track, trackinfo_param1.cur, trackinfo_param3.cur,
    // trackinfo_param2.cur, euclid_scale, euclid_oct);
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON4) && BUTTON_DOWN(Buttons.BUTTON3)) {
    random_pattern(trackinfo_param1.cur, PatternLengths[last_md_track],
                   trackinfo_param2.cur, trackinfo_param4.cur,
                   trackinfo_param3.cur);

    return true;
  }

    if ((EVENT_PRESSED(evnt, Buttons.BUTTON1) && BUTTON_DOWN(Buttons.BUTTON4)) ||
      (EVENT_PRESSED(evnt, Buttons.BUTTON4) && BUTTON_DOWN(Buttons.BUTTON3))) {

    for (uint8_t n = 0; n < 16; n++) {
      clear_seq_locks(n);
    }
    return true;
  }
   return false;
}
