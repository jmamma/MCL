#include "GridSavePage.h"
#include "MCL.h"
#define S_PAGE 3

void GridSavePage::setup() {
  MD.getCurrentTrack(CALLBACK_TIMEOUT);
  MD.getCurrentPattern(CALLBACK_TIMEOUT);
  encoders[0]->cur = (int)MD.currentPattern / (int)16;
  encoders[1]->cur =
      MD.currentPattern - 16 * ((int)MD.currentPattern / (int)16);
  if ((mcl_cfg.auto_save == 1) && (MidiClock.state != 2)) {
    MD.saveCurrentKit(MD.currentKit);
  }
  md_exploit.on();
  note_interface.state = true;
  curpage = S_PAGE;
  grid_page.reload_slot_models = false;
}

void GridSavePage::init() {}

void GridSavePage::cleanup() {}

void GridSavePage::display() {
  GUI.setLine(GUI.LINE1);
  char strn[17] = "----------------";

  for (int i = 0; i < 16; i++) {

    if (note_interface.notes[i] == 1) {
/*Char 219 on the minicommand LCD is a []*/
#ifdef OLED_DISPLAY
      strn[i] = (char)3;
#else
      strn[i] = (char)255;
#endif
    } else if (note_interface.notes[i] == 3) {

#ifdef OLED_DISPLAY
      strn[i] = (char)2;
#else
      strn[i] = (char)219;
#endif
    }
  }

  GUI.put_string_at(0, strn);

  GUI.setLine(GUI.LINE2);
  GUI.put_string_at(0, "S");

  char str[5];

  if (encoders[0]->getValue() < 8) {
    MD.getPatternName(encoders[0]->getValue() * 16 + encoders[1]->getValue(),
                      str);
    GUI.put_string_at(2, str);
  } else {
    GUI.put_string_at(2, "OG");
  }

  uint8_t step_count =
      (MidiClock.div16th_counter - mcl_actions_callbacks.start_clock32th / 2) -
      (64 * ((MidiClock.div16th_counter -
              mcl_actions_callbacks.start_clock32th / 2) /
             64));
  GUI.put_value_at2(14, step_count);
}
bool GridSavePage::handleEvent(gui_event_t *event) {

  if (note_interface.is_event(event)) {
    if (note_interface.notes_all_off()) {
      if (BUTTON_DOWN(Buttons.BUTTON2)) {
         return true;
      }
      else {
        md_exploit.off();
        mcl_actions.store_tracks_in_mem(0, grid_page.encoders[1]->getValue(),
                                        STORE_IN_PLACE);
      }
      GUI.setPage(&grid_page);

      curpage = 0;
    }
    return true;
  }
  if (GridIOPage::handleEvent(event)) {
    return true;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON2)) {
    md_exploit.off();
    DEBUG_PRINTLN("notes");
    DEBUG_PRINTLN(note_interface.notes_all_off());

    if (note_interface.notes_count() > 0) {
            for (uint8_t i = 0; i < 20; i++) {
        if (note_interface.notes[i] == 1) {
          note_interface.notes[i] = 3;
        }
      }
      mcl_actions.store_tracks_in_mem(grid_page.encoders[0]->getValue(),
                                      grid_page.encoders[1]->getValue(),
                                      STORE_AT_SPECIFIC);

    }
    GUI.setPage(&grid_page);
    curpage = 0;
    return true; 
  }
  if (EVENT_RELEASED(event, Buttons.BUTTON3)) {

    md_exploit.off();
    DEBUG_PRINTLN("notes");
    DEBUG_PRINTLN(note_interface.notes_all_off());

     for (int i = 0; i < 20; i++) {

        note_interface.notes[i] = 3;
      }

      mcl_actions.store_tracks_in_mem(grid_page.encoders[0]->getValue(),
                                      grid_page.encoders[1]->getValue(),
                                      STORE_IN_PLACE);
    GUI.setPage(&grid_page);
    curpage = 0;
    return true;
  }
}
