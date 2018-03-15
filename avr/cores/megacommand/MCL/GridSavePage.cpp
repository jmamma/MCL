#include "GridSavePage.h"

void GridSavePage::setup() {
  midi_events.setupCallbacks();
  MD.getCurrentTrack(CALLBACK_TIMEOUT);
  MD.getCurrentPattern(CALLBACK_TIMEOUT);
  patternload_param1.cur = (int)MD.currentPattern / (int)16;
  patternload_param2.cur =
      MD.currentPattern - 16 * ((int)MD.currentPattern / (int)16);
  md_exploit_on();
  collect_trigs = true;
  curpage = S_PAGE;
  reload_slot_models = 0;
}

bool GridSavePage::display() {}
bool GridSavePage::handleEvent(gui_event_t *event) {

  if (note_interface.is_event(event)) {
     md_exploit.off();
        store_tracks_in_mem( 0, param2.getValue(), STORE_IN_PLACE);
        GUI.setPage(&page);
        curpage = 0;
  return true;
  }
  if (GridIOPage::handleEvent(event)) {
    return true;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON2)) {
    for (int i = 0; i < 20; i++) {

      md_exploit.notes[i] = 3;
    }
    md_exploit.off();
    store_tracks_in_mem(param1.getValue(), param2.getValue(), STORE_IN_PLACE);
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

      // MD.getCurrentPattern(CALLBACK_TIMEOUT);
      md_exploit.off();
      store_tracks_in_mem(param1.getValue(), param2.getValue(),
                          STORE_AT_SPECIFIC);
      GUI.setPage(&page);
      curpage = 0;
      return true;
  }
}
