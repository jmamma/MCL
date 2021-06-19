#include "MCL_impl.h"

uint32_t GridIOPage::track_select = 0;
bool GridIOPage::show_track_type = false;
uint8_t GridIOPage::old_grid = 0;

void GridIOPage::cleanup() {
  trig_interface.send_md_leds();
  MD.popup_text(127, 2);
  proj.select_grid(old_grid);
}

void GridIOPage::init() {
  old_grid = proj.get_grid();
  show_track_type = false;
  track_select = 0;
}

void GridIOPage::track_select_array_from_type_select(
    uint8_t *track_select_array) {
  uint8_t track_idx, dev_idx;

  for (uint8_t n = 0; n < NUM_SLOTS; n++) {
    GridDeviceTrack *gdt =
        mcl_actions.get_grid_dev_track(n, &track_idx, &dev_idx);

    if (gdt == nullptr)
      continue;

    if ((gdt->group_type == GROUP_DEV) &&
        IS_BIT_SET16(mcl_cfg.track_type_select, dev_idx)) {
      track_select_array[n] = 1;
    }
    // AUX tracks
    if ((gdt->group_type == GROUP_AUX) &&
        IS_BIT_SET16(mcl_cfg.track_type_select, 2)) {
      track_select_array[n] = 1;
    }

    if ((gdt->group_type == GROUP_TEMPO) &&
        IS_BIT_SET16(mcl_cfg.track_type_select, 3)) {
      track_select_array[n] = 1;
    }
  }
}

bool GridIOPage::handleEvent(gui_event_t *event) {
  if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
    for (uint8_t n = 0; n < GRID_WIDTH; n++) {
      if (note_interface.is_note(n)) {
        TOGGLE_BIT32(track_select, n + proj.get_grid() * 16);
        if (note_interface.is_note_on(n)) {
          note_interface.ignoreNextEvent(n);
        }
        note_interface.clear_note(n);
      }
    }
    proj.toggle_grid();
    trig_interface.send_md_leds();
  }
  if (EVENT_CMD(event)) {
    uint8_t key = event->source - 64;
    if (event->mask == EVENT_BUTTON_PRESSED) {
      switch (key) {
      case MDX_KEY_NO: {
        goto close;
      }
      case MDX_KEY_YES: {
        group_select();
        return true;
      }
     }
    }
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
    group_select();
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.ENCODER1) ||
      EVENT_PRESSED(event, Buttons.ENCODER2) ||
      EVENT_PRESSED(event, Buttons.ENCODER3) ||
      EVENT_PRESSED(event, Buttons.ENCODER4) ||
      EVENT_RELEASED(event, Buttons.BUTTON1) ||
      EVENT_RELEASED(event, Buttons.BUTTON4)) {
  close:
    GUI.setPage(&grid_page);
    return true;
  }
  return false;
}
