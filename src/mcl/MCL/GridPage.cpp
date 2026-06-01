#include "GridPage.h"
#include "CommonPages.h"
#ifdef PLATFORM_TBD
#include "Ui.h"
#endif
#include "GridPages.h"
#include "ResourceManager.h"
#include "MCLGUI.h"
#include "MCLSysConfig.h"
#include "GridTask.h"
#include "EmptyTrack.h"
#include "Project.h"
#include "DeviceManager.h"
#include "../Drivers/MidiDevice.h"
#include "../Drivers/Generic/GenericMidiDevice.h"
#include "../Drivers/MD/MD.h"
#include "../Drivers/MNM/MNMParams.h"
#ifdef PLATFORM_TBD
#include "GridIOOverlay.h"
#endif
#include "MCLActions.h"
#include "MCLClipBoard.h"
#include "MCLStrings.h"

#define PERF_ENC 1

namespace {

void set_slot_label(char label[3], char a, char b) {
  label[0] = a;
  label[1] = b;
  label[2] = '\0';
}

#if defined(__AVR__)
uint16_t grid_slot_label_for_type(uint8_t track_type,
                                  GridSlotLabelContext ctx) {
  const char *machine_name = nullptr;
  switch (track_type) {
  case MD_TRACK_TYPE:
    machine_name = getMDMachineNameShort(ctx.model, 2);
    return machine_name ? make_grid_slot_label(machine_name[0], machine_name[1])
                        : 0;
  case MNM_TRACK_TYPE:
    machine_name = getMNMMachineNameShort(ctx.model, 2);
    return machine_name ? make_grid_slot_label(machine_name[0], machine_name[1])
                        : 0;
  case A4_TRACK_TYPE:
    return make_grid_slot_label('A', ctx.column + '1');
  case EXT_TRACK_TYPE:
    return make_grid_slot_label('M', ctx.column + '1');
  case MDFX_TRACK_TYPE:
    return make_grid_slot_label('F', 'X');
  case MDTEMPO_TRACK_TYPE:
    return make_grid_slot_label('T', 'P');
  case MD_ROUTE_TRACK_TYPE:
    return make_grid_slot_label('R', 'T');
  case GRIDCHAIN_TRACK_TYPE:
    return make_grid_slot_label('C', 'N');
  case PERF_TRACK_TYPE:
    return make_grid_slot_label('P', 'F');
  default:
    return 0;
  }
}
#endif

void copy_grid_slot_label(uint8_t track_type,
                          GridSlotLabelContext ctx, char label[3]) {
#if defined(__AVR__)
  uint16_t packed = grid_slot_label_for_type(track_type, ctx);
#else
  EmptyTrack scratch;
  auto *track = ((DeviceTrack *)&scratch)->init_track_type(track_type);
  uint16_t packed = track->grid_slot_label(ctx);
#endif
  if (packed == 0) {
    return;
  }
  label[0] = packed >> 8;
  label[1] = packed;
  label[2] = '\0';
}

void inherit_grid_x_row_name(GridRowHeader &row_header, GridRow row) {
  GridRowHeader grid_x_header;
  if (!proj.read_grid_row_header(&grid_x_header, row, 0) ||
      !grid_x_header.active || grid_x_header.name[0] == '\0') {
    row_header.name[0] = '\0';
    return;
  }

  strncpy(row_header.name, grid_x_header.name, sizeof(row_header.name));
  row_header.name[sizeof(row_header.name) - 1] = '\0';
}

#if defined(MCL_HAS_DESKTOP_MOUSE)
bool grid_mouse_hit_visible_cell(GridPage &page, const mcl_mouse_event_t *event,
                                 uint8_t *visible_col, uint8_t *visible_row,
                                 GridColumn *track_idx, GridRow *row_idx) {
  constexpr int16_t x_offset = 43;
  constexpr int16_t y_offset = 8;
  constexpr int16_t row_top = y_offset - 6;

  int row = (event->y - row_top) / 8;
  if (event->y < row_top || row < 0 || row >= MAX_VISIBLE_ROWS) {
    return false;
  }

  int base_col = (int)page.getCol() - page.cur_col;
  int base_row = (int)page.getRow() - page.cur_row;
  GridSpan grid_width = page.getWidth();
  GridSpan col_shift = 0;
  if (page.show_slot_menu) {
    if (page.cur_col + param3.cur > MAX_VISIBLE_COLS - 1) {
      col_shift = page.cur_col + param3.cur - MAX_VISIBLE_COLS;
    }
  }

  int cur_posx = x_offset;
  int hit_x = -1;
  for (uint8_t x = col_shift; x < MAX_VISIBLE_COLS + col_shift && x < grid_width;
       x++) {
    int track = x + base_col;
    if (event->x >= cur_posx - 1 && event->x < cur_posx + 9) {
      hit_x = x;
      break;
    }
    if (track >= 0 && track % 4 == 3 && grid_width >= 8) {
      cur_posx += 12;
    } else {
      cur_posx += 10;
    }
  }

  if (hit_x < 0) {
    return false;
  }

  int track = hit_x + base_col;
  int grid_row = row + base_row;
  if (track < 0 || track >= GRID_WIDTH || grid_row < 0 ||
      grid_row >= GRID_LENGTH) {
    return false;
  }

  *visible_col = (uint8_t)hit_x;
  *visible_row = (uint8_t)row;
  *track_idx = (GridColumn)track;
  *row_idx = (GridRow)grid_row;
  return true;
}

void set_slot_menu_range_to_cell(GridPage &page, uint8_t visible_col,
                                 uint8_t visible_row) {
  GridSpan row_shift = 0;
  if (page.cur_row + param4.cur > MAX_VISIBLE_ROWS - 1) {
    row_shift = page.cur_row + param4.cur - MAX_VISIBLE_ROWS;
  }

  int width = (int)visible_col - page.cur_col + 1;
  int height = (int)visible_row + row_shift - page.cur_row + 1;
  if (width < 1) {
    width = 1;
  }
  if (height < 1) {
    height = 1;
  }
  if (width > page.getWidth()) {
    width = page.getWidth();
  }
  int max_height = GRID_LENGTH - page.getRow();
  if (height > max_height) {
    height = max_height;
  }

  param3.cur = width;
  param4.cur = height;
}

void close_slot_menu_from_mouse(GridPage &page) {
  if (!page.show_slot_menu) {
    return;
  }

  uint8_t old_undo = page.slot_undo;
  bool restore_undo = false;
  if (!(page.slot_copy || page.slot_paste || page.slot_clear ||
        page.slot_load)) {
    page.slot_undo = 0;
    restore_undo = true;
  }

  page.apply_slot_changes(false, true);

  if (restore_undo) {
    page.slot_undo = old_undo;
  }

  page.init();
}
#endif

} // namespace

