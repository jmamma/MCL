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
GridSlot GridIOPage::offset = 0;

GridIndex GridIOPage::old_grid = 0;

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

void GridIOPage::draw_popup_P(const char *title_P) {
  char str[16];
  mclstr_copy_progmem(str, title_P, sizeof(str));
  mcl_gui.draw_popup(str, true);
#ifdef PLATFORM_TBD
  draw_title(str);
#endif
}

void GridIOPage::draw_grid_marker(uint8_t body_y_offset) {
  oled_display.setFont(&Elektrothic);
  oled_display.setCursor(MCLGUI::s_menu_x + 4, 21 + body_y_offset);
  oled_display.print((char)(0x3A + old_grid));
  oled_display.setFont(&TomThumb);
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
  oled_display.setFont();

  uint8_t title_len = 0;
  while (title[title_len] != '\0') title_len++;

  constexpr uint8_t title_x = 0;
  constexpr uint8_t title_h = 12;
  const uint8_t title_y = y_offset + (32 - title_h) / 2;
  const uint8_t title_w = title_len * 6 + 4;

  oled_display.fillRect(title_x, title_y, title_w, title_h, WHITE);
  oled_display.setTextColor(BLACK, WHITE);
  oled_display.setCursor(title_x + 2, title_y + 2);
  oled_display.print(title);

  oled_display.setTextColor(WHITE, BLACK);
  char val[3];
  const GridRow row = grid_page.getRow();
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

GridSlot GridIOPage::slot_for_note(uint8_t note) {
  if (note < GRID_WIDTH) {
    return note + old_grid * GRID_WIDTH;
  }
  return note;
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
    if (offset < NUM_SLOTS && offset / GRID_WIDTH == old_grid) {
      SET_BIT16(active_mask, offset % GRID_WIDTH);
    }
  } else {
    for (uint8_t n = 0; n < GRID_WIDTH && n < 16; n++) {
      GridSlot slot = n + old_grid * GRID_WIDTH;
      if (IS_BIT_SET32(track_select, slot) || note_interface.is_note(slot) ||
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
  memset(track_select_array, 0, NUM_SLOTS);
  for (uint8_t n = 0; n < NUM_SLOTS; n++) {
    if (note_interface.is_note(n)) {
      SET_BIT32(track_select, slot_for_note(n));
    }
  }
  for (uint8_t n = 0; n < NUM_SLOTS; n++) {
    if (IS_BIT_SET32(track_select, n)) {
      track_select_array[n] = 1;
    }
  }
}

bool GridIOPage::slot_matches_track_type_select(GridSlot slot) {
  if (slot >= NUM_SLOTS) return false;

  GridDeviceTrack *gdt = mcl_actions.get_grid_dev_track(slot);
  if (gdt == nullptr) return false;

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

  return match != 255 && IS_BIT_SET16(mcl_cfg.track_type_select, match);
}

void GridIOPage::track_select_array_from_type_select(
    uint8_t *track_select_array) {
  memset(track_select_array, 0, NUM_SLOTS);
  for (uint8_t n = 0; n < NUM_SLOTS; n++) {
    if (slot_matches_track_type_select(n)) {
      track_select_array[n] = 1;
    }
  }
}

bool GridIOPage::handleEvent(gui_event_t *event) {
  if (EVENT_NOTE(event)) {
    uint8_t track = event->source;
    if (track >= NUM_SLOTS) return true;
    GridSlot slot = slot_for_note(track);
    if (event->mask == EVENT_BUTTON_PRESSED) {
      if (show_track_type) {
        if (track < 5) {
          TOGGLE_BIT16(mcl_cfg.track_type_select, track);
          mcl_gui.set_trigleds(mcl_cfg.track_type_select, TRIGLED_EXCLUSIVE);
        }
      } else {
        if (show_offset) {
          offset = slot;
        }
        paint_track_select_leds();
      }
    } else {
      if (!show_track_type) {
        paint_track_select_leds();

        if (note_interface.notes_all_off()) {
          if (show_offset) {
            show_offset = false;
            note_interface.init_notes();
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
      case MDX_KEY_YES: {
        group_select();
        return true;
      }
      case MDX_KEY_NO: {
        goto close;
      }
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
        GridSlot slot = n + old_grid * GRID_WIDTH;
        if (note_interface.is_note(n) || note_interface.is_note(slot)) {
          TOGGLE_BIT32(track_select, slot);
          if (note_interface.is_note_on(n)) {
            note_interface.ignoreNextEvent(n);
          }
          if (slot != n && note_interface.is_note_on(slot)) {
            note_interface.ignoreNextEvent(slot);
          }
          note_interface.clear_note(n);
          if (slot != n) {
            note_interface.clear_note(slot);
          }
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
