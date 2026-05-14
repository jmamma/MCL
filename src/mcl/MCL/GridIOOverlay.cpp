#include "GridIOOverlay.h"

#ifdef PLATFORM_TBD

#include "GridChain.h"
#include "GridIOPage.h"
#include "GridPages.h"
#include "GridTask.h"
#include "MCLActions.h"
#include "DeviceManager.h"
#include "MCLGUI.h"
#include "MCLStrings.h"
#include "MCLSysConfig.h"
#include "MDTrack.h"
#include "../Drivers/MD/MD.h"
#include <string.h>

GridIOOverlay grid_io_overlay;

namespace {

bool save_needs_md_current_pattern() {
  return MD.connected &&
         (device_manager.primary_device() == &MD ||
          device_manager.secondary_device() == &MD);
}

} // namespace

GridIOOverlay::GridIOOverlay()
    : LightPage(&unused_enc_, &mode_enc_, &queue_len_enc_, &quant_enc_),
      mode_enc_(LOAD_QUEUE, LOAD_MANUAL, ENCODER_RES_PAT),
      queue_len_enc_(64, 1, ENCODER_RES_PAT),
      unused_enc_(0, 0, ENCODER_RES_PAT),
      quant_enc_(64, 1, ENCODER_RES_PAT) {}

void GridIOOverlay::begin(Mode mode) {
  mode_ = mode;
  if (GUI.overlay == this) {
    init();
  } else {
    GUI.setOverlay(this);
  }
}

bool GridIOOverlay::is_active() const {
  return GUI.overlay == this;
}

void GridIOOverlay::init() {
  GridIOPage::show_track_type = false;
  GridIOPage::track_select = 0;
  GridIOPage::show_offset = false;
  GridIOPage::offset = 255;
  GridIOPage::old_grid = grid_page.cur_grid;
  saved_grid_ = grid_page.cur_grid;

  note_interface.init_notes();
  key_interface.send_md_leds(TRIGLED_OVERLAY);
  key_interface.on();

  mode_enc_.cur = mcl_cfg.load_mode;
  mode_enc_.old = mcl_cfg.load_mode;
  queue_len_enc_.cur = mcl_cfg.chain_queue_length;
  queue_len_enc_.old = mcl_cfg.chain_queue_length;
  quant_enc_.cur = mcl_cfg.chain_load_quant;
  quant_enc_.old = mcl_cfg.chain_load_quant;
  init_encoders_used_clock();

  if (mode_ == MODE_LOAD) {
    char title[16];
    load_mode_title(title, sizeof(title));
    MD.popup_text(title, true);
  } else {
    if (save_needs_md_current_pattern()) {
      MD.getCurrentPattern(CALLBACK_TIMEOUT);
    }
    MD.popup_text_P(mclstr_save_slots, true);
    grid_page.reload_slot_models = false;
  }
  sync_preview_grid();
  GridIOPage::paint_track_select_leds();
}

void GridIOOverlay::cleanup() {
  key_interface.send_md_leds();
  key_interface.off();
  MD.popup_text(127, 2);
  GridIOPage::offset = 255;
  note_interface.init_notes();
  if (grid_page.cur_grid != saved_grid_) {
    grid_page.cur_grid = saved_grid_;
    grid_page.reload_slot_models = false;
  }
}

void GridIOOverlay::loop() {
  if (mode_ == MODE_LOAD) {
    if (mode_enc_.hasChanged()) {
      mcl_cfg.load_mode = mode_enc_.cur;
      char title[16];
      load_mode_title(title, sizeof(title));
      MD.popup_text(title, true);
    }
    if (queue_len_enc_.hasChanged()) {
      if (mode_enc_.cur == LOAD_QUEUE) {
        mcl_cfg.chain_queue_length = queue_len_enc_.cur;
      } else {
        queue_len_enc_.cur = queue_len_enc_.old;
      }
    }
    if (quant_enc_.hasChanged()) {
      mcl_cfg.chain_load_quant = quant_enc_.cur;
    }
  }
}

void GridIOOverlay::display() {
  constexpr uint8_t y_offset = 32;
  oled_display.fillRect(0, y_offset, 128, 32, BLACK);

  if (mode_ == MODE_LOAD) {
    grid_load_page.display_at(y_offset);
  } else {
    grid_save_page.display_at(y_offset);
  }
}

