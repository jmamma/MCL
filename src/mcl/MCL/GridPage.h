/* Justin Mammarella jmamma@gmail.com 2018 */

#pragma once

#include "GridRowHeader.h"
#include "GUI.h"
#include "MCL.h"
#include "MCLStrings.h"

#define MAX_VISIBLE_ROWS 4
#define MAX_VISIBLE_COLS 8

#define SLOT_DISABLED 255
#define SLOT_PENDING 254
#define SLOT_OFFSET_LOAD 253
#define SLOT_RAM_RECORD 252
#define SLOT_RAM_PLAY 251

class GridTrack;

class GridPage : public LightPage {
public:
  GridRowHeader row_headers[MAX_VISIBLE_ROWS];
#ifdef PLATFORM_TBD
  char slot_labels[MAX_VISIBLE_ROWS][GRID_WIDTH][3];
#endif

  GridColumn col = 0;
  GridRow row = 0;
  GridColumn cur_col = 0;
  GridRow cur_row = 0;
  GridColumn old_col = 255;
  uint8_t bank = 0;

  bool reload_slot_models;
  bool show_slot_menu = false;
  bool write_cfg = false;

  GridRow active_slots[NUM_SLOTS];

  GridIndex cur_grid = 0;
  uint8_t slot_clear;

  uint8_t slot_apply;
  uint8_t slot_copy;
  uint8_t slot_paste;
  uint8_t slot_undo;
  GridColumn slot_undo_x;
  GridRow slot_undo_y;
  uint8_t slot_load;
  uint8_t slot_load_sound = 1;

  uint16_t grid_lastclock;

  GridRow row_scan = 0;
  uint64_t row_states[2];

  PageIndex last_page = NULL_PAGE;

  // bank_popup states:
  //   0 = closed
  //   1 = bank held (preview, MD-bank-key entry)
  //   2 = pattern stage (trig press loads/chains in current bank)
  uint8_t bank_popup = 0;
  uint16_t bank_popup_lastclock;
  uint16_t bank_popup_loadmask;
#ifdef PLATFORM_TBD
  // First trig pressed inside the popup, recorded so the LED repaint can
  // distinguish "head" (red) from "chained" (yellow). 255 = none yet.
  uint8_t bank_popup_first_trig = 255;
  // OLED bank-grid is hidden once trig presses start so the user can see
  // the underlying page; re-shown on the next arrow press.
  bool bank_popup_oled_visible = true;
#endif

  bool draw_encoders;
  uint16_t draw_encoders_lastclock;
  GridPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
           Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {}
  virtual bool handleEvent(gui_event_t *event);
  GridSpan getWidth();
  GridColumn getCol();
  GridRow getRow();

  void jump_to_row(GridRow row);
  void load_row(uint8_t n, GridRow row);

  void row_state_scan();
  void update_row_state(GridRow row, bool state);
  void set_active_row(GridRow row);
  bool is_slot_queue(uint8_t x, uint8_t y);
  void load_slot_models();
  void display_slot_menu();
  void display_counters();
  void display_grid_info();
  void display_grid();
  void display_row_info();
  void display();
  void display_oled();
  void setup();
  void cleanup();
  void init();
  void apply_slot_changes(bool ignore_undo = false, bool ignore_func = false);

  void load_old_col();
  void close_bank_popup();

  void loop();
  void send_row_led();
};

extern void apply_slot_changes_cb();
extern void rename_row();
extern void gen_menu_row_names();