void GridPage::init() {
  DEBUG_PRINTLN("Grid page init");
  if (mcl_cfg.grid_page_mode == PERF_ENC) {
    encoders[0] = &perf_param1;
    encoders[1] = &perf_param2;
    encoders[2] = &perf_param3;
    encoders[3] = &perf_param4;
  }
  else {
    encoders[0] = &param1;
    encoders[1] = &param2;
    encoders[2] = &param3;
    encoders[3] = &param4;
  }
  param1.max = getWidth() - 1;
  show_slot_menu = false;
  old_col = 255;
  reload_slot_models = false;
  draw_encoders = false;
  // Edge case, prevent R.Clear being called if we're outside of GridPage
  mcl_gui.reset_trigleds();
  if (mcl.currentPage() != GRID_PAGE) {
    return;
  }
  key_interface.off();
  //load_slot_models();
  R.Clear();
  R.use_machine_names_short();
  R.use_icons_knob();
#ifdef PLATFORM_TBD
  R.use_icons_logo();
#endif
}

void GridPage::setup() {
  param1.cur = param1.old = mcl_cfg.col;
  param2.cur = param2.old = mcl_cfg.row;
  param3.cur = 1;
  cur_col = mcl_cfg.cur_col;
  cur_row = mcl_cfg.cur_row;
  memset(active_slots, SLOT_DISABLED, sizeof(active_slots));
  /*
  cur_col = 0;
  if (mcl_cfg.row < MAX_VISIBLE_ROWS) { cur_row = mcl_cfg.row; }
  else { cur_row = MAX_VISIBLE_ROWS - 1; }
  */
}

void GridPage::cleanup() {
  oled_display.setFont();
  oled_display.setTextColor(WHITE, BLACK);
  bank_popup = 0;
}

void GridPage::load_row(uint8_t n, GridRow row) {
  if (row >= GRID_LENGTH) { return; }
  if (IS_BIT_CLEAR16(grid_page.bank_popup_loadmask, n)) {
    grid_load_page.group_load(row);
    SET_BIT16(grid_page.bank_popup_loadmask, n);
  }
}

void GridPage::jump_to_row(GridRow row) {
  //  uint8_t y = (row / MAX_VISIBLE_ROWS) * MAX_VISIBLE_ROWS;
  //  uint8_t r = row - y;
  GridRow y = row;
  GridRow r = row - (row / MAX_VISIBLE_ROWS) * MAX_VISIBLE_ROWS;
  param2.cur = y;
  param2.old = y;
  cur_row = r;
  reload_slot_models = false;
  grid_lastclock = read_clock_ms();
  write_cfg = true;
}

void GridPage::set_active_row(GridRow row) {
  grid_task.last_active_row = row;
  if (bank_popup) {
    send_row_led();
  }
}

void GridPage::send_row_led() {
  GridRow active_row = grid_task.last_active_row;
  if (active_row >= GRID_LENGTH) { return; }
  uint16_t blink_mask = 0;
  if ((active_row >> 4) == bank) {
    blink_mask = (uint16_t)1 << (active_row & 0x0F);
  }
  MD.set_trigleds(blink_mask, TRIGLED_EXCLUSIVENDYNAMIC, 1);
}
void GridPage::close_bank_popup() {
  if (bank_popup == 2) {
    MD.draw_close_bank();
  }
  key_interface.off();
  if (last_page != 255) {
    DEBUG_PRINTLN("setting page");
    mcl.setPage(last_page);
  }
  last_page = NULL_PAGE;
  bank_popup = 0;
  bank_popup_loadmask = 0;
#ifdef PLATFORM_TBD
  bank_popup_first_trig = 255;
  bank_popup_oled_visible = true;
#endif
  note_interface.init_notes();
  // Clear blink leds. On TBD this also drops the trig colour override
  // (set_trigleds_local resets it) so the strip returns to monochrome.
  MD.set_trigleds(0, TRIGLED_EXCLUSIVENDYNAMIC, 1);
#ifdef PLATFORM_TBD
  mcl_gui.set_trigleds_local(0, TRIGLED_EXCLUSIVE);
#endif
}

void GridPage::load_old_col() {
  cur_col = old_col;
  param1.cur = cur_col;
  param1.old = cur_col;
  param3.cur = GRID_WIDTH - cur_col;
  param3.old = param3.cur;
  cur_grid = 0;
  param3.max = getWidth() + 1;
}