bool GridIOOverlay::handleEvent(gui_event_t *event) {
  if (EVENT_NOTE(event)) {
    uint8_t track = event->source;
    if (track >= NUM_SLOTS) return true;
    GridSlot slot = GridIOPage::slot_for_note(track);

    if (event->mask == EVENT_BUTTON_PRESSED) {
      if (GridIOPage::show_track_type) {
        if (track < 5) {
          TOGGLE_BIT16(mcl_cfg.track_type_select, track);
          mcl_gui.set_trigleds(mcl_cfg.track_type_select, TRIGLED_EXCLUSIVE);
        }
      } else {
        focus_slot(slot);
        if (GridIOPage::show_offset) {
          GridIOPage::offset = slot;
        }
        GridIOPage::paint_track_select_leds();
      }
      return true;
    }

    if (event->mask == EVENT_BUTTON_RELEASED && !GridIOPage::show_track_type) {
      GridIOPage::paint_track_select_leds();
      if (note_interface.notes_all_off()) {
        if (GridIOPage::show_offset) {
          GridIOPage::show_offset = false;
          note_interface.init_notes();
          GridIOPage::paint_track_select_leds();
        } else if (BUTTON_DOWN(Buttons.BUTTON2)) {
          return true;
        } else {
          action();
        }
      }
      return true;
    }

    return true;
  }

  if (EVENT_CMD(event)) {
    uint8_t key = event->source;
    if (event->mask == EVENT_BUTTON_PRESSED) {
      switch (key) {
      case MDX_KEY_YES:
        group_select();
        return true;
      case MDX_KEY_NO:
        close();
        return true;
      case MDX_KEY_BANKD:
        return true;
      case MDX_KEY_SCALE:
        toggle_grid();
        return true;
      case MDX_KEY_FUNC:
        if (mode_ == MODE_LOAD && mcl_cfg.load_mode == LOAD_MANUAL) {
          GridIOPage::show_offset = !GridIOPage::show_offset;
          note_interface.init_notes();
          if (GridIOPage::show_offset) {
            GridIOPage::offset = 255;
          }
          GridIOPage::paint_track_select_leds();
        }
        return true;
      case MDX_KEY_BANKA:
      case MDX_KEY_BANKB:
      case MDX_KEY_BANKC:
        if (mode_ == MODE_LOAD && !key_interface.is_key_down(MDX_KEY_FUNC)) {
          mode_enc_.cur = key - MDX_KEY_BANKA + 1;
          return true;
        }
        if (mode_ == MODE_SAVE) {
          mode_enc_.cur = key - MDX_KEY_BANKA;
          return true;
        }
        return false;
      default:
        close();
        return true;
      }
    }

    if (event->mask == EVENT_BUTTON_RELEASED) {
      if (key == MDX_KEY_YES && GridIOPage::show_track_type) {
        group_action();
        return true;
      }
    }
    return false;
  }

  if (EVENT_BUTTON(event)) {
    if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
      toggle_grid();
      return true;
    }
    if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
      group_select();
      return true;
    }
    if (EVENT_RELEASED(event, Buttons.BUTTON3) && GridIOPage::show_track_type) {
      group_action();
      return true;
    }
    if (EVENT_RELEASED(event, Buttons.BUTTON1) ||
        EVENT_RELEASED(event, Buttons.BUTTON4)) {
      close();
      return true;
    }
  }

  return false;
}

void GridIOOverlay::close() {
  GUI.clearOverlay();
}

void GridIOOverlay::action() {
  uint8_t track_select_array[NUM_SLOTS] = {0};
  selected_tracks(track_select_array);
  if (mode_ == MODE_LOAD) {
    grid_task.load_queue.put(mcl_cfg.load_mode, grid_page.getRow(),
                             track_select_array, GridIOPage::offset);
  } else {
    mcl_actions.save_tracks(grid_page.getRow(), track_select_array, SAVE_SEQ);
  }
  close();
}

void GridIOOverlay::group_action() {
  key_interface.off();
  if (mode_ == MODE_LOAD) {
    uint8_t track_select_array[NUM_SLOTS] = {0};
    grid_load_page.track_select_array_from_type_select(track_select_array);
    mcl_actions.write_original = 1;
    grid_task.load_queue.put(mcl_cfg.load_mode, grid_page.getRow(),
                             track_select_array, GridIOPage::offset);
  } else {
    uint8_t track_select_array[NUM_SLOTS] = {0};
    grid_save_page.track_select_array_from_type_select(track_select_array);
    mcl_actions.save_tracks(grid_page.getRow(), track_select_array, SAVE_SEQ);
  }
  close();
}

