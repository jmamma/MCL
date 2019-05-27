#include "MCL.h"
#include "RoutePage.h"

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

void RoutePage::toggle_route(int i, uint8_t routing) {
  if (mcl_cfg.routing[i] != 6) {
    mcl_cfg.routing[i] = 6;
  } else {
    mcl_cfg.routing[i] = routing;
  }
  MD.setTrackRouting(i, mcl_cfg.routing[i]);
}
void RoutePage::toggle_routes_batch() {
  uint16_t quantize_mute;
  quantize_mute = 1 << encoders[2]->getValue();
  int i;
  for (i = 0; i < 16; i++) {
    if (note_interface.notes[i] == 3) {
      MD.muteTrack(i, true);
    }
  }
  if ((encoders[2]->getValue() < 7) && (encoders[2]->getValue() > 0)) {
    while (((((MidiClock.div32th_counter - mcl_actions.start_clock32th) + 3) %
             (quantize_mute * 2)) != 0) &&
           (MidiClock.state == 2)) {
      GUI.loop();
    }
  }

  // send the track to master before unmuting

  for (i = 0; i < 16; i++) {
    if (note_interface.notes[i] == 3) {
      if (encoders[2]->getValue() == 7) {
        set_level(i, 0);
      }
      toggle_route(i, encoders[0]->cur);

      MD.muteTrack(i, false);
    }
    //  note_interface.notes[i] = 0;
    // trackinfo_page.display();
  }
  hasChanged = true;
}
void RoutePage::display() {
  GUI.clearLines();
  GUI.setLine(GUI.LINE2);
  uint8_t x;

  // GUI.put_string_at(12,"Route");
  GUI.put_string_at(0, "R");

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
bool RoutePage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {
    uint8_t track = event->source - 128;
    if (midi_active_peering.get_device(event->port) != DEVICE_MD) {
      return true;
    }
    draw_routes(0);
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
  if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
    GUI.setPage(&mixer_page);
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
    GUI.setPage(&page_select_page);
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.ENCODER1) ||
      EVENT_PRESSED(event, Buttons.ENCODER2) ||
      EVENT_PRESSED(event, Buttons.ENCODER3) ||
      EVENT_PRESSED(event, Buttons.ENCODER1)) {
    if (hasChanged) {
      ElektronDataToSysexEncoder encoder2(&MidiUart);
      md_exploit.setup_global(1);
      while ((MidiClock.state == 2) &&
             ((MidiClock.mod12_counter > 6) || (MidiClock.mod12_counter == 0)))
        ;
      MD.global.toSysex(encoder2);
    }
    GUI.setPage(&grid_page);

    return true;
  }
  return false;
}