void GridPage::loop() {
  if (mcl_cfg.grid_page_mode == PERF_ENC) {
     //need to limit range of the alternate encoders
     encoder_t *enc = nullptr;
     perf_page.func_enc_check();
     param1.update(enc);
     param2.update(enc);
     param3.update(enc);
     param4.update(enc);
  }
  int8_t diff, new_val;
  if (show_slot_menu) {
    if (param3.hasChanged()) {
        if ((cur_grid == 0) && (getCol() + param3.cur > (int)GRID_WIDTH)) {
        old_col = getCol();
        cur_col = 0;
        param1.cur = 0;
        param1.old = 0;
        param3.cur = 1;
        param3.old = 1;
        cur_grid = 1;
        param3.max = getWidth();
        load_slot_models();
        reload_slot_models = true;
      }
      else if ((cur_grid == 1) && (param3.cur == 0) && (old_col != 255)) {
        load_old_col();
        load_slot_models();
        reload_slot_models = true;
      }
      if (param3.cur == 0) { param3.cur = 1; param3.old = 1; }
    }
    if (param4.hasChanged()) {
      if (cur_row + param4.cur > MAX_VISIBLE_ROWS - 1) {
        load_slot_models();
        reload_slot_models = true;
      }
    }
    grid_slot_page.loop();
    return;
  } else {
    if (mcl_cfg.grid_page_mode == PERF_ENC) {
      if (encoders[0]->hasChanged() || encoders[1]->hasChanged() || encoders[2]->hasChanged() || encoders[3]->hasChanged()) {
         //mcl.setPage(MIXER_PAGE);
         draw_encoders_lastclock = read_clock_ms();
         draw_encoders = true;
         //return;
         perf_page.encoder_send();
      }
    }
  }

  if (param3.hasChanged()) {
    if (param3.cur == 0) { param3.cur = 1; param3.old = 1; }
  }
  if (param1.hasChanged()) {
    diff = param1.cur - param1.old;
    new_val = cur_col + diff;
    if (new_val > MAX_VISIBLE_COLS - 1) {
      new_val = MAX_VISIBLE_COLS - 1;
    }
    if (new_val < 0) {
      new_val = 0;
    }
    cur_col = new_val;
  }

  if (param2.hasChanged()) {
    diff = param2.cur - param2.old;
    new_val = cur_row + diff;

    if (new_val > MAX_VISIBLE_ROWS - 1) {
      new_val = MAX_VISIBLE_ROWS - 1;
    }
    if (new_val < 0) {
      new_val = 0;
    }
    cur_row = new_val;
    if ((cur_row == MAX_VISIBLE_ROWS - 1) || (cur_row == 0)) {
      load_slot_models();
    }
    reload_slot_models = true;
    grid_lastclock = read_clock_ms();

    write_cfg = true;
  }
  param3.cur = 1;
  param4.cur = 1;
  param4.max = GRID_LENGTH - getRow();

  if (!reload_slot_models) {
    load_slot_models();
    reload_slot_models = true;
  }

  uint16_t now = read_clock_ms();
  if (now < grid_lastclock) {
    grid_lastclock = now + GUI_NAME_TIMEOUT;
  }

  if (clock_diff(grid_lastclock, now) > GUI_NAME_TIMEOUT) {
    ///   DEBUG_DUMP(grid_lastclock);
    //   DEBUG_DUMP(read_clock_ms());
    if ((write_cfg) && (MidiClock.state != 2)) {
      mcl_cfg.cur_col = cur_col;
      mcl_cfg.cur_row = cur_row;

      mcl_cfg.col = param1.cur;
      mcl_cfg.row = param2.cur;

      mcl_cfg.tempo = MidiClock.get_tempo();
      DEBUG_PRINTLN(F("write cfg"));
      if (mcl_cfg.write_cfg()) {
        proj.store_config_from_system();
      }
      grid_lastclock = now;
      write_cfg = false;
      // }
    }
    row_state_scan();
  }

  if (mcl_cfg.grid_page_mode == PERF_ENC) {
     param1.checkHandle();
     param2.checkHandle();
     param3.checkHandle();
     param4.checkHandle();
  }

#ifndef PLATFORM_TBD
  // AVR: state 2 is the post-release countdown — popup auto-closes after
  // 800 ms. On TBD the popup is opened by a NO-button gesture instead of
  // a held BANK key, so there's no natural release to time off; the popup
  // stays up until trig→pattern or a re-press of NO closes it.
  if (bank_popup == 2 &&
      clock_diff(bank_popup_lastclock, now) > 800) {
    close_bank_popup();
    return;
  }
#endif

}

void GridPage::row_state_scan() {
  if (row_scan) {
    GridRowHeader header_tmp;
    row_scan--;

    GridRow row = GRID_LENGTH - row_scan - 1;
    proj.read_grid_row_header(&header_tmp, row, 0);
    bool state = header_tmp.is_empty();

    proj.read_grid_row_header(&header_tmp, row, 1);
    state |= header_tmp.is_empty();

    update_row_state(row, !state);
  }
}

void GridPage::update_row_state(GridRow row, bool state) {
  // DEBUG_PRINTLN("updating row state");
  // DEBUG_PRINTLN(row);
  if (state) {
    SET_BIT128_P(row_states, row);
  } else {
    CLEAR_BIT128_P(row_states, row);
  }
}

GridRow GridPage::getRow() { return param2.cur; }

GridColumn GridPage::getCol() { return param1.cur; }

GridSpan GridPage::getWidth() { return GRID_WIDTH; }

void GridPage::load_slot_models() {
  DEBUG_PRINTLN("load slot models");
  GridSpan row_shift = 0;
  if ((cur_row + param4.cur > MAX_VISIBLE_ROWS - 1)) {
    row_shift = cur_row + param4.cur - MAX_VISIBLE_ROWS;
  }
  GridRow base_row = getRow() - cur_row + row_shift;

  for (uint8_t n = 0; n < MAX_VISIBLE_ROWS; n++) {
    for (uint8_t x = 0; x < GRID_WIDTH; x++) {
      set_slot_label(slot_labels[n][x], '-', '-');
    }
    GridRow row = base_row + n;
    if (row >= GRID_LENGTH) { continue; }
    proj.read_grid_row_header(&row_headers[n], row, cur_grid);
    if (cur_grid != 0) {
      inherit_grid_x_row_name(row_headers[n], row);
    }
    update_row_state(row, row_headers[n].active);
    for (uint8_t x = 0; x < GRID_WIDTH; x++) {
      uint8_t track_type = row_headers[n].track_type[x];
      GridSlotLabelContext ctx = {row_headers[n].model[x], x};
#if defined(PLATFORM_TBD)
      ctx.slot = (GridSlot)(x + cur_grid * GRID_WIDTH);
      ctx.row = row;
#endif
      copy_grid_slot_label(track_type, ctx, slot_labels[n][x]);
    }
  }
}

void GridPage::display_counters() {
  uint8_t y_offset = 8;

  char val[3];

  mcl_gui.put_value_at2(MidiClock.bar_counter, val);

  if (val[0] == '0') {
    val[0] = (char)0x60;
  }

  oled_display.setFont(&TomThumb);
  oled_display.setCursor(24, y_offset);
  oled_display.print(val);

  mcl_print_P(mclstr_colon);
  oled_display.print(MidiClock.beat_counter);

  if ((mcl_actions.next_transition != (uint16_t)-1) &&
      (MidiClock.bar_counter <= mcl_actions.nearest_bar) &&
      (mcl_actions.nearest_beat > MidiClock.beat_counter ||
       mcl_actions.nearest_bar != MidiClock.bar_counter)) {
    mcl_gui.put_value_at2(mcl_actions.nearest_bar, val);

    if (val[0] == '0') {
      val[0] = (char)0x60;
      if (val[1] == '0') {
        val[1] = (char)0x60;
      }
    }
    oled_display.setCursor(24, y_offset + 8);
    oled_display.print(val);
    mcl_print_P(mclstr_colon);
    oled_display.print(mcl_actions.nearest_beat);
  }
}

static void draw_grid_device_label(uint8_t x, uint8_t y,
                                   MidiDevice *device) {
  const char *name = device->name;
  char label[3] = {name[0] ? name[0] : ' ', name[1] ? name[1] : ' ', '\0'};
  oled_display.setCursor(x, y);
  oled_display.print(label);
}

static void draw_bank_row_label(uint8_t row, char *val) NOINLINE();
static void draw_bank_row_label(uint8_t row, char *val) {
  oled_display.print((char)('A' + row / 16));
  mcl_gui.put_value_at2((row & 0x0F) + 1, val);
  oled_display.print(val);
}

