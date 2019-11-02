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
  mcl_gui.draw_popup("SAVE", true, 28);
#endif
}

void GridSavePage::cleanup() {}

#ifndef OLED_DISPLAY
void GridSavePage::display() {
  GUI.setLine(GUI.LINE1);
  char strn[17] = "----------------";

  for (int i = 0; i < 16; i++) {

    if (note_interface.notes[i] != 0) {

      strn[i] = (char)219;
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
#else
void GridSavePage::display() {

  mcl_gui.clear_popup(28);

  mcl_gui.draw_trigs(MCLGUI::s_menu_x + 4, MCLGUI::s_menu_y + 21, 0, 0, 0, 16);

  const char* merge = "NO";
  if (encoders[0]->cur == 1) {
    merge = "YES";
  }
  if (MidiClock.state == 2) {
    merge = "---";
  }

  mcl_gui.draw_text_encoder(MCLGUI::s_menu_x + 4, MCLGUI::s_menu_y + 4, "MERGE", merge);

  char step[4] = {'\0'};
  uint8_t step_count =
      (MidiClock.div16th_counter - mcl_actions.start_clock32th / 2) -
      (64 *
       ((MidiClock.div16th_counter - mcl_actions.start_clock32th / 2) / 64));
  itoa(step_count, step, 10);

 // mcl_gui.draw_text_encoder(MCLGUI::s_menu_x + MCLGUI::s_menu_w - 26, MCLGUI::s_menu_y + 4, "STEP", step);

  oled_display.setFont(&TomThumb);
  // draw data flow in the center
  if (MidiClock.state != 2 && encoders[0]->cur == 1) {
    oled_display.setCursor(52, MCLGUI::s_menu_y + 12);
    oled_display.print("MD");
    oled_display.setCursor(50, MCLGUI::s_menu_y + 19);
    oled_display.print("SEQ");

    oled_display.drawFastHLine(63, MCLGUI::s_menu_y + 8, 2, WHITE);
    oled_display.drawFastHLine(63, MCLGUI::s_menu_y + 15, 2, WHITE);
    oled_display.drawFastVLine(65, MCLGUI::s_menu_y + 8, 8, WHITE);
    mcl_gui.draw_horizontal_arrow(66, MCLGUI::s_menu_y + 12, 5);
  } else {
    oled_display.setCursor(50, MCLGUI::s_menu_y + 15);
    oled_display.print("SEQ");
    mcl_gui.draw_horizontal_arrow(63, MCLGUI::s_menu_y + 12, 8);
  }

  oled_display.setCursor(74, MCLGUI::s_menu_y + 15);
  oled_display.print("GRID");


  oled_display.display();
}
#endif

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
