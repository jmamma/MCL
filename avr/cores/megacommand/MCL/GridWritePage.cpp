#include "GridWritePage.h"

void GridWritePage::setup() {
  MD.getCurrentTrack(CALLBACK_TIMEOUT);
  MD.getCurrentPattern(CALLBACK_TIMEOUT);
  patternload_param1.cur = (int)MD.currentPattern / (int)16;
  patternload_param2.cur =
      MD.currentPattern - 16 * ((int)MD.currentPattern / (int)16);

  patternswitch = 1;
  currentkit_temp = MD.getCurrentKit(CALLBACK_TIMEOUT);
  patternload_param3.cur = currentkit_temp;
  MD.saveCurrentKit(currentkit_temp);

  // MD.requestKit(currentkit_temp);
  md_exploit.on();
  collect_trigs = true;
  // GUI.display();
  curpage = W_PAGE;
}

bool GridWritePage::displayPage() {}
bool GridWritePage::handleEvent(gui_event_t *event) {
  // Call parent GUI handler first.
  if (GridIOPage::handleEvent(event)) {
    return true;
  }

  if (note_interface.is_event(event)) {
  md_exploit.off();
  write_tracks_to_md( 0, param2.getValue(), 0);
  GUI.setPage(&page);
  curpage = 0;
  return true;
  }

  if ((EVENT_RELEASED(event, Buttons.ENCODER1) ||
       EVENT_RELEASED(event, Buttons.ENCODER2) ||
       EVENT_RELEASED(event, Buttons.ENCODER3) ||
       EVENT_RELEASED(event, Buttons.ENCODER4)) &&
      (BUTTON_UP(Buttons.ENCODER1) && BUTTON_UP(Buttons.ENCODER2) &&
       BUTTON_UP(Buttons.ENCODER3) && BUTTON_UP(Buttons.ENCODER4))) {

    // MD.getCurrentTrack(CALLBACK_TIMEOUT);
    int curtrack = last_md_track;
    //        int curtrack = MD.getCurrentTrack(CALLBACK_TIMEOUT);

    md_exploit.off();
    write_original = 0;
    write_tracks_to_md(MD.currentTrack, param2.getValue(), 254);
    GUI.setPage(&page);
    curpage = 0;
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
    for (int i = 0; i < 20; i++) {

      md_exploit.notes[i] = 3;
    }
    trackposition = TRUE;
    //   write_tracks_to_md(-1);
    md_exploit.off();
    write_original = 1;
    write_tracks_to_md(0, param2.getValue(), 0);

    GUI.setPage(&page);
    curpage = 0;
    return true;
  }
}