void GridPage::display_grid_info() {
  uint8_t y_offset = 8;

  oled_display.setFont(&Elektrothic);
  oled_display.setCursor(0, 10);
  oled_display.print((uint16_t)(MidiClock.get_tempo() + 0.5f));

  display_counters();
  oled_display.setFont(&TomThumb);
  //  mcl_print_P(mclstr_colon);
  // oled_display.print(MidiClock.step_counter);

  oled_display.setCursor(22, y_offset + 1 * 8);

  uint8_t tri_x = 10, tri_y = 12;
  if (MidiClock.state == 2) {
    oled_display.drawFastVLine(tri_x, tri_y, 5, WHITE);
    oled_display.fillTriangle_3px(tri_x + 1, tri_y, WHITE);
  }
  if (MidiClock.state == 0) {
    oled_display.fillRect(tri_x - 1, tri_y, 2, 5, WHITE);
    oled_display.fillRect(tri_x + 2, tri_y, 2, 5, WHITE);
  }

  draw_grid_device_label(0, y_offset + 1 + 1 * 8,
                         device_manager.primary_device());
  draw_grid_device_label(0, y_offset + 3 * 8,
                         device_manager.secondary_device());

  oled_display.setCursor(10, y_offset + (MAX_VISIBLE_ROWS - 1) * 8);
  oled_display.print((char)('X' + cur_grid));
  oled_display.print(':');

  char val[4];
  mcl_gui.put_value_at2(param1.cur + 1, val);
  val[2] = '\0';
  oled_display.print(val);
  mcl_print_P(mclstr_space);
  draw_bank_row_label(param2.cur, val);

  oled_display.setCursor(1, y_offset + 2 * 8);
  oled_display.fillRect(oled_display.getCursorX() - 1,
                        oled_display.getCursorY() - 6, 37, 7, WHITE);

  oled_display.setTextColor(BLACK, WHITE);
  if (row_headers[cur_row].name[0] != '\0') {
    char rowname[10];
    strncpy(rowname, row_headers[cur_row].name, 9);
    rowname[9] = '\0';

    oled_display.print(rowname);
  }

  oled_display.setTextColor(WHITE, BLACK);
}
bool GridPage::is_slot_queue(uint8_t x, uint8_t y) {
  if (show_slot_menu) {
    return false;
  }
  GridSlot slot = cur_grid * GRID_WIDTH + x;
  if (mcl_actions.chains[slot].is_mode_queue()) {
    for (uint8_t n = 0; n < mcl_actions.chains[slot].num_of_links; n++) {
      if (mcl_actions.chains[slot].rows[n] == y) {
        return true;
      }
    }
  }
  return false;
}

void GridPage::display_grid() {
  uint8_t x_offset = 43;
  uint8_t y_offset = 8;

  oled_display.setFont(&TomThumb);

  GridColumn base_col = getCol() - cur_col;
  GridRow base_row = getRow() - cur_row;
  GridSpan grid_width = getWidth();
#ifdef PLATFORM_TBD
  GridRow selected_row = getRow();
#endif
  bool blink_hint = MidiClock.getBlinkHint(false);

//  encoders[1]->handler = NULL;

  GridSpan row_shift = 0;
  GridSpan col_shift = 0;
  if (show_slot_menu) {
    if (cur_col + param3.cur > MAX_VISIBLE_COLS - 1) {

      col_shift = cur_col + param3.cur - MAX_VISIBLE_COLS;
    }

    if (cur_row + param4.cur > MAX_VISIBLE_ROWS - 1) {
      row_shift = cur_row + param4.cur - MAX_VISIBLE_ROWS;
    }
  }
  for (uint8_t y = 0; y < MAX_VISIBLE_ROWS; y++) {

    uint8_t cur_posx = x_offset;
    uint8_t cur_posy = y_offset + y * 8;
    GridSpan w = grid_width;
    for (uint8_t x = col_shift; x < MAX_VISIBLE_COLS + col_shift && x < w;
         x++) {
      oled_display.setCursor(cur_posx, cur_posy);

      GridColumn track_idx = x + base_col;
      GridRow row_idx = y + base_row;

      uint8_t active_cue_color = WHITE;

      char *label = slot_labels[y][track_idx];
      //  Highlight the current cursor position + slot menu apply range
      bool a = in_area(x, y + row_shift, cur_col, cur_row, param3.cur - 1,
                       param4.cur - 1);
      bool b = is_slot_queue(track_idx, row_idx);
      bool io_selected = false;
#ifdef PLATFORM_TBD
      io_selected = (row_idx == selected_row) &&
                    grid_io_overlay.is_slot_selected(track_idx);
#endif

      if ((a ^ b) || io_selected) {
        oled_display.fillRect(cur_posx - 1, cur_posy - 6, 9, 7, WHITE);
        oled_display.setTextColor(BLACK, WHITE);
        active_cue_color = BLACK;
      } else {
        oled_display.setTextColor(WHITE, BLACK);
      }

      GridSlot track_grid_idx = track_idx + GRID_WIDTH * cur_grid;
      GridRow active_slot = active_slots[track_grid_idx];
      bool active = row_idx == active_slot;
      if (blink_hint && active) {
        // blink, don't print
      } else {
        oled_display.print(label);
        if (active) {
          // a gentle visual cue for active slots
          oled_display.drawPixel(cur_posx - 1, cur_posy - 6, active_cue_color);
        }
      }

      // tomThumb is 4x6
      if (track_idx % 4 == 3 && w >= 8) {
        if (y == 0) {
          // draw vertical separator
          mcl_gui.draw_vertical_dashline(cur_posx + 9, 3);
        }
        cur_posx += 12;
      } else {
        cur_posx += 10;
      }
    }
  }

  // optionally, draw the first separator
  if ((base_col + col_shift) % 4 == 0) {
    mcl_gui.draw_vertical_dashline(x_offset - 3, 3);
  }
  oled_display.setTextColor(WHITE, BLACK);
}
void GridPage::display_slot_menu() {
  uint8_t y_offset = 8;
  grid_slot_page.draw_menu(1, y_offset, 39);
  // grid_slot_page.draw_scrollbar(36);
}

