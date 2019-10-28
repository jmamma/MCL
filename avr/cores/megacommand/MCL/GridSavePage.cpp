#include "GridSavePage.h"
#include "MCL.h"
#define S_PAGE 3

void GridSavePage::setup() {
  MD.getCurrentTrack(CALLBACK_TIMEOUT);
  MD.getCurrentPattern(CALLBACK_TIMEOUT);
  encoders[0]->cur = 1;
  md_exploit.on();
  note_interface.state = true;
  curpage = S_PAGE;
  grid_page.reload_slot_models = false;
}

void GridSavePage::init() {
#ifdef OLED_DISPLAY
  oled_display.clearDisplay();
#endif
}

void GridSavePage::cleanup() {}

void GridSavePage::display() {
  GUI.setLine(GUI.LINE1);
  char strn[17] = "----------------";

  for (int i = 0; i < 16; i++) {

    if (note_interface.notes[i] != 0) {

#ifdef OLED_DISPLAY
      strn[i] = (char)2;
#else
      strn[i] = (char)219;
#endif
    }
  }

  GUI.put_string_at(0, strn);

  GUI.setLine(GUI.LINE2);
  GUI.put_string_at_fill(0, "S");

  if ((MidiClock.state != 2) && (encoders[0]->cur == 1)) {
    GUI.put_string_at(3, "MERGE");
  } else {
    GUI.put_string_at(3, "--");
  }

  uint8_t step_count =
      (MidiClock.div16th_counter - mcl_actions.start_clock32th / 2) -
      (64 *
       ((MidiClock.div16th_counter - mcl_actions.start_clock32th / 2) / 64));
  GUI.put_value_at2(14, step_count);
}

bool GridSavePage::handleEvent(gui_event_t *event) {

  if (note_interface.is_event(event)) {
    if (note_interface.notes_all_off()) {
      if (BUTTON_DOWN(Buttons.BUTTON2)) {
        return true;
      } else {
        md_exploit.off();
        bool merge = (encoders[0]->cur == 1);
        mcl_actions.store_tracks_in_mem(0, grid_page.encoders[1]->getValue(),
                                        merge);
      }
      GUI.setPage(&grid_page);

      curpage = 0;
    }
    return true;
  }
  if (GridIOPage::handleEvent(event)) {
    return true;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON3)) {

    md_exploit.off();
    DEBUG_PRINTLN("notes");
    DEBUG_DUMP(note_interface.notes_all_off());

    for (int i = 0; i < 20; i++) {

      note_interface.notes[i] = 3;
    }
    bool merge = (encoders[0]->cur == 1);
    mcl_actions.store_tracks_in_mem(grid_page.encoders[0]->getValue(),
                                    grid_page.encoders[1]->getValue(), merge);
    GUI.setPage(&grid_page);
    curpage = 0;
    return true;
  }
}
