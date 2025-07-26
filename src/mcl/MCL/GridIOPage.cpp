#include "MCL_impl.h"
#include "ResourceManager.h"

uint32_t GridIOPage::track_select = 0;
bool GridIOPage::show_track_type = false;
bool GridIOPage::show_offset = false;
uint8_t GridIOPage::offset = 0;

uint8_t GridIOPage::old_grid = 0;

void GridIOPage::cleanup() {
  trig_interface.send_md_leds();
  MD.popup_text(127, 2);
  proj.select_grid(old_grid);
  offset = 255;
}

void GridIOPage::init() {
  old_grid = proj.get_grid();
  show_track_type = false;
  track_select = 0;
  show_offset = 0;
  offset = 255;
  R.Clear();
  R.use_icons_logo();
}

void GridIOPage::track_select_array_from_type_select(
    uint8_t *track_select_array) {
  for (uint8_t n = 0; n < NUM_SLOTS; n++) {
    GridDeviceTrack *gdt = mcl_actions.get_grid_dev_track(n);

    uint8_t device_idx = gdt->device_idx;
    if (gdt == nullptr)
      continue;
    uint8_t match = 255;
    switch (gdt->group_type) {
    case GROUP_DEV:
      match = gdt->device_idx;
      break;
    case GROUP_AUX:
    case GROUP_PERF:
    case GROUP_TEMPO:
      match = gdt->group_type + 1;
      break;
    }
    if (match == 255) {
      continue;
    }
    if (IS_BIT_SET16(mcl_cfg.track_type_select, match)) {
      track_select_array[n] = 1;
    }
  }
}

bool GridIOPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {
    uint8_t track = event->source - 128;
    if (event->mask == EVENT_BUTTON_PRESSED) {
      if (show_track_type) {
        if (track < 5) {
          TOGGLE_BIT16(mcl_cfg.track_type_select, track);
          MD.set_trigleds(mcl_cfg.track_type_select, TRIGLED_EXCLUSIVE);
        }
      } else {
        if (show_offset) {
          offset = track;
        }
        trig_interface.send_md_leds(TRIGLED_OVERLAY);
      }
    } else {
      if (!show_track_type) {
        trig_interface.send_md_leds(TRIGLED_OVERLAY);

        if (note_interface.notes_all_off()) {
          if (show_offset) {
            show_offset = !show_offset;
            note_interface.init_notes();
            if (show_offset) {
              offset = 255;
            }
          }
          else if (BUTTON_DOWN(Buttons.BUTTON2)) {
            return true;
          } else {
            action();
          }
        }
      }
    }

    return true;
  }
  if (EVENT_CMD(event)) {
    uint8_t key = event->source - 64;
    if (event->mask == EVENT_BUTTON_PRESSED) {
      switch (key) {
      case MDX_KEY_BANKD: {
        return true;
      }
      case MDX_KEY_NO: {
        goto close;
      }
      case MDX_KEY_YES: {
        group_select();
        return true;
      }
      case MDX_KEY_SCALE: {
        goto toggle_grid;
      }
      }
    }
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
  toggle_grid:
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
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
    group_select();
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.ENCODER4)) {
    //   encoders[3]->pressmode = !encoders[3]->pressmode;
  }
  // if (EVENT_PRESSED(event, Buttons.ENCODER1) ||
  //    EVENT_PRESSED(event, Buttons.ENCODER2) ||
  //    EVENT_PRESSED(event, Buttons.ENCODER3) ||
  //    EVENT_PRESSED(event, Buttons.ENCODER4)) {
  // }
  if (EVENT_RELEASED(event, Buttons.BUTTON1) ||
      EVENT_RELEASED(event, Buttons.BUTTON4)) {
  close:
    mcl.setPage(GRID_PAGE);
    return true;
  }
  return false;
}
