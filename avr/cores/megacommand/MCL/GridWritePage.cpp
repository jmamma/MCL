#include "GridWritePage.h"
#include "MCL.h"

void GridWritePage::setup() {
  MD.getCurrentTrack(CALLBACK_TIMEOUT);
  MD.getCurrentPattern(CALLBACK_TIMEOUT);
  encoders[0]->cur = (int)MD.currentPattern / (int)16;
  encoders[1]->cur =
      MD.currentPattern - 16 * ((int)MD.currentPattern / (int)16);

  patternswitch = 1;
  MD.currentKit = MD.getCurrentKit(CALLBACK_TIMEOUT);
  encoders[2]->cur = MD.currentKit;
  MD.saveCurrentKit(MD.currentKit);

  // MD.requestKit(MD.currentKit);
  md_exploit.on();
  note_interface.state = true;
  // GUI.display();
  curpage = W_PAGE;
}

void GridWritePage::display() {
  note_interface.draw_notes(0);
  GUI.setLine(GUI.LINE2);
  if (curpage == S_PAGE) {
    GUI.put_string_at(0, "S");
  }
  else if (curpage == W_PAGE) {
    GUI.put_string_at(0, "W");
  }

  char str[5];


  if (encoders[1]->getValue() < 8) {
    MD.getPatternName(encoders[0]->getValue() * 16 + encoders[1]->getValue() , str);
    GUI.put_string_at(2, str);
  }
  else {
    GUI.put_string_at(2, "OG");
  }

  uint8_t step_count = (MidiClock.div16th_counter - mcl_actions_callbacks.start_clock32th / 2) - (64 * ((MidiClock.div16th_counter - mcl_actions_callbacks.start_clock32th / 2) / 64));
  GUI.put_value_at2(14, step_count);
  if (curpage == W_PAGE) {
    uint8_t x;

    GUI.put_string_at(9, "Q:");

    //0-63 OG
    if (encoders[2]->getValue() == 64) {
      GUI.put_string_at(6, "OG");
    }
    else {
      GUI.put_value_at2(6, encoders[2]->getValue() + 1);
    }


    if (encoders[3]->getValue() == 0) {
      GUI.put_string_at(11, "--");
    }
    if (encoders[3]->getValue() == 7) {
      GUI.put_string_at(11, "CU");
    }
    if (encoders[3]->getValue() == 8) {
      GUI.put_string_at(11, "LV");
    }
    if (encoders[3]->getValue() == 9) {
      GUI.put_string_at(11, "P ");
    }
    if (encoders[3]->getValue() == 10) {
      GUI.put_string_at(11, "P+");
    }
    if (encoders[3]->getValue() == 11) {
      GUI.put_string_at(11, "P-");
    }

    if ((encoders[3]->getValue() < 7) && (encoders[3]->getValue() > 0)) {
      x = 1 << encoders[3]->getValue();
      GUI.put_value_at2(11, x);
    }


  }
}
bool GridWritePage::handleEvent(gui_event_t *event) {
  // Call parent GUI handler first.
  if (GridIOPage::handleEvent(event)) {
    return true;
  }

  if (note_interface.is_event(event)) {
  md_exploit.off();
  mcl_actions.write_tracks_to_md( 0, grid_page.encoders[1]->getValue(), 0);
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

    // MD.getCurrentTrack(CALLBACK_TIMEOUT);
    int curtrack = last_md_track;
    //        int curtrack = MD.getCurrentTrack(CALLBACK_TIMEOUT);

    md_exploit.off();
    mcl_actions.write_original = 0;
    mcl_actions.write_tracks_to_md(MD.currentTrack, grid_page.encoders[1]->getValue(), 254);
    GUI.setPage(&grid_page);
    curpage = 0;
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
    for (int i = 0; i < 20; i++) {

      note_interface.notes[i] = 3;
    }
    //   write_tracks_to_md(-1);
    md_exploit.off();
    mcl_actions.write_original = 1;
    mcl_actions.write_tracks_to_md(0, grid_page.encoders[1]->getValue(), 0);

    GUI.setPage(&grid_page);
    curpage = 0;
    return true;
  }
}

