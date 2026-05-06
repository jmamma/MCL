#include "GridIOPage.h"
#include "GridPages.h"
#include "GridTrack.h"
#include "MCLActions.h"
#include "MCLGUI.h"
#include "../Drivers/MD/MD.h"
#include "Project.h"
#include "ResourceManager.h"
#include "MCLStrings.h"

uint32_t GridIOPage::track_select = 0;
bool GridIOPage::show_track_type = false;
bool GridIOPage::show_offset = false;
uint8_t GridIOPage::offset = 0;

uint8_t GridIOPage::old_grid = 0;

void GridIOPage::cleanup() {
  key_interface.send_md_leds();
  MD.popup_text(127, 2);
  offset = 255;
}

void GridIOPage::init() {
  show_track_type = false;
  track_select = 0;
  show_offset = 0;
  offset = 255;
  old_grid = grid_page.cur_grid;
  R.Clear();
  R.use_icons_logo();
}

void GridIOPage::show_group_select_ui(const char *title_P) {
  show_track_type = true;
  MD.popup_text_P(title_P, true);
  mcl_gui.set_trigleds(mcl_cfg.track_type_select, TRIGLED_EXCLUSIVE);
  char str[16];
  mclstr_copy_progmem(str, title_P, sizeof(str));
  draw_title(str);
}

void GridIOPage::draw_title(const char *title, uint8_t y_offset) {
#ifdef PLATFORM_TBD
  mcl_gui.draw_popup_title_plain(title, y_offset);
#else
  mcl_gui.draw_popup_title(title, y_offset);
#endif
}

void GridIOPage::draw_tbd_panel_header(const char *title, uint8_t y_offset) {
#ifdef PLATFORM_TBD
  oled_display.setTextColor(WHITE, BLACK);
  oled_display.setFont();
  oled_display.setCursor(2, y_offset + 2);
  oled_display.print(title);

  char val[3];
  const uint8_t row = grid_page.getRow();
  const uint8_t bank = row / 16;
  mcl_gui.put_value_at2(row - bank * 16 + 1, val);

  oled_display.setCursor(92, y_offset + 2);
  oled_display.print((char)('X' + old_grid));
  oled_display.print(':');
  oled_display.print((char)('A' + bank));
  oled_display.print(val);
#else
  (void)title;
  (void)y_offset;
#endif
}

uint8_t GridIOPage::content_y_offset(uint8_t y_offset) {
  return y_offset;
}

void GridIOPage::clear_body(uint8_t y_offset) {
#ifdef PLATFORM_TBD
  if (y_offset >= 32) {
    oled_display.fillRect(0, y_offset, 128, 32, BLACK);
    return;
  }
#endif
  mcl_gui.clear_popup(0, y_offset);
}

void GridIOPage::paint_track_select_leds() {
#ifdef PLATFORM_TBD
  if (show_track_type) {
    mcl_gui.set_trigleds(mcl_cfg.track_type_select, TRIGLED_EXCLUSIVE);
    return;
  }

  uint16_t occupied_mask = 0;
  if (grid_page.cur_row < MAX_VISIBLE_ROWS &&
      grid_page.row_headers[grid_page.cur_row].active) {
    for (uint8_t n = 0; n < GRID_WIDTH && n < 16; n++) {
      uint8_t type = grid_page.row_headers[grid_page.cur_row].track_type[n];
      if (type != EMPTY_TRACK_TYPE && type != NULL_TRACK_TYPE && type != 255) {
        SET_BIT16(occupied_mask, n);
      }
    }
  }

  uint16_t active_mask = 0;
  if (show_offset) {
    if (offset < GRID_WIDTH && offset < 16) {
      SET_BIT16(active_mask, offset);
    }
  } else {
    for (uint8_t n = 0; n < GRID_WIDTH && n < 16; n++) {
      if (IS_BIT_SET32(track_select, n + old_grid * GRID_WIDTH) ||
          note_interface.is_note(n)) {
        SET_BIT16(active_mask, n);
      }
    }
  }

  constexpr uint32_t kSlotRed = ((uint32_t)255 << 16);
  constexpr uint32_t kSlotWhite = ((uint32_t)255 << 16) |
                                  ((uint32_t)255 << 8) |
                                  (uint32_t)255;
  mcl_gui.set_trigleds_local(0, TRIGLED_EXCLUSIVE);
  mcl_gui.set_trigleds_color(occupied_mask, kSlotRed);
  mcl_gui.set_trigleds_color(active_mask, kSlotWhite);
#else
  key_interface.send_md_leds(TRIGLED_OVERLAY);
#endif
}

void GridIOPage::populate_track_select_from_notes(uint8_t *track_select_array) {
  for (uint8_t n = 0; n < GRID_WIDTH; n++) {
    if (note_interface.is_note(n)) {
      SET_BIT32(track_select, n + old_grid * 16);
    }
  }
  for (uint8_t n = 0; n < NUM_SLOTS; n++) {
    if (IS_BIT_SET32(track_select, n)) {
      track_select_array[n] = 1;
    }
  }
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
  if (EVENT_NOTE(event)) {
    uint8_t track = event->source;
    if (event->mask == EVENT_BUTTON_PRESSED) {
      if (show_track_type) {
        if (track < 5) {
          TOGGLE_BIT16(mcl_cfg.track_type_select, track);
          mcl_gui.set_trigleds(mcl_cfg.track_type_select, TRIGLED_EXCLUSIVE);
        }
      } else {
        if (show_offset) {
          offset = track;
        }
        paint_track_select_leds();
      }
    } else {
      if (!show_track_type) {
        paint_track_select_leds();

        if (note_interface.notes_all_off()) {
          if (show_offset) {
            show_offset = !show_offset;
            note_interface.init_notes();
            if (show_offset) {
              offset = 255;
            }
            paint_track_select_leds();
          } else if (BUTTON_DOWN(Buttons.BUTTON2)) {
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
    uint8_t key = event->source;
    if (event->mask == EVENT_BUTTON_PRESSED) {
      switch (key) {
      default: {
        return false;
      }
#ifdef PLATFORM_TBD
      // TBD: A (MDX_KEY_NO) drives the group save / load gesture —
      // press opens the popup, release commits (handled per page).
      // The legacy AVR mapping (YES press → popup, NO press → close)
      // is preserved on AVR.
      case MDX_KEY_NO: {
        group_select();
        return true;
      }
      case MDX_KEY_YES: {
        return true; // X plays no group role on TBD
      }
#else
      case MDX_KEY_YES: {
        group_select();
        return true;
      }
      case MDX_KEY_NO: {
        goto close;
      }
#endif
      case MDX_KEY_BANKD: {
        return true;
      }
      case MDX_KEY_SCALE: {
        goto toggle_grid;
      }
      }
    }
  }
  if (EVENT_BUTTON(event)) {
    if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
    toggle_grid:
      for (uint8_t n = 0; n < GRID_WIDTH; n++) {
        if (note_interface.is_note(n)) {
          TOGGLE_BIT32(track_select, n + old_grid * 16);
          if (note_interface.is_note_on(n)) {
            note_interface.ignoreNextEvent(n);
          }
          note_interface.clear_note(n);
        }
      }
      old_grid = !old_grid;
      paint_track_select_leds();
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
  }
  return false;
}
