/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef GRIDPAGE_H__
#define GRIDPAGE_H__

#include "GUI.h"
#include "GridRowHeader.h"

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

  uint8_t cursor_x = 0;
  uint8_t cursoy_y = 0;
  uint8_t col = 0;
  uint8_t row = 0;
  uint8_t cur_col = 0;
  uint8_t cur_row = 0;
  uint8_t old_col = 0;
  uint8_t display_name = 0;
  uint8_t bank = 0;

  bool reload_slot_models;
  bool show_slot_menu = false;
  bool slot_menu_hold = false;
  bool write_cfg = false;

  uint8_t active_slots[NUM_SLOTS];

  uint8_t grid_select_apply;
  uint8_t slot_clear;

  uint8_t slot_apply;
  uint8_t slot_copy;
  uint8_t slot_paste;
  uint8_t slot_undo;
  uint8_t slot_undo_x;
  uint8_t slot_undo_y;
  uint8_t slot_load;
  uint8_t insert_rows;

  uint16_t grid_lastclock;

  uint8_t row_scan = 0;
  uint64_t row_states[2];

  PageIndex last_page = NULL_PAGE;

  uint8_t bank_popup = 0;
  uint16_t bank_popup_lastclock;
  uint16_t bank_popup_loadmask;

  bool draw_encoders;
  uint16_t draw_encoders_lastclock;
  GridPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
           Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {}
  virtual bool handleEvent(gui_event_t *event);
  void displayScroll(uint8_t i);
  uint8_t getWidth();
  uint8_t getCol();
  uint8_t getRow();

  void jump_to_row(uint8_t row);
  void load_row(uint8_t n, uint8_t row);

  void row_state_scan();
  void update_row_state(uint8_t row, bool state);
  void set_active_row(uint8_t row);
  void send_active_row();
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
  bool swap_grids();
  void apply_slot_changes(bool ignore_undo = false, bool ignore_func = false);

  void load_old_col();
  void close_bank_popup();

  void loop();
  void send_row_led();
};

extern void apply_slot_changes_cb();
extern void rename_row();
extern void gen_menu_row_names();

#endif /* GRIDPAGE_H__ */
