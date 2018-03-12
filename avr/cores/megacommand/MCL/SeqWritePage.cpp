#include "SeqPages.hh"

bool SeqWritePage::displayPage() {

  draw_notes(0);
  GUI.setLine(GUI.LINE2);
  GUI.put_string_at(0, "W");

  char str[5];

  if (encoders[1]->getValue() < 8) {
    MD.getPatternName(encoders[1]->getValue() * 16 + encoders[2]->getValue(),
                      str);
    GUI.put_string_at(2, str);
  } else {
    GUI.put_string_at(2, "OG");
  }
  uint8_t x;

  GUI.put_string_at(9, "Q:");

  // 0-63 OG
  if (encoders[3]->getValue() == 64) {
    GUI.put_string_at(6, "OG");
  } else {
    GUI.put_value_at2(6, encoders[3]->getValue() + 1);
  }

  if (encoders[4]->getValue() == 0) {
    GUI.put_string_at(11, "--");
  }
  if (encoders[4]->getValue() == 7) {
    GUI.put_string_at(11, "CU");
  }
  if (encoders[4]->getValue() == 8) {
    GUI.put_string_at(11, "LV");
  }
  if (encoders[4]->getValue() == 9) {
    GUI.put_string_at(11, "P ");
  }
  if (encoders[4]->getValue() == 10) {
    GUI.put_string_at(11, "P+");
  }
  if (encoders[4]->getValue() == 11) {
    GUI.put_string_at(11, "P-");
  }

  if ((encoders[4]->getValue() < 7) && (encoders[4]->getValue() > 0)) {
    x = 1 << encoders[4]->getValue();
    GUI.put_value_at2(11, x);
  }
}

bool SeqWritePage::handleEvent(gui_event_t *event) {
  if ((EVENT_RELEASED(evt, Buttons.ENCODER1) ||
       EVENT_RELEASED(evt, Buttons.ENCODER2) ||
       EVENT_RELEASED(evt, Buttons.ENCODER3) ||
       EVENT_RELEASED(evt, Buttons.ENCODER4)) &&
      (BUTTON_UP(Buttons.ENCODER1) && BUTTON_UP(Buttons.ENCODER2) &&
       BUTTON_UP(Buttons.ENCODER3) && BUTTON_UP(Buttons.ENCODER4))) {
    // MD.getCurrentTrack(CALLBACK_TIMEOUT);
    int curtrack = last_md_track;
    //        int curtrack = MD.getCurrentTrack(CALLBACK_TIMEOUT);

    exploit_off();
    write_original = 0;
    write_tracks_to_md(MD.currentTrack, param2.getValue(), 254);
    GUI.setPage(&page);
    curpage = 0;
    return true;
  }

  if (EVENT_PRESSED(evt, Buttons.BUTTON3)) {
    for (int i = 0; i < 20; i++) {

      notes[i] = 3;
    }
    trackposition = TRUE;
    //   write_tracks_to_md(-1);
    exploit_off();
    write_original = 1;
    write_tracks_to_md(0, param2.getValue(), 0);

    GUI.setPage(&page);
    curpage = 0;
    return true;
  }
  return false;
}
