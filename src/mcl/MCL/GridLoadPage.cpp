#include "GridChain.h"
#include "GridLoadPage.h"
#include "GridPages.h"
#include "GridTask.h"
#include "MCLActions.h"
#include "MCLGUI.h"
#include "MCLSysConfig.h"
#include "../Drivers/MD/MD.h"
#include "MidiClock.h"
#include "Project.h"
#include "MCLStrings.h"

#ifdef PLATFORM_TBD
static constexpr uint8_t kLoadModeEncoder = 1;
static constexpr uint8_t kLoadLenEncoder = 2;
#else
static constexpr uint8_t kLoadModeEncoder = 0;
static constexpr uint8_t kLoadLenEncoder = 1;
#endif

void GridLoadPage::init() {
  GridIOPage::init();
  note_interface.init_notes();
#ifdef PLATFORM_TBD
  grid_page.load_slot_models();
  paint_track_select_leds();
#else
  key_interface.send_md_leds(TRIGLED_OVERLAY);
#endif
  key_interface.on();
  // GUI.display();
  encoders[kLoadModeEncoder]->cur = mcl_cfg.load_mode;
  encoders[kLoadLenEncoder]->cur = mcl_cfg.chain_queue_length;
  encoders[3]->cur = mcl_cfg.chain_load_quant;

  md_popup_title(mcl_cfg.load_mode);
  draw_popup();
}

void GridLoadPage::get_mode_str(char *str, uint8_t mode) {
  switch (mode) {
  case LOAD_MANUAL: {
    strcpy_P(str, mclstr_manual);
    break;
  }

  case LOAD_QUEUE: {
    strcpy_P(str, mclstr_queue);
    break;
  }

  case LOAD_AUTO: {
    strcpy_P(str, mclstr_auto);
    break;
  }
  }
}
void GridLoadPage::md_popup_title(uint8_t mode, bool persistent) {
  char modestr[16] = "LOAD ";
  get_mode_str(modestr + 5, mode);
  MD.popup_text(modestr, persistent);
}

void GridLoadPage::draw_popup() {
  draw_popup_P(mclstr_load_tracks);
}

void GridLoadPage::display_load() {
  const char *str2 = " SLOTS";
  const char *str1 = "LOAD";
  if (mcl_cfg.load_mode == LOAD_QUEUE) {
    str1 = "QUEUE";
  }
  char str3[16] = "";
  strcat(str3, str1);
  strcat(str3, str2);
  MD.popup_text(str3);
  oled_display.textbox(str1, str2);
}

void GridLoadPage::loop() {
  if (encoders[kLoadModeEncoder]->hasChanged()) {
    mcl_cfg.load_mode = encoders[kLoadModeEncoder]->cur;
    md_popup_title(mcl_cfg.load_mode);
  }
  if (encoders[kLoadLenEncoder]->hasChanged()) {
    if (encoders[kLoadModeEncoder]->cur == LOAD_QUEUE) {
      mcl_cfg.chain_queue_length = encoders[kLoadLenEncoder]->cur;
    } else {
      // Lock encoder
      encoders[kLoadLenEncoder]->cur = encoders[kLoadLenEncoder]->old;
    }
  }
  if (encoders[3]->hasChanged()) {
    mcl_cfg.chain_load_quant = encoders[3]->cur;
  }
}

void GridLoadPage::get_modestr(char *modestr) {
  switch (mcl_cfg.load_mode) {
  case LOAD_MANUAL: {
    strcpy_P(modestr, mclstr_manual_short);
    break;
  }

  case LOAD_QUEUE: {
    strcpy_P(modestr, mclstr_queue_short);
    break;
  }

  case LOAD_AUTO: {
    strcpy_P(modestr, mclstr_auto_short);
    break;
  }
  }
}

void GridLoadPage::group_select() {
  show_group_select_ui(mclstr_load_groups);
}

void GridLoadPage::display() {
  display_at(0);
}