void GridPage::display_row_info() {
  GridSpan row_shift = 0;
  if ((cur_row + param4.cur > MAX_VISIBLE_ROWS - 1)) {
    row_shift = cur_row + param4.cur - MAX_VISIBLE_ROWS;
  }
  GridRow base_row = getRow() - cur_row + row_shift;
  char val[4];
  val[2] = '\0';

  for (uint8_t n = 0; n < MAX_VISIBLE_ROWS; n++) {
    GridRow row = base_row + n;
    if (row >= GRID_LENGTH) { return; }
    oled_display.setCursor(27, (n + 1) * 8);
    draw_bank_row_label(row, val);
  }
}
#ifdef FPS
int frames;
int frameclock;
#endif
void GridPage::display() {

  #ifdef FPS
  if (clock_diff(frameclock, read_clock_ms()) >= 1000) {
  DEBUG_PRINT("FPS: "); DEBUG_PRINTLN(frames);
  frames = 0;
  frameclock = read_clock_ms();
  }
  frames++;
  #endif
  oled_display.clearDisplay();
  oled_display.setFont(&TomThumb);
  oled_display.setTextColor(WHITE, BLACK);
  if (!show_slot_menu) {
    display_grid_info();
  } else {
    if (param4.cur > 4) {
     display_row_info();
    }
    else{
     display_slot_menu();
    }
  }
  display_grid();
  if (draw_encoders && clock_diff(draw_encoders_lastclock, read_clock_ms()) < 750) {
    oled_display.setFont();
    mixer_page.draw_encs();
  }
  else {
    draw_encoders = false;
  }
#ifdef DEBUGMODE
  if (row_scan) {
    mcl_gui.draw_progress_bar(8, 8, false, 18, 2, 18, 7, false);
  }
#endif

#if defined(PLATFORM_TBD) && defined(DEBUG_TBD_BUTTONS)
  {
    extern Ui tbd_ui;
    ui_data_t dbg = tbd_ui.CopyUiData();
    char buf[32];
    oled_display.setFont();
    oled_display.setTextSize(1);
    oled_display.setTextColor(WHITE, BLACK);
    oled_display.setCursor(0, 32);
    snprintf(buf, sizeof(buf), "f:%02X d:%04X", dbg.f_btns, dbg.d_btns);
    oled_display.print(buf);
    oled_display.setCursor(0, 40);
    snprintf(buf, sizeof(buf), "m:%04X p:%02X%02X%02X%02X",
      dbg.mcl_btns,
      dbg.pot_states[0], dbg.pot_states[1],
      dbg.pot_states[2], dbg.pot_states[3]);
    oled_display.print(buf);
    oled_display.setCursor(0, 48);
    snprintf(buf, sizeof(buf), "fl:%02X ml:%04X", dbg.f_btns_long_press, dbg.mcl_btns_long_press);
    oled_display.print(buf);
  }
#endif

  // The SPS bottom-32 strip is now an overlay (SpsStripPage) installed
  // by SpsMode when the latch turns on; rendering happens in
  // GuiClass::display() via the GUI overlay slot, not here.
}

void rename_row() {
  const char *my_title = "ROW NAME:";
  GridRowHeader row_headers[NUM_GRIDS];

  for (uint8_t n = 0; n < NUM_GRIDS; n++) {
    proj.read_grid_row_header(&row_headers[n], grid_page.getRow(), n);
  }

  if (row_headers[0].active) {
    if (mcl_gui.wait_for_input(row_headers[0].name, my_title, 8)) {
      strcpy(row_headers[1].name, row_headers[0].name);
      for (uint8_t n = 0; n < NUM_GRIDS; n++) {
        proj.write_grid_row_header(&(row_headers[n]), grid_page.getRow(), n);
        proj.sync_grid(n);
      }
    }
  } else {
    gfx.alert("Error", "Row not active");
  }
  grid_page.load_slot_models();
  grid_slot_page.select_item(0);
}

void apply_slot_changes_cb() { grid_page.apply_slot_changes(); }

