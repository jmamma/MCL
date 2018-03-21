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
  note_interface.state = true;
  // GUI.display();
  curpage = W_PAGE;
}

bool GridWritePage::display() {
  draw_notes(0);
  GUI.setLine(GUI.LINE2);
  if (curpage == S_PAGE) {
    GUI.put_string_at(0, "S");
  }
  else if (curpage == W_PAGE) {
    GUI.put_string_at(0, "W");
  }

  char str[5];


  if (gridio_param1.getValue() < 8) {
    MD.getPatternName(gridio_param1.getValue() * 16 + gridio_param2.getValue() , str);
    GUI.put_string_at(2, str);
  }
  else {
    GUI.put_string_at(2, "OG");
  }

  uint8_t step_count = (MidiClock.div16th_counter - pattern_start_clock32th / 2) - (64 * ((MidiClock.div16th_counter - pattern_start_clock32th / 2) / 64));
  GUI.put_value_at2(14, step_count);
  if (curpage == W_PAGE) {
    uint8_t x;

    GUI.put_string_at(9, "Q:");

    //0-63 OG
    if (gridio_param3.getValue() == 64) {
      GUI.put_string_at(6, "OG");
    }
    else {
      GUI.put_value_at2(6, gridio_param3.getValue() + 1);
    }


    if (gridio_param4.getValue() == 0) {
      GUI.put_string_at(11, "--");
    }
    if (gridio_param4.getValue() == 7) {
      GUI.put_string_at(11, "CU");
    }
    if (gridio_param4.getValue() == 8) {
      GUI.put_string_at(11, "LV");
    }
    if (gridio_param4.getValue() == 9) {
      GUI.put_string_at(11, "P ");
    }
    if (gridio_param4.getValue() == 10) {
      GUI.put_string_at(11, "P+");
    }
    if (gridio_param4.getValue() == 11) {
      GUI.put_string_at(11, "P-");
    }

    if ((gridio_param4.getValue() < 7) && (gridio_param4.getValue() > 0)) {
      x = 1 << gridio_param4.getValue();
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
  write_tracks_to_md( 0, param2.getValue(), 0);
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
    write_original = 0;
    write_tracks_to_md(MD.currentTrack, param2.getValue(), 254);
    GUI.setPage(&grid_page);
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

    GUI.setPage(&grid_page);
    curpage = 0;
    return true;
  }
}

