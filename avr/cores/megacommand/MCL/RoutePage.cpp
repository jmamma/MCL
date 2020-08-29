#include "MCL_impl.h"

void RoutePage::setup() {}
void RoutePage::init() {
  hasChanged = false;
  note_interface.state = true;
}
void RoutePage::cleanup() { note_interface.state = false; }
void RoutePage::set_level(int curtrack, int value) {
  // in_sysex = 1;
  MD.setTrackParam(curtrack, 33, value);
  // in_sysex = 0;
}

#ifndef OLED_DISPLAY
void RoutePage::draw_routes(uint8_t line_number) {
  if (line_number == 0) {
    GUI.setLine(GUI.LINE1);
  } else {
    GUI.setLine(GUI.LINE2);
  }
  /*Initialise the string with blank steps*/
  char str[17] = "----------------";

  /*Display 16 track routes on screen,
   For 16 tracks check to see if there is a route*/
  for (int i = 0; i < 16; i++) {

    if (mcl_cfg.routing[i] != 6) {
      str[i] = (char)'A' + mcl_cfg.routing[i];
    }
    if (note_interface.notes[i] > 0 && note_interface.notes[i] != 3) {
      /*If the bit is set, there is a route at this position. We'd like to
       * display it as [] on screen*/
      /*Char 219 on the minicommand LCD is a []*/

#ifdef OLED_DISPLAY
      str[i] = (char)2;
#else
      str[i] = (char)219;
#endif
    }
  }

  /*Display the routes*/
  GUI.put_string_at(0, str);
}
#else
void RoutePage::draw_routes() {
  mcl_gui.draw_trigs(MCLGUI::seq_x0, MCLGUI::trig_y, 0, 0, -1, 16);

  char cur;

  oled_display.setFont(&TomThumb);

  /*Display 16 track routes on screen,
   For 16 tracks check to see if there is a route*/
  for (int i = 0; i < 16; i++) {

    if (mcl_cfg.routing[i] != 6) {
      cur = (char)'A' + mcl_cfg.routing[i];
      auto x = MCLGUI::seq_x0 + i * (MCLGUI::seq_w + 1);
      oled_display.setCursor(x + 1, MCLGUI::trig_y + 5);

      if (note_interface.notes[i] > 0 && note_interface.notes[i] != 3) {
        oled_display.fillRect(x, MCLGUI::trig_y, MCLGUI::seq_w, MCLGUI::trig_h,
                              WHITE);
        oled_display.setTextColor(BLACK);
      } else {
        oled_display.fillRect(x, MCLGUI::trig_y, MCLGUI::seq_w, MCLGUI::trig_h,
                              BLACK);
        oled_display.setTextColor(WHITE);
      }

      oled_display.print(cur);
    }
  }

  oled_display.setTextColor(WHITE);
}
#endif

void RoutePage::toggle_route(int i, uint8_t routing) {
  if (mcl_cfg.routing[i] != 6) {
    mcl_cfg.routing[i] = 6;
  } else {
    mcl_cfg.routing[i] = routing;
  }
  MD.setTrackRouting(i, mcl_cfg.routing[i]);
}

void RoutePage::toggle_routes_batch(bool solo) {
  uint16_t quantize_mute;
  quantize_mute = 1 << encoders[1]->getValue();
  uint8_t i;
  hasChanged = true;
  if ((encoders[1]->getValue() < 7) && (encoders[1]->getValue() > 0)) {
    while (((((MidiClock.div32th_counter - mcl_actions.start_clock32th) + 3) %
             (quantize_mute * 2)) != 0) &&
           (MidiClock.state == 2)) {
      GUI.display();
    }
  }
  /*
  for (i = 0; i < 16; i++) {
    if (note_interface.notes[i] == 3) {
      MD.muteTrack(i, true);
    }
  }
*/
  // send the track to master before unmuting

  for (i = 0; i < 16; i++) {
    if (!solo) {
      if ((note_interface.notes[i] == 3)) {
        toggle_route(i, encoders[0]->cur);
      }
    } else {
      uint8_t routing_last = mcl_cfg.routing[i];
      if (note_interface.notes[i] == 3) {
        mcl_cfg.routing[i] = 6;
      } else {
        if (mcl_cfg.routing[i] == 6) {
          mcl_cfg.routing[i] = encoders[0]->cur;
        }
      }
      if (mcl_cfg.routing[i] != routing_last) {
        MD.setTrackRouting(i, mcl_cfg.routing[i]);
      }
    } ///
    //  note_interface.notes[i] = 0;
    // trackinfo_page.display();
  }
}