void GridPage::apply_slot_changes(bool ignore_undo, bool ignore_func) {
  GridSpan width;
  GridSpan height;

  GridColumn _col = getCol();

  bool activate_header = false;
  //old_col != 255 indicates that the grid selection spans grids x and y.
  if (old_col != 255) {
    _col = old_col;
    cur_grid = 0;
  }

  uint8_t track_select_array[NUM_SLOTS];
  uint8_t load_mode_old = mcl_cfg.load_mode;
  uint8_t undo = slot_undo && !ignore_undo && slot_undo_x == _col &&
                 slot_undo_y == getRow();
  DEBUG_PRINTLN("apply slot");


  if (!ignore_func) {
    void (*row_func)() =
        grid_slot_page.menu.get_row_function(grid_slot_page.encoders[1]->cur);
    if (row_func != NULL) {
      DEBUG_PRINTLN("calling menu func");
      (*row_func)();
      return;
    }
  }

  GridTrack temp_slot;
  if (!temp_slot.load_from_grid(_col + cur_grid * GRID_WIDTH, getRow())) { return; }
  slot.set_load_sound(slot_load_sound != 0);

  width = old_col != 255 ? GRID_WIDTH - _col : param3.cur;
  height = param4.cur;

  uint8_t slot_update = 0;
  bool slot_changed_length = temp_slot.link.length != slot.link.length;
  bool slot_changed_loops = temp_slot.link.loops != slot.link.loops;
  bool slot_changed_row = temp_slot.link.row != slot.link.row;
  bool slot_changed_load_sound =
      temp_slot.load_sound() != slot.load_sound();

  if (!(slot_copy || slot_paste || slot_clear || slot_load || undo)) {
    if ((slot_changed_length) ||
        (slot_changed_loops) ||
        (slot_changed_row) ||
        (slot_changed_load_sound)) {
      slot_update = 1;
      DEBUG_PRINTLN("Slot update");
    }
    height = 1;
  }
  if (undo == 1) {
    slot_paste = 1;
  }

  if (slot_copy == 1 || (slot_clear == 1 && !undo)) {
    const char *verb;
    if (slot_clear == 1) {
      slot_undo_x = _col;
      slot_undo_y = getRow();
      verb = mclstr_clear;
      slot_undo = 1;
    } else {
      slot_undo = 0;
      verb = mclstr_copy;
    }
    oled_display.textbox_P(verb, width > 0 ? mclstr_slots : mclstr_slot);
    uint8_t copy_w = (old_col != 255) ? width + param3.cur : width;
    mcl_clipboard.copy(_col + GRID_WIDTH * cur_grid, getRow(), copy_w, height);
  }
  if (slot_clear) {
    goto run;
  }

  else if (slot_paste == 1) {
    if (undo) {
      oled_display.textbox_P(mclstr_undo);
    } else {
      oled_display.textbox_P(mclstr_paste);
    }
    slot_undo = 0;
    mcl_clipboard.paste(_col + GRID_WIDTH * cur_grid, getRow());
  } else {
    if (slot_update == 1) {
      oled_display.textbox_P(mclstr_slot, mclstr_update);
    }

    if (slot_load) {
      if (height > 1) {
        mcl_cfg.load_mode = LOAD_QUEUE;
      }
      grid_load_page.display_load();
    }
  run:

    oled_display.display();

    GridRowHeader header;

  again:

    for (uint8_t y = 0; y < height && y + getRow() < GRID_LENGTH; y++) {
      GridRow ypos = y + getRow();
      proj.read_grid_row_header(&header, ypos, cur_grid);

      if (slot_load == 1) {
        memset(track_select_array, 0, sizeof(track_select_array));
      }

      if (slot_clear && height > 8) {
        mcl_gui.draw_progress("", y, height);
      }
      for (uint8_t x = 0; x < width && x + _col < getWidth(); x++) {
        GridColumn xpos = x + _col;
        if (slot_clear == 1) {
          // Delete slot(s)
          proj.clear_slot_grid(xpos + cur_grid * GRID_WIDTH, ypos);
          header.update_model(xpos, 0, EMPTY_TRACK_TYPE);
        } else if (slot_update == 1) {
          // Save slot link data
          activate_header = true;
          if (x == 0 && (old_col == 255 || cur_grid == 0)) {
            //slot.active = header.track_type[xpos];
            slot.store_in_grid(xpos + cur_grid * GRID_WIDTH, ypos);
          }
          else {
            if (!temp_slot.load_from_grid(xpos + cur_grid * GRID_WIDTH, ypos)) { continue; }
            uint16_t temp_slot_length =
                (uint16_t)temp_slot.link.length *
                SeqTrack::get_speed_multiplier_int(temp_slot.link.speed_value()) / 12;
            bool store_slot = false;
            if (slot_changed_load_sound) {
              temp_slot.set_load_sound(slot.load_sound());
              store_slot = true;
            }
            if (slot_changed_loops && slot.link.loops == 0) {
                temp_slot.link.loops = 0;
                store_slot = true;
            }
            else if (slot_changed_loops || slot_changed_length) {
              //User adjusted both loops and length on src, assume they want to update all selected slots to this value.
              if (slot_changed_loops && slot_changed_length) {
                temp_slot.link.loops = slot.link.loops;
                temp_slot.link.length = slot.link.length;
                store_slot = true;
              }
              //User changed loops, check if length of current is an even multiple of src length, if so increase loops to match
              else if (slot_changed_loops && temp_slot_length) {
                uint16_t target_length =
                    (uint32_t)slot.link.length *
                    SeqTrack::get_speed_multiplier_int(slot.link.speed_value()) *
                    slot.link.loops / 12;
                if (!(target_length % temp_slot_length) && temp_slot_length <= target_length) {
                  temp_slot.link.loops = target_length / temp_slot_length; //try and match the src track target length
                }
                else {
                  temp_slot.link.loops = slot.link.loops; //just change the loops
                }
                store_slot = true;
              }
              //User changed length, if speeds are the same we can increaase the length;
              else if (slot_changed_length &&
                       temp_slot.link.speed_value() == slot.link.speed_value()) {
                temp_slot.link.length = slot.link.length;
                store_slot = true;
              }
            }
            if (slot_changed_row) {
              store_slot = true;
              temp_slot.link.row = slot.link.row;
            }
            if (store_slot) { temp_slot.store_in_grid(xpos + cur_grid * GRID_WIDTH, ypos); }
          }
        } else if (slot_load == 1) {
          // if (height > 1 && y == 0) {
          //   mcl_actions.chains[xpos].init();
          // }
          track_select_array[xpos + cur_grid * GRID_WIDTH] = 1;
        }
      }
      if (slot_load == 1) {
        DEBUG_PRINTLN("slot load put");
        grid_task.load_queue.put(mcl_cfg.load_mode,ypos, track_select_array);
      }
      // If all slots are deleted then clear the row name
      else if ((header.is_empty() && (slot_clear == 1)) || (activate_header)) {
        header.active = activate_header;
        header.name[0] = '\0';
        proj.write_grid_row_header(&header, ypos, cur_grid);
      }
    }
  }
  if ((slot_clear == 1) || (slot_paste == 1) || (slot_update == 1)) {
    proj.sync_grid(cur_grid);
  }
  if (old_col != 255 && slot_paste) {
    proj.sync_grid(1);
  }

  if (old_col != 255) {
    if (cur_grid == 0) {
      cur_grid = 1;
      width = param3.cur;
      _col = 0;
      goto again;
    }
    load_old_col();
  }
  mcl_cfg.load_mode = load_mode_old;
  slot_apply = 0;
  slot_load = 0;
  slot_clear = 0;
  slot_copy = 0;
  slot_paste = 0;
  slot.load_from_grid(_col + cur_grid * GRID_WIDTH, getRow());
  slot_load_sound = slot.load_sound() ? 1 : 0;
  old_col = 255;
}

