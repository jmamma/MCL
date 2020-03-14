#include "GridWritePage.h"
#include "MCL.h"

void GridWritePage::setup() {
  MD.getCurrentTrack(CALLBACK_TIMEOUT);
  MD.getCurrentPattern(CALLBACK_TIMEOUT);
  MD.currentKit = MD.getCurrentKit(CALLBACK_TIMEOUT);
  encoders[0]->cur = MD.currentPattern / 16;
  encoders[1]->cur =
      MD.currentPattern - 16 * (MD.currentPattern / 16);

  patternswitch = 1;
  ((MCLEncoder *)encoders[3])->max = 6;
  if (mode == WRITE_PAGE) {
    encoders[3]->cur = 4;
    mode = CHAIN_PAGE;
  }
  ((MCLEncoder *)encoders[2])->max = 1;

  // MD.requestKit(MD.currentKit);
  trig_interface.on();
  note_interface.state = true;
  // GUI.display();
  curpage = W_PAGE;
}

void GridWritePage::init() {
#ifdef OLED_DISPLAY
  mcl_gui.draw_popup("CHAIN FROM GRID", true, 28);
#endif
}

void GridWritePage::cleanup() {}

#ifndef OLED_DISPLAY
void GridWritePage::display() {

  GUI.setLine(GUI.LINE1);
  char strn[17] = "----------------";

  for (uint8_t i = 0; i < 16; i++) {

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
  if (mcl_cfg.chain_mode > 0) {
    GUI.put_string_at(0, "CHAIN");
  } else {
    GUI.put_string_at(0, "W");

    char str[5];

    if (encoders[1]->getValue() < 16) {
      MD.getPatternName(encoders[0]->getValue() * 16 + encoders[1]->getValue(),
                        str);
      GUI.put_string_at(2, str);
    } else {
      GUI.put_string_at(2, "OG");
    }
  }
  uint8_t step_count =
      (MidiClock.div16th_counter - mcl_actions.start_clock32th / 2) -
      (64 *
       ((MidiClock.div16th_counter - mcl_actions.start_clock32th / 2) / 64));
  GUI.put_value_at2(14, step_count);
  uint8_t x;
  /*
    if (encoders[2]->getValue() == 0) {
      GUI.put_string_at(6, "--");
    }
 */

  GUI.put_string_at(9, "Q:");

  if ((encoders[3]->getValue() < 7) && (encoders[3]->getValue() > 0)) {
    x = 1 << encoders[3]->getValue();
    GUI.put_value_at2(11, x);
  }
}
#else
void GridWritePage::display() {

  mcl_gui.clear_popup(28);

  mcl_gui.draw_trigs(MCLGUI::s_menu_x + 4, MCLGUI::s_menu_y + 21, 0, 0, 0, 16);

  char K[4] = {'\0'};

  // draw step count
  uint8_t step_count =
      (MidiClock.div16th_counter - mcl_actions.start_clock32th / 2) -
      (64 *
       ((MidiClock.div16th_counter - mcl_actions.start_clock32th / 2) / 64));
  itoa(step_count, K, 10);
  mcl_gui.draw_text_encoder(MCLGUI::s_menu_x + 4, MCLGUI::s_menu_y + 4, "STEP",
                            K);

  // draw quantize
  strcpy(K, "---");
  if ((encoders[3]->getValue() < 7) && (encoders[3]->getValue() > 0)) {
    uint8_t x = 1 << encoders[3]->getValue();
    itoa(x, K, 10);
  }
  mcl_gui.draw_text_encoder(MCLGUI::s_menu_x + MCLGUI::s_menu_w - 26,
                            MCLGUI::s_menu_y + 4, "QUANT", K);

  oled_display.setFont(&TomThumb);
  // draw data flow in the center
  oled_display.setCursor(48, MCLGUI::s_menu_y + 12);
  oled_display.print("SND");
  oled_display.setCursor(46, MCLGUI::s_menu_y + 19);
  oled_display.print("GRID");

  mcl_gui.draw_horizontal_arrow(63, MCLGUI::s_menu_y + 8, 5);
  mcl_gui.draw_horizontal_arrow(63, MCLGUI::s_menu_y + 15, 5);

  oled_display.setCursor(74, MCLGUI::s_menu_y + 12);
  oled_display.print("MD");
  oled_display.setCursor(74, MCLGUI::s_menu_y + 19);
  oled_display.print("SEQ");

  oled_display.display();
}

#endif

bool GridWritePage::handleEvent(gui_event_t *event) {
  // Call parent GUI handler first.
  if (GridIOPage::handleEvent(event)) {
    return true;
  }
  DEBUG_DUMP(event->source);
  if (note_interface.is_event(event)) {
    DEBUG_PRINTLN("note event");
    if (note_interface.notes_all_off()) {
      DEBUG_PRINTLN("notes all off");
      if (BUTTON_DOWN(Buttons.BUTTON2)) {
        return true;
      } else {
#ifdef OLED_DISPLAY
        oled_display.textbox("CHAIN SLOTS", "");
        oled_display.display();
#endif
        trig_interface.off();
        mcl_actions.write_tracks(0, grid_page.encoders[1]->getValue());
      }
      GUI.setPage(&grid_page);
      curpage = 0;
    }

    return true;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON3)) {
    //  write the whole row

    trig_interface.off();
    for (uint8_t i = 0; i < 20; i++) {

      note_interface.notes[i] = 3;
    }
    //   write_tracks_to_md(-1);
#ifdef OLED_DISPLAY
    if (MidiClock.state != 2) {
    oled_display.textbox("CHAIN PAT", " + FX");
    oled_display.display();
    }
    else {
    oled_display.textbox("CHAIN PAT", "");
    oled_display.display();
    }
#endif
    mcl_actions.write_original = 1;
    mcl_actions.write_tracks(0, grid_page.encoders[1]->getValue());
    GUI.setPage(&grid_page);
    curpage = 0;
    return true;
  }
}
