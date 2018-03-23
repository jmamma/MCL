#include "GridSavePage.h"
#include "MCL.h"
#define S_PAGE 3


void GridSavePage::setup() {
  MD.getCurrentTrack(CALLBACK_TIMEOUT);
  MD.getCurrentPattern(CALLBACK_TIMEOUT);
  encoders[1]->cur = (int)MD.currentPattern / (int)16;
  encoders[2]->cur =
      MD.currentPattern - 16 * ((int)MD.currentPattern / (int)16);
  md_exploit.on();
  note_interface.state = true;
  curpage = S_PAGE;
  grid_page.reload_slot_models = 0;
}

void GridSavePage::display() {
  note_interface.draw_notes(0);
  GUI.setLine(GUI.LINE2);
    GUI.put_string_at(0, "S");

  char str[5];


  if (encoders[1]->getValue() < 8) {
    MD.getPatternName(encoders[1]->getValue() * 16 + encoders[2]->getValue() , str);
    GUI.put_string_at(2, str);
  }
  else {
    GUI.put_string_at(2, "OG");
  }

  uint8_t step_count = (MidiClock.div16th_counter - mcl_actions_callbacks.start_clock32th / 2) - (64 * ((MidiClock.div16th_counter - mcl_actions_callbacks.start_clock32th / 2) / 64));
  GUI.put_value_at2(14, step_count);

}
bool GridSavePage::handleEvent(gui_event_t *event) {

  if (note_interface.is_event(event)) {
    md_exploit.off();
    mcl_actions.store_tracks_in_mem(0, param2.getValue(), STORE_IN_PLACE);
    GUI.setPage(&grid_page);
    curpage = 0;
    return true;
  }
  if (GridIOPage::handleEvent(event)) {
    return true;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON2)) {
    for (int i = 0; i < 20; i++) {

      note_interface.notes[i] = 3;
    }
    md_exploit.off();
    mcl_actions.store_tracks_in_mem(param1.getValue(), param2.getValue(), STORE_IN_PLACE);
    GUI.setPage(&grid_page);
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
    mcl_actions.store_tracks_in_mem(param1.getValue(), param2.getValue(),
                        STORE_AT_SPECIFIC);
    GUI.setPage(&grid_page);
    curpage = 0;
    return true;
  }
}