bool GridPage::handleEvent(gui_event_t *event) {
  if (EVENT_NOTE(event)) {
    uint8_t port = event->port;

    uint8_t track = event->source;
    if (!device_manager.port_supports(
            port, MidiDeviceCapability::MdTrigInterface)) {
      return true;
    }

    GridRow row = grid_page.bank * 16 + track;
    if (event->mask == EVENT_BUTTON_RELEASED) {
      if (grid_page.bank_popup > 0) {
        if (note_interface.notes_all_off()) {
          grid_page.bank_popup_loadmask = 0;
          grid_page.close_bank_popup();
        }
        return true;
      }
    }

    else if (event->mask == EVENT_BUTTON_PRESSED) {
      if (grid_page.bank_popup > 0) {

        uint8_t load_mode_old = mcl_cfg.load_mode;
        uint16_t loadmask = grid_page.bank_popup_loadmask;
        bool has_load = loadmask != 0;
        bool single_load = has_load && ((loadmask & (loadmask - 1)) == 0);

        if (!has_load) {
          grid_page.jump_to_row(row);
          if (load_mode_old != LOAD_AUTO) {
            mcl_cfg.load_mode = LOAD_MANUAL;
          }
          mcl_actions.init_chains();
        }
        else {
          mcl_cfg.load_mode = LOAD_QUEUE;
        }

        if (single_load) {
          uint8_t n = 0;
          while ((loadmask & 1) == 0) {
            loadmask >>= 1;
            n++;
          }
          uint8_t r = grid_page.bank * 16 + n;
          grid_page.bank_popup_loadmask = 0;
          // Reload as queue.
          grid_page.load_row(n, r);
        }

        grid_page.load_row(track, row);

#ifdef PLATFORM_TBD
        // Track first trig and repaint the strip: head=red, chained=yellow.
        // Keep the OLED bank grid visible while selecting patterns.
        if (grid_page.bank_popup_first_trig == 255) {
          grid_page.bank_popup_first_trig = track;
        }
        {
          uint16_t loadmask = grid_page.bank_popup_loadmask;
          uint16_t head_mask = (uint16_t)1 << grid_page.bank_popup_first_trig;
          uint16_t chained_mask = loadmask & ~head_mask;
          // Layer order matters — later calls overwrite earlier per-trig
          // colours. Populated rows first as a dim red baseline, then the
          // chained set in yellow, then the head in bright red.
          constexpr uint32_t kRedDim    = ((uint32_t)48  << 16);
          constexpr uint32_t kYellow    = ((uint32_t)255 << 16) | ((uint32_t)200 << 8);
          constexpr uint32_t kRedBright = ((uint32_t)255 << 16);
          mcl_gui.set_trigleds_color(grid_row_bank_mask(grid_page.row_states, grid_page.bank),
                                     kRedDim);
          mcl_gui.set_trigleds_color(chained_mask, kYellow);
          mcl_gui.set_trigleds_color(head_mask, kRedBright);
        }
#else
        // AVR: a held BANK key is the gesture modifier — pressing a trig
        // while no BANK key is held collapses straight to "load and close".
        // TBD keeps the popup up across multiple trig presses; the
        // EVENT_BUTTON_RELEASED branch closes once all trigs are released.
        if (!key_interface.is_key_down(MDX_KEY_BANKA) &&
            !key_interface.is_key_down(MDX_KEY_BANKB) &&
            !key_interface.is_key_down(MDX_KEY_BANKC) &&
            !key_interface.is_key_down(MDX_KEY_BANKD)) {
          grid_page.close_bank_popup();
        }
#endif
        mcl_cfg.load_mode = load_mode_old;
        return true;
      }
    }
  }
  if (EVENT_CMD(event)) {

    uint8_t key = event->source;
    if (key_interface.is_key_down(MDX_KEY_PATSONG)) {
      if (show_slot_menu) {
        if (event->mask == EVENT_BUTTON_PRESSED) {
          switch (key) {
          case MDX_KEY_BANKA:
          case MDX_KEY_BANKB:
          case MDX_KEY_BANKC: {
          loadmode:
            mcl_cfg.load_mode = key - MDX_KEY_BANKA + 1;
            bool persistent = false;
            grid_load_page.md_popup_title(mcl_cfg.load_mode, persistent);
            return true;
          }
          case MDX_KEY_NO: {
            goto next;
          }
          case MDX_KEY_PATSONGKIT: {
            set_active_row(grid_task.last_active_row);
            return true;
          }
          }
        }
        return grid_slot_page.handleEvent(event);
      }
    }
  next:
    if (event->mask == EVENT_BUTTON_PRESSED) {
      if (key != MDX_KEY_FUNC) {
        draw_encoders = false;
      }
      uint8_t inc = 1;
      bool func_down = key_interface.is_key_down(MDX_KEY_FUNC);
      if (func_down) {
        inc = 4;
      }
      if (show_slot_menu) {
#ifdef PLATFORM_TBD
        const bool is_arrow =
            key == MDX_KEY_UP || key == MDX_KEY_DOWN ||
            key == MDX_KEY_LEFT || key == MDX_KEY_RIGHT;
        if (is_arrow) {
          // TBD default: when the slot menu is open, arrows navigate the
          // menu first. Hold the normal FUNC modifier to adjust the slot
          // geometry instead.
          if (!func_down) {
            switch (key) {
            case MDX_KEY_UP:
              grid_slot_page.encoders[1]->cur -= 1;
              return true;
            case MDX_KEY_DOWN:
              grid_slot_page.encoders[1]->cur += 1;
              return true;
            case MDX_KEY_LEFT:
              grid_slot_page.encoders[0]->cur -= 1;
              return true;
            case MDX_KEY_RIGHT:
              grid_slot_page.encoders[0]->cur += 1;
              return true;
            default:
              break;
            }
          }
          switch (key) {
          case MDX_KEY_UP:
            param4.cur -= inc;
            return true;
          case MDX_KEY_DOWN:
            param4.cur += inc;
            return true;
          case MDX_KEY_LEFT:
            if (inc > 1) {
              inc = 4;
            }
            param3.cur = max(0, param3.cur - inc);
            return true;
          case MDX_KEY_RIGHT:
            if (inc > 1) {
              inc = 4;
            }
            param3.cur += inc;
            return true;
          default:
            break;
          }
        }
#endif
        switch (key) {
        // case MDX_KEY_NO: {
        //  goto slot_menu_off;
        //}
        case MDX_KEY_YES: {
          slot_load = 1;
          goto slot_menu_off;
        }
        case MDX_KEY_COPY: {
          slot_copy = 1;
          goto apply_slot_edit;
        }
        case MDX_KEY_CLEAR: {
          slot_clear = 1;
          goto apply_slot_edit;
        }
        case MDX_KEY_PASTE: {
          slot_paste = 1;
        apply_slot_edit:
          apply_slot_changes(false, true);
          init();
          // if (key_interface.is_key_down(MDX_KEY_NO)) {
          //  goto slot_menu_on;
          //}
          return true;
        }
        case MDX_KEY_UP: {
          param4.cur -= inc;
          return true;
        }
        case MDX_KEY_DOWN: {
          param4.cur += inc;
          return true;
        }
        case MDX_KEY_LEFT: {
          param3.cur = max(0, param3.cur - inc);
          return true;
        }
        case MDX_KEY_RIGHT: {
          param3.cur += inc;
          return true;
        }
        case MDX_KEY_BANKA:
        case MDX_KEY_BANKB:
        case MDX_KEY_BANKC: {
          if (!func_down) {
            goto loadmode;
          }
        }
        }
      }
      switch (key) {
      case MDX_KEY_SCALE: {
        cur_grid = !cur_grid;
        param1.max = getWidth() - 1;
        init();
        return true;
      }
      case MDX_KEY_UP: {
        param2.cur -= inc;
        reset_undo();
        return true;
      }
      case MDX_KEY_DOWN: {
        param2.cur += inc;
        reset_undo();
        return true;
      }
      case MDX_KEY_LEFT: {
        param1.cur = max(0, param1.cur - inc);
        reset_undo();
        return true;
      }
      case MDX_KEY_RIGHT: {
        param1.cur += inc;
        reset_undo();
        return true;
      }
      case MDX_KEY_YES: {
        key_interface.ignoreNextEvent(MDX_KEY_YES);
        if (!func_down) {
          goto load;
        }
        goto save;
      }
      case MDX_KEY_NO: {
        if (!show_slot_menu) {
          goto slot_menu_on;
        }
        break;
      }
      }
    }
    if (event->mask == EVENT_BUTTON_RELEASED) {
      if (bank_popup) {
        switch (key) {
        case MDX_KEY_BANKA:
        case MDX_KEY_BANKB:
        case MDX_KEY_BANKC:
        case MDX_KEY_BANKD: {
          uint8_t bank = key - MDX_KEY_BANKA;
          if (bank_popup == 1) {
            bank_popup = 2;
            bank_popup_lastclock = read_clock_ms();
            MD.draw_bank(bank);
          }
          return true;
        }
        }
      }
      switch (key) {
      case MDX_KEY_NO: {
        if (!key_interface.is_key_down(MDX_KEY_PATSONG)) {
          goto slot_menu_off;
        }
        return true;
      }
      }
    }
  }

  if (EVENT_BUTTON(event)) {

    if (!show_slot_menu) {
      if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
      save:
#ifdef PLATFORM_TBD
        grid_io_overlay.begin(GridIOOverlay::MODE_SAVE);
#else
        mcl.setPage(GRID_SAVE_PAGE);
#endif

        return true;
      }

      if (EVENT_RELEASED(event, Buttons.BUTTON4)) {
      load:
#ifdef PLATFORM_TBD
        grid_io_overlay.begin(GridIOOverlay::MODE_LOAD);
#else
        mcl.setPage(GRID_LOAD_PAGE);
#endif

        return true;
      }
    }

    if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
      if (key_interface.is_key_down(MDX_KEY_NO)) { return true; }
    slot_menu_on:
      DEBUG_DUMP(getCol());
      DEBUG_DUMP(getRow());
      if (!slot.load_from_grid(getCol() + cur_grid * GRID_WIDTH, getRow())) { return true; }
      DEBUG_PRINTLN(F("what's in the slot"));
      DEBUG_DUMP(slot.link.loops);
      DEBUG_DUMP(slot.link.row);
      slot_load_sound = slot.load_sound() ? 1 : 0;
      encoders[0] = &grid_slot_param1;
      encoders[1] = &grid_slot_param2;
      encoders[2] = &param3;
      encoders[3] = &param4;
      param3.cur = 1;
      param4.cur = 1;
      slot_apply = 0;
      old_col = 255;
      if (!slot.is_ext_track()) {
        grid_slot_page.menu.enable_entry(1, true);
        grid_slot_page.menu.enable_entry(2, false);
      } else {
        grid_slot_page.menu.enable_entry(1, false);
        grid_slot_page.menu.enable_entry(2, true);
      }
      show_slot_menu = true;
      grid_slot_page.init();
      return true;
    }

    if (EVENT_RELEASED(event, Buttons.BUTTON3)) {
    slot_menu_off:
      if (show_slot_menu && !key_interface.is_key_down(MDX_KEY_NO)) {
        uint8_t old_undo = slot_undo;
        bool restore_undo = false;
        // Prevent undo from occuring when re-entering shift menu. Want to keep
        // undo flag in case user decides to undo with MD GUI.
        if (!(slot_copy || slot_paste || slot_clear || slot_load)) {
          slot_undo = 0;
          restore_undo = true;
        }

        apply_slot_changes();

        if (restore_undo) {
          slot_undo = old_undo;
        }

        DEBUG_PRINTLN("here");
        init();
      }
      return true;
    }