void RoutePage::update_globals() {
  if (hasChanged) {
    ElektronDataToSysexEncoder encoder2(&MidiUart);
    md_exploit.setup_global(1);
    while ((MidiClock.state == 2) &&
           ((MidiClock.mod12_counter > 6) || (MidiClock.mod12_counter == 0)))
      ;
    USE_LOCK();
    SET_LOCK();
    MD.global.toSysex(encoder2);
    CLEAR_LOCK();
    hasChanged = false;
  }
}

#ifndef OLED_DISPLAY
void RoutePage::display() {
  GUI.clearLines();
  GUI.setLine(GUI.LINE2);
  uint8_t x;

  // GUI.put_string_at(12,"Route");
  GUI.put_string_at(0, "ROUTE");

  GUI.put_string_at(9, "Q:");
  if (encoders[1]->getValue() == 0) {
    GUI.put_string_at(11, "--");
  } else {
    x = 1 << encoders[1]->getValue();
    GUI.put_value_at2(11, x);
  }
  char str_tmp[2] = "0";
  str_tmp[0] = encoders[0]->cur + 'A';
  GUI.put_string_at(7, str_tmp);
  uint8_t step_count =
      (MidiClock.div16th_counter - mcl_actions.start_clock32th / 2) -
      (64 *
       ((MidiClock.div16th_counter - mcl_actions.start_clock32th / 2) / 64));
  GUI.put_value_at2(14, step_count);
  draw_routes(0);
}
#else
void RoutePage::display() {
  uint8_t x;

  auto *oldfont = oled_display.getFont();
  oled_display.clearDisplay();
  oled_display.drawBitmap(0, 0, icon_route, 24, 16, WHITE);

  mcl_gui.draw_knob_frame();

  char str_tmp[2] = "0";
  str_tmp[0] = encoders[0]->cur + 'A';
  mcl_gui.draw_knob(0, "ROUTE", str_tmp);

  char Q[4] = {'\0'};
  if (encoders[1]->getValue() == 0) {
    strcpy(Q, "--");
  } else {
    x = 1 << encoders[1]->getValue();
    itoa(x, Q, 10);
  }
  mcl_gui.draw_knob(1, "QUANT", Q);

  uint8_t step_count =
      (MidiClock.div16th_counter - mcl_actions.start_clock32th / 2) -
      (64 *
       ((MidiClock.div16th_counter - mcl_actions.start_clock32th / 2) / 64));

  itoa(step_count, Q, 10);
  strcpy(info_line2, "STEP ");
  strcat(info_line2, Q);
  mcl_gui.draw_panel_labels("ROUTE", info_line2);

  draw_routes();
  oled_display.display();
  oled_display.setFont(oldfont);
}
#endif

bool RoutePage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {
    uint8_t track = event->source - 128;
    if (midi_active_peering.get_device(event->port) != DEVICE_MD) {
      return true;
    }
#ifndef OLED_DISPLAY
    draw_routes(0);
#endif
    /*    if (event->mask == EVENT_BUTTON_PRESSED) {

      if ((encoders[2]->getValue() == 0)) {
        toggle_route(track);
        md_exploit.send_globals();
      }
    } */
    if (event->mask == EVENT_BUTTON_RELEASED) {

      if (note_interface.notes_all_off()) {
        toggle_routes_batch();
        note_interface.init_notes();
        //  md_exploit.send_globals();
        curpage = 0;
      }
    }
    return true;
  }
  //if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
    //update_globals();
    //md_exploit.off();
    //md_exploit.on();
    //GUI.setPage(&mixer_page);
    //return true;
  //}
  if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
    GUI.setPage(&page_select_page);
    return true;
  }
/*
  if (EVENT_PRESSED(event, Buttons.ENCODER1) ||
      EVENT_PRESSED(event, Buttons.ENCODER2) ||
      EVENT_PRESSED(event, Buttons.ENCODER3) ||
      EVENT_PRESSED(event, Buttons.ENCODER1)) {
    update_globals();
    GUI.setPage(&grid_page);

    return true;
  }
*/
  return false;
}