void GridLoadPage::display_at(uint8_t y_offset) {

  const uint8_t body_y_offset = content_y_offset(y_offset);
  const uint8_t menu_y = MCLGUI::s_menu_y + body_y_offset;

  oled_display.setFont(&TomThumb);
  if (show_track_type) {
    char str[16];
    mclstr_copy_progmem(str, mclstr_load_groups, sizeof(str));
    draw_title(str, y_offset);
    mcl_gui.draw_track_type_select(mcl_cfg.track_type_select, y_offset);
  } else {
#ifdef PLATFORM_TBD
    if (y_offset >= 32) {
      clear_body(y_offset);
      draw_tbd_panel_header("LOAD", y_offset);

      char K[4] = {'\0'};
      char modestr[7];
      get_modestr(modestr);

      if (show_offset) {
        if (offset < NUM_SLOTS) {
          mcl_gui.put_value_at(offset + 1, K);
        } else {
          strcpy_P(K, mclstr_dash);
        }
        mcl_gui.draw_text_encoder(42, y_offset + 15, "DST", K, false, false);
      } else {
        mcl_gui.draw_text_encoder(30, y_offset + 15, mclstr_mode, modestr);

        if (mcl_cfg.load_mode == LOAD_QUEUE) {
          if (mcl_cfg.chain_queue_length == 1) {
            strcpy_P(K, mclstr_dash);
          } else {
            mcl_gui.put_value_at(mcl_cfg.chain_queue_length, K);
          }
          mcl_gui.draw_text_encoder(62, y_offset + 15, mclstr_len, K);
        }

        if (mcl_cfg.chain_load_quant == 1) {
          strcpy_P(K, mclstr_dash);
        } else {
          mcl_gui.put_value_at(mcl_cfg.chain_load_quant, K);
        }
        mcl_gui.draw_text_encoder(96, y_offset + 15, mclstr_quant, K);
      }
      return;
    }
#endif
    clear_body(y_offset);
    uint16_t trig_mask = note_interface.notes_off | note_interface.notes_on;
    //    mcl_gui.draw_text_encoder(MCLGUI::s_menu_x + 4, MCLGUI::s_menu_y + 8,
    //                              "STEP", K);
    if (show_offset) {
      oled_display.setCursor(MCLGUI::s_menu_x + 26, 14 + body_y_offset);
      mcl_print_P(mclstr_destination);
      trig_mask = 0;
      if (offset < NUM_SLOTS && offset / GRID_WIDTH == old_grid) {
        SET_BIT16(trig_mask, offset % GRID_WIDTH);
      }
      // if (offset < 16) {
      //   oled_display.setCursor(MCLGUI::s_menu_x + 4 + offset * MCLGUI::seq_w
      //   + 1, 16);
      //  oled_display.print(">");
      // }
    } else {

      draw_grid_marker(body_y_offset);
      char K[4] = {'\0'};

      char modestr[7];
      get_modestr(modestr);

      mcl_gui.draw_text_encoder(MCLGUI::s_menu_x + 4 + 9, menu_y + 7,
                                mclstr_mode, modestr);

      if (mcl_cfg.load_mode == LOAD_QUEUE) {
        if (mcl_cfg.chain_queue_length == 1) {
          strcpy_P(K, mclstr_dash);
        } else {
          mcl_gui.put_value_at(mcl_cfg.chain_queue_length, K);
        }
        mcl_gui.draw_text_encoder(MCLGUI::s_menu_x + 28 + 9, menu_y + 7,
                                  mclstr_len, K);
      }
      // draw quantize
      if (mcl_cfg.chain_load_quant == 1) {
        strcpy_P(K, mclstr_dash);
      } else {
        mcl_gui.put_value_at(mcl_cfg.chain_load_quant, K);
      }
      mcl_gui.draw_text_encoder(MCLGUI::s_menu_x + MCLGUI::s_menu_w - 38,
                                menu_y + 7, mclstr_quant, K);

      // draw step count
    }
    oled_display.setFont(&TomThumb);
    uint8_t step_count = (MidiClock.div16th_counter - mcl_actions.start_clock32th / 2) % 64;

    oled_display.setCursor(MCLGUI::s_menu_x + MCLGUI::s_menu_w - 11,
                           menu_y + 4 + 17);
    oled_display.print(step_count);

#ifndef PLATFORM_TBD
    mcl_gui.draw_trigs(MCLGUI::s_menu_x + 4, menu_y + 4 + 20,
                       trig_mask);
#endif
    // draw data flow in the center
    /*
    oled_display.setCursor(48, MCLGUI::s_menu_y + 4 + 12);
    mcl_print_P(mclstr_snd);
    oled_display.setCursor(46, MCLGUI::s_menu_y + 4 + 19);
    mcl_print_P(mclstr_grid);

    mcl_gui.draw_horizontal_arrow(63, MCLGUI::s_menu_y + 4 + 8, 5);
    mcl_gui.draw_horizontal_arrow(63, MCLGUI::s_menu_y + 4 + 15, 5);

    oled_display.setCursor(74, MCLGUI::s_menu_y + 4 + 12);
    mcl_print_P(mclstr_md_prefix);
    oled_display.setCursor(74, MCLGUI::s_menu_y + 4 + 19);
    mcl_print_P(mclstr_seq);
    */
  }
}
void GridLoadPage::load() {

  uint8_t track_select_array[NUM_SLOTS] = {0};

  populate_track_select_from_notes(track_select_array);
  grid_task.load_queue.put(mcl_cfg.load_mode, grid_page.getRow(),
                           track_select_array, offset);
  mcl.setPage(GRID_PAGE);
}

