#include "MCL_impl.h"

uint32_t GridIOPage::track_select = 0;
bool GridIOPage::show_track_type = false;
uint8_t GridIOPage::old_grid = 0;

void GridIOPage::cleanup() { proj.select_grid(old_grid); }

void GridIOPage::init() {
  old_grid = proj.get_grid();
  show_track_type = false;
  track_select = 0;
}

void GridIOPage::track_select_array_from_type_select(uint8_t *track_select_array) {
  uint8_t grid_idx, track_idx, track_type, dev_idx;
  bool is_aux;

  for (uint8_t n = 0; n < NUM_SLOTS; n++) {
    SeqTrack *seq_track = mcl_actions.get_dev_slot_info(
        n, &grid_idx, &track_idx, &track_type, &dev_idx, &is_aux);
    if (track_type == 255)
      continue;
    if (!is_aux && IS_BIT_SET16(mcl_cfg.track_type_select, dev_idx)) {
      track_select_array[n] = 1;
    }
    // AUX tracks
    if (is_aux && IS_BIT_SET16(mcl_cfg.track_type_select, dev_idx + 1)) {
      track_select_array[n] = 1;
    }
  }
}

bool GridIOPage::handleEvent(gui_event_t *event) {
  if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
    for (uint8_t n = 0; n < GRID_WIDTH; n++) {
      if (note_interface.notes[n] > 0) {
        TOGGLE_BIT32(track_select, n + proj.get_grid() * 16);
        if (note_interface.notes[n] == 1) {
          note_interface.ignoreNextEvent(n);
        }
        note_interface.notes[n] = 0;
      }
    }
    proj.toggle_grid();
    trig_interface.send_md_leds();
  }

  if (EVENT_PRESSED(event, Buttons.ENCODER1) ||
      EVENT_PRESSED(event, Buttons.ENCODER2) ||
      EVENT_PRESSED(event, Buttons.ENCODER3) ||
      EVENT_PRESSED(event, Buttons.ENCODER4) ||
      EVENT_RELEASED(event, Buttons.BUTTON1) ||
      EVENT_RELEASED(event, Buttons.BUTTON4)) {
    GUI.setPage(&grid_page);
    curpage = 0;
    return true;
  }
  return false;
}
