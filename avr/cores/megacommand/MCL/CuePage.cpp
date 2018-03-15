#include "CuePage.h"

void CuePage::display() {
  GUI.setLine(GUI.LINE2);

  // GUI.put_string_at(12,"Cue");
  GUI.put_string_at(0, "CUES     ");

  GUI.put_string_at(9, "Q:");

  if (encoders[4]->getValue() == 0) {
    GUI.put_string_at(11, "--");
  } else if (encoders[4]->getValue() == 7) {
    GUI.put_string_at(11, "LV");
  } else {
    x = 1 << encoders[4]->getValue();

    GUI.put_value_at2(11, x);
  }
  uint8_t step_count =
      (MidiClock.div16th_counter - pattern_start_clock32th / 2) -
      (64 * ((MidiClock.div16th_counter - pattern_start_clock32th / 2) / 64));
  GUI.put_value_at2(14, step_count);

  note_interface.draw_notes(0);
}
bool CuePage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {

    note_interface.draw_notes(0);
    if (note_interface.notes_all_off()) {
      if ((curpage == CUE_PAGE) && (encoders[4]->getValue() > 0) &&
          (note_noteinterface.notes_count_off() > 1)) {
        toggle_cues_batch();
        md_exploit.send_globals();
        md_exploit.off();
        GUI.setPage(&page);
        curpage = 0;
      }
    }
    return true;
  }
  if (EVENT_PRESSED(evt, Buttons.BUTTON1)) {
    curpage = MIXER_PAGE;
    return true;
  }
  return false;
}