void GridIOOverlay::group_select() {
  GridIOPage::show_track_type = true;
  MD.popup_text_P(mode_ == MODE_LOAD ? mclstr_load_groups : mclstr_save_groups,
                  true);
  mcl_gui.set_trigleds(mcl_cfg.track_type_select, TRIGLED_EXCLUSIVE);
}

void GridIOOverlay::toggle_grid() {
  for (uint8_t n = 0; n < GRID_WIDTH; n++) {
    GridSlot slot = n + GridIOPage::old_grid * GRID_WIDTH;
    if (note_interface.is_note(n) || note_interface.is_note(slot)) {
      TOGGLE_BIT32(GridIOPage::track_select, slot);
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
  GridIOPage::old_grid = !GridIOPage::old_grid;
  sync_preview_grid();
  GridIOPage::paint_track_select_leds();
}

void GridIOOverlay::focus_slot(GridSlot slot) {
  if (slot >= NUM_SLOTS) return;

  GridIndex grid = slot / GRID_WIDTH;
  if (grid >= NUM_GRIDS) return;

  if (GridIOPage::old_grid == grid && grid_page.cur_grid == grid) {
    return;
  }

  GridIOPage::old_grid = grid;
  sync_preview_grid();
}

void GridIOOverlay::selected_tracks(uint8_t *track_select_array) {
  for (uint8_t n = 0; n < NUM_SLOTS; n++) {
    if (note_interface.is_note(n)) {
      SET_BIT32(GridIOPage::track_select, GridIOPage::slot_for_note(n));
    }
  }
  for (uint8_t n = 0; n < NUM_SLOTS; n++) {
    if (IS_BIT_SET32(GridIOPage::track_select, n)) {
      track_select_array[n] = 1;
    }
  }
}

uint16_t GridIOOverlay::visible_select_mask() const {
  uint16_t mask = 0;
  if (GridIOPage::show_offset) {
    if (GridIOPage::offset < NUM_SLOTS &&
        GridIOPage::offset / GRID_WIDTH == GridIOPage::old_grid) {
      SET_BIT16(mask, GridIOPage::offset % GRID_WIDTH);
    }
    return mask;
  }
  for (uint8_t n = 0; n < GRID_WIDTH; n++) {
    GridSlot slot = n + GridIOPage::old_grid * GRID_WIDTH;
    if (IS_BIT_SET32(GridIOPage::track_select, slot) ||
        note_interface.is_note(slot) || note_interface.is_note(n)) {
      SET_BIT16(mask, n);
    }
  }
  return mask;
}

bool GridIOOverlay::is_slot_selected(uint8_t visible_slot) const {
  if (!is_active()) return false;
  if (visible_slot >= GRID_WIDTH) return false;
  if (GridIOPage::show_track_type) {
    if (grid_page.cur_grid != GridIOPage::old_grid) return false;
    return GridIOPage::slot_matches_track_type_select(
        visible_slot + GridIOPage::old_grid * GRID_WIDTH);
  }
  if (grid_page.cur_grid != GridIOPage::old_grid) return false;
  uint16_t mask = visible_select_mask();
  return IS_BIT_SET16(mask, visible_slot);
}

void GridIOOverlay::load_mode_title(char *title, uint8_t size) const {
  strncpy(title, "LOAD ", size);
  title[size - 1] = '\0';
  switch (mode_enc_.cur) {
  case LOAD_MANUAL:
    strncat(title, "MANUAL", size - strlen(title) - 1);
    break;
  case LOAD_QUEUE:
    strncat(title, "QUEUE", size - strlen(title) - 1);
    break;
  case LOAD_AUTO:
  default:
    strncat(title, "AUTO", size - strlen(title) - 1);
    break;
  }
}

void GridIOOverlay::sync_preview_grid() {
  if (grid_page.cur_grid != GridIOPage::old_grid) {
    grid_page.cur_grid = GridIOPage::old_grid;
    grid_page.reload_slot_models = false;
  }
  grid_page.load_slot_models();
  grid_page.reload_slot_models = true;
}

#endif // PLATFORM_TBD
