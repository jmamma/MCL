#include "MCL_impl.h"
#include "ResourceManager.h"

void RoutePage::setup() {}
void RoutePage::init() {
  hasChanged = false;
  R.Clear();
  R.use_icons_page();
  trig_interface.on();
}
void RoutePage::cleanup() { }
void RoutePage::set_level(int curtrack, int value) {
  // in_sysex = 1;
  MD.setTrackParam(curtrack, 33, value);
  // in_sysex = 0;
}

void RoutePage::draw_routes() {
  const uint64_t slide_mask = 0, mute_mask = 0;

  mcl_gui.draw_trigs(MCLGUI::seq_x0, MCLGUI::trig_y, 0, 0, -1, 16, mute_mask,
                     slide_mask);

  char cur;

  oled_display.setFont(&TomThumb);

  /*Display 16 track routes on screen,
   For 16 tracks check to see if there is a route*/
  for (uint8_t i = 0; i < 16; i++) {

    if (mcl_cfg.routing[i] != 6) {
      cur = (char)'A' + mcl_cfg.routing[i];
      auto x = MCLGUI::seq_x0 + i * (MCLGUI::seq_w + 1);
      oled_display.setCursor(x + 1, MCLGUI::trig_y + 5);

      if (note_interface.is_note_on(i)) {
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
  quantize_mute = encoders[1]->getValue();
  uint8_t i;
  hasChanged = true;
  if ((quantize_mute <= 64) && (quantize_mute > 1)) {
    while (((((MidiClock.div32th_counter - mcl_actions.start_clock32th) + 3) %
             (quantize_mute * 2)) != 0) &&
           (MidiClock.state == 2)) {
      GUI.display();
    }
  }

  for (i = 0; i < 16; i++) {
    if (!solo) {
      if (note_interface.is_note_off(i)) {
        toggle_route(i, encoders[0]->cur);
      }
    } else {
      uint8_t routing_last = mcl_cfg.routing[i];
      if (note_interface.is_note_off(i)) {
        mcl_cfg.routing[i] = 6;
      } else {
        if (mcl_cfg.routing[i] == 6) {
          mcl_cfg.routing[i] = encoders[0]->cur;
        }
      }
      if (mcl_cfg.routing[i] != routing_last) {
        MD.setTrackRouting(i, mcl_cfg.routing[i]);
      }
    }
  }
}

void RoutePage::display() {
  uint8_t x;

  oled_display.clearDisplay();
  oled_display.drawBitmap(0, 0, R.icons_page->icon_route, 24, 14, WHITE);

  mcl_gui.draw_knob_frame();

  char str_tmp[2] = "0";
  str_tmp[0] = encoders[0]->cur + 'A';
  mcl_gui.draw_knob(0, "ROUTE", str_tmp);

  char Q[4] = {'\0'};
  if (encoders[1]->getValue() == 1) {
    strcpy(Q, "--");
  } else {
    x = encoders[1]->getValue();
    mcl_gui.put_value_at(x, Q);
  }
  mcl_gui.draw_knob(1, "QUANT", Q);

  uint8_t step_count =
      (MidiClock.div16th_counter - mcl_actions.start_clock32th / 2) -
      (64 *
       ((MidiClock.div16th_counter - mcl_actions.start_clock32th / 2) / 64));

  mcl_gui.put_value_at(step_count, Q);
  strcpy(info_line2, "STEP ");
  strcat(info_line2, Q);
  mcl_gui.draw_panel_labels("ROUTE", info_line2);

  draw_routes();
}

bool RoutePage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {
    uint8_t track = event->source - 128;
    if (midi_active_peering.get_device(event->port)->id != DEVICE_MD) {
      return true;
    }
    trig_interface.send_md_leds(TRIGLED_OVERLAY);

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
      }
    }
    return true;
  }
  // if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
  // update_globals();
  // md_exploit.off();
  // md_exploit.on();
  // mcl.setPage(MIXER_PAGE);
  // return true;
  //}
  /*
    if (EVENT_PRESSED(event, Buttons.ENCODER1) ||
        EVENT_PRESSED(event, Buttons.ENCODER2) ||
        EVENT_PRESSED(event, Buttons.ENCODER3) ||
        EVENT_PRESSED(event, Buttons.ENCODER1)) {
      update_globals();
      mcl.setPage(GRID_PAGE);

      return true;
    }
  */
  return false;
}