#ifndef PLATFORM_TBD
    // A + X → SYSTEM_PAGE on AVR. TBD reaches the system page via the
    // TL → TR chord in tbd_handleEvent so this slot is free for Y/X
    // chord assignments.
    if ((EVENT_PRESSED(event, Buttons.BUTTON1) &&
         BUTTON_DOWN(Buttons.BUTTON4)) ||
        (EVENT_PRESSED(event, Buttons.BUTTON4) &&
         BUTTON_DOWN(Buttons.BUTTON1))) {
      system_page.isSetup = false;
      mcl.pushPage(SYSTEM_PAGE);
      return true;
    }
#endif
  }
  return false;
}

#if defined(MCL_HAS_DESKTOP_MOUSE)
bool GridPage::handleMouseEvent(mcl_mouse_event_t *event) {
  if (event == NULL) {
    return false;
  }

  const bool left_button = (event->buttons & MCL_MOUSE_BUTTON_LEFT) != 0;
  const bool right_button = (event->buttons & MCL_MOUSE_BUTTON_RIGHT) != 0;

  if (show_slot_menu) {
    if (right_button && (event->type == MCL_MOUSE_DOWN ||
                         event->type == MCL_MOUSE_DOUBLE_CLICK)) {
      close_slot_menu_from_mouse(*this);
      return true;
    }

    if (event->x < 40 &&
        grid_slot_page.handleMouseEventAt(event, 1, 8, 39)) {
      return true;
    }

    if (event->type == MCL_MOUSE_WHEEL && event->deltaY != 0) {
      int step = event->deltaY > 0 ? -1 : 1;
      if ((event->modifiers & MCL_MOUSE_MODIFIER_SHIFT) != 0) {
        param3.cur += step;
      } else {
        param4.cur += step;
      }
      return true;
    }

    if (event->type != MCL_MOUSE_DOWN &&
        event->type != MCL_MOUSE_DOUBLE_CLICK &&
        event->type != MCL_MOUSE_DRAG) {
      return false;
    }
    if (!left_button) {
      return false;
    }

    uint8_t visible_col = 0;
    uint8_t visible_row = 0;
    GridColumn track_idx = 0;
    GridRow row_idx = 0;
    if (!grid_mouse_hit_visible_cell(*this, event, &visible_col, &visible_row,
                                     &track_idx, &row_idx)) {
      return false;
    }

    set_slot_menu_range_to_cell(*this, visible_col, visible_row);
    return true;
  }

  if (event->type == MCL_MOUSE_WHEEL && event->deltaY != 0) {
    param2.cur += event->deltaY > 0 ? -1 : 1;
    reset_undo();
    return true;
  }

  if (event->type != MCL_MOUSE_DOWN &&
      event->type != MCL_MOUSE_DOUBLE_CLICK) {
    return false;
  }
  if (!left_button && !right_button) {
    return false;
  }

  uint8_t visible_col = 0;
  uint8_t visible_row = 0;
  GridColumn track_idx = 0;
  GridRow row_idx = 0;
  if (!grid_mouse_hit_visible_cell(*this, event, &visible_col, &visible_row,
                                   &track_idx, &row_idx)) {
    return false;
  }

  param1.cur = track_idx;
  param2.cur = row_idx;
  reset_undo();

  if (right_button) {
    GUI.queueVirtualButton(Buttons.BUTTON3, true);
    return true;
  }

  if (event->type == MCL_MOUSE_DOUBLE_CLICK && !show_slot_menu) {
#ifdef PLATFORM_TBD
    grid_io_overlay.begin(GridIOOverlay::MODE_LOAD);
#else
    mcl.setPage(GRID_LOAD_PAGE);
#endif
  }
  return true;
}
#endif