void GridLoadPage::group_load(uint8_t row, uint8_t load_offset) {

  if (row >= GRID_LENGTH) {
    return;
  }
  uint8_t track_select_array[NUM_SLOTS] = {0};
  track_select_array_from_type_select(track_select_array);
  //   load_tracks_to_md(-1);
  // if (!silent) {
  //   oled_display.textbox("LOAD GROUPS", "");
  // }
  // oled_display.display();

  mcl_actions.write_original = 1;
  grid_task.load_queue.put(mcl_cfg.load_mode, row, track_select_array, load_offset);
}

bool GridLoadPage::handleEvent(gui_event_t *event) {
  // Call parent GUI handler first.
  if (GridIOPage::handleEvent(event)) {
    return true;
  }
  DEBUG_DUMP(event->source);
  if (EVENT_CMD(event)) {
    uint8_t key = event->source;
    if (event->mask == EVENT_BUTTON_PRESSED) {
      switch (key) {
      default: {
        mcl.setPage(GRID_PAGE);
        return false;
      }
      case MDX_KEY_FUNC: {
        if (mcl_cfg.load_mode == LOAD_MANUAL) {
          show_offset = !show_offset;
          note_interface.init_notes();
          if (show_offset) {
            offset = 255;
          }
        }
        return true;
      }
      case MDX_KEY_BANKA:
      case MDX_KEY_BANKB:
      case MDX_KEY_BANKC: {
        if (!key_interface.is_key_down(MDX_KEY_FUNC)) {
          encoders[kLoadModeEncoder]->cur = key - MDX_KEY_BANKA + 1;
          return true;
        }
      }
      }
    }

    if (event->mask == EVENT_BUTTON_RELEASED) {
      switch (key) {
      case MDX_KEY_YES:
        if (show_track_type) {
          goto load_groups;
        }
        return true;
      }
    }
    return false;
  }
  if (EVENT_BUTTON(event)) {
    if (EVENT_RELEASED(event, Buttons.BUTTON3)) {
    //  write the whole row
    load_groups:
      key_interface.off();

      group_load(grid_page.getRow(), offset);
      mcl.setPage(GRID_PAGE);
      return true;
    }
  }
  return false;
}
