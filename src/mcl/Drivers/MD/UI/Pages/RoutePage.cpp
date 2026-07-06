#include "RoutePage.h"
#include "ResourceManager.h"
#include "MCLGUI.h"
#include "../../MD.h"
#include "Devices/DeviceManager.h"
#include "../../../MidiDevice.h"
#include "MidiClock.h"
#include "Grid/MCLActions.h"
#include "MCLPlatformFeatures.h"
#include "platform.h"
#if MCL_FEATURE_HOST_ARRANGER_RECORD_HOOKS
#include "Arrangement/MCLArrangement.h"
#include "Host/SpsHostArrBridge.h"
#include "MCLMemory.h"
#endif

#if MCL_FEATURE_HOST_ARRANGER_RECORD_HOOKS
namespace {

void notify_arranger_route_recorded() {
  sps_host_arr_bridge.notifyDirty(0xFF, (uint8_t)spsarr::DIRTY_ARRANGEMENT);
}

void record_arranger_route(uint8_t track, uint8_t route) {
  if (!mcl_arrangement.automationRecordArmed() || track >= NUM_MD_TRACKS) {
    return;
  }
  uint8_t value = route > 6 ? 6 : route;
  if (mcl_arrangement.recordAutomationPoint(
          (uint8_t)(NUM_MD_TRACKS + MDROUTE_TRACK_NUM),
          mclarrfile::AUTOMATION_TARGET_ROUTING, 0, track,
          mclarrfile::AUTOMATION_VALUE_U7, value,
          mclarrfile::AUTOMATION_INTERP_HOLD, 0)) {
    notify_arranger_route_recorded();
  }
}

}  // namespace
#endif

void RoutePage::init() {
  R.Clear();
  R.use_icons_page();
  key_interface.on();
}
void RoutePage::set_level(int curtrack, int value) {
  // in_sysex = 1;
  MD.setTrackParam(curtrack, 33, value);
  // in_sysex = 0;
}

void RoutePage::draw_routes() {
  const uint64_t slide_mask = 0, mute_mask = 0;

  mcl_gui.draw_trigs(MCLGUI::seq_x0, MCLGUI::trig_y, 0, 0, -1, 16, mute_mask,
                     slide_mask);

  oled_display.setFont(&TomThumb);

  /*Display 16 track routes on screen,
   For 16 tracks check to see if there is a route*/
  for (uint8_t i = 0; i < 16; i++) {
    if (mcl_cfg.routing[i] == 6)
      continue;

    const auto x = MCLGUI::seq_x0 + i * (MCLGUI::seq_w + 1);
    const bool note_on = note_interface.is_note_on(i);

    oled_display.fillRect(x, MCLGUI::trig_y, MCLGUI::seq_w, MCLGUI::trig_h,
                          note_on ? WHITE : BLACK);
    oled_display.setTextColor(note_on ? BLACK : WHITE);
    oled_display.setCursor(x + 1, MCLGUI::trig_y + 5);
    oled_display.print((char)('A' + mcl_cfg.routing[i]));
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
#if MCL_FEATURE_HOST_ARRANGER_RECORD_HOOKS
  record_arranger_route((uint8_t)i, mcl_cfg.routing[i]);
#endif
}

void RoutePage::toggle_routes_batch(bool solo) {
  uint16_t quantize_mute;
  quantize_mute = encoders[1]->getValue();
  uint8_t i;
  if ((quantize_mute <= 64) && (quantize_mute > 1)) {
    while (((((MidiClock.div32th_counter - mcl_actions.start_clock32th) + 3) %
             (quantize_mute * 2)) != 0) &&
           (MidiClock.state == 2)) {
      platform_poll();
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
#if MCL_FEATURE_HOST_ARRANGER_RECORD_HOOKS
        record_arranger_route(i, mcl_cfg.routing[i]);
#endif
      }
    }
  }
}

void RoutePage::display() {
  oled_display.clearDisplay();
  oled_display.drawBitmap(0, 0, R.icons_page->icon_route, 24, 14, WHITE);

  mcl_gui.draw_knob_frame();

  char str_tmp[2];
  str_tmp[0] = encoders[0]->cur + 'A';
  str_tmp[1] = '\0';
  mcl_gui.draw_knob(0, mclstr_route, str_tmp);

  char Q[4];
  uint8_t quantize = encoders[1]->getValue();
  if (quantize == 1) {
    strcpy_P(Q, mclstr_dash);
  } else {
    mcl_gui.put_value_at(quantize, Q);
  }
  mcl_gui.draw_knob(1, mclstr_quant, Q);

  uint8_t step_count = (MidiClock.div16th_counter - mcl_actions.start_clock32th / 2) % 64;

  info_line2[0] = 'S';
  info_line2[1] = 'T';
  info_line2[2] = 'E';
  info_line2[3] = 'P';
  mcl_gui.put_value_at(step_count, info_line2 + 4);
  mcl_gui.draw_panel_labels("ROUTE", info_line2);

  draw_routes();
}

bool RoutePage::handleEvent(gui_event_t *event) {
  if (EVENT_NOTE(event)) {
    if (!device_manager.port_supports(
            event->port, MidiDeviceCapability::MdTrigInterface)) {
      return true;
    }
    key_interface.send_md_leds(TRIGLED_OVERLAY);

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
