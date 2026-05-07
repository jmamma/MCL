#include "GridSavePage.h"
#include "GridIOPage.h"
#include "MCLGUI.h"
#include "Project.h"
#include "MidiClock.h"
#include "GridPages.h"
#include "MCLActions.h"
#include "../Drivers/MD/MD.h"
#include "MDTrack.h"
#include "MCLStrings.h"

#define S_PAGE 3

#ifdef PLATFORM_TBD
static constexpr uint8_t kSaveModeEncoder = 1;
#else
static constexpr uint8_t kSaveModeEncoder = 0;
#endif

void GridSavePage::init() {
  GridIOPage::init();
  MD.getCurrentPattern(CALLBACK_TIMEOUT);
#ifdef PLATFORM_TBD
  grid_page.load_slot_models();
  paint_track_select_leds();
#else
  key_interface.send_md_leds(TRIGLED_OVERLAY);
#endif
  key_interface.on();
  grid_page.reload_slot_models = false;
  MD.popup_text_P(mclstr_save_slots, true);
  draw_popup();
}

void GridSavePage::draw_popup() {
  char str[16];
  mclstr_copy_progmem(str, mclstr_save_tracks, sizeof(str));
  mcl_gui.draw_popup(str, true);
#ifdef PLATFORM_TBD
  draw_title(str);
#endif
}

void GridSavePage::display() {
  display_at(0);
}

void GridSavePage::display_at(uint8_t y_offset) {
  const uint8_t body_y_offset = content_y_offset(y_offset);
  const uint8_t menu_y = MCLGUI::s_menu_y + body_y_offset;
  oled_display.setFont(&TomThumb);
  if (show_track_type) {
    char str[16];
    mclstr_copy_progmem(str, mclstr_save_groups, sizeof(str));
    draw_title(str, y_offset);
    mcl_gui.draw_track_type_select(mcl_cfg.track_type_select, y_offset);
  } else {
#ifdef PLATFORM_TBD
    if (y_offset >= 32) {
      clear_body(y_offset);
      draw_tbd_panel_header("SAVE", y_offset);

      oled_display.setFont(&TomThumb);
      oled_display.setTextColor(WHITE, BLACK);
      constexpr uint8_t flow_x = 39;
      oled_display.setCursor(flow_x, y_offset + 20);
      mcl_print_P(mclstr_name_snd);
      oled_display.print('+');
      mcl_print_P(mclstr_seq);
      mcl_gui.draw_horizontal_arrow(flow_x + 42, y_offset + 17, 5);
      oled_display.setCursor(flow_x + 55, y_offset + 20);
      mcl_print_P(mclstr_grid);
      return;
    }
#endif
    clear_body(y_offset);
#ifndef PLATFORM_TBD
    mcl_gui.draw_trigs(MCLGUI::s_menu_x + 4, menu_y + 24,
                       note_interface.notes_off | note_interface.notes_on);
#endif
    oled_display.setFont(&Elektrothic);
    oled_display.setCursor(MCLGUI::s_menu_x + 4, 21 + body_y_offset);
    oled_display.print((char)(0x3A + old_grid));

    oled_display.setFont(&TomThumb);

#ifndef PLATFORM_TBD
    char save_label[8];
    mclstr_copy_progmem(save_label, mclstr_save, sizeof(save_label));
    mcl_gui.draw_text_encoder(MCLGUI::s_menu_x + 4 + 9, menu_y + 7,
                              mclstr_mode, save_label);
#endif

    char step[4] = {'\0'};
    uint8_t step_count =
        (MidiClock.div16th_counter - mcl_actions.start_clock32th / 2) -
        (64 *
         ((MidiClock.div16th_counter - mcl_actions.start_clock32th / 2) / 64));
    mcl_gui.put_value_at(step_count, step);

    // mcl_gui.draw_text_encoder(MCLGUI::s_menu_x + MCLGUI::s_menu_w - 26,
    // MCLGUI::s_menu_y + 8, "STEP", step);

    oled_display.setFont(&TomThumb);
    // draw data flow in the center
    constexpr uint8_t data_x = 56;

    oled_display.setCursor(data_x + 9, menu_y + 15);
    mcl_print_P(mclstr_name_snd);
    oled_display.setCursor(data_x + 9, menu_y + 22);
    mcl_print_P(mclstr_seq);

    oled_display.drawFastHLine(data_x + 13 + 9, menu_y + 11, 2, WHITE);
    oled_display.drawFastHLine(data_x + 13 + 9, menu_y + 18, 2, WHITE);
    oled_display.drawFastVLine(data_x + 15 + 9, menu_y + 11, 8, WHITE);
    mcl_gui.draw_horizontal_arrow(data_x + 16 + 9, menu_y + 15, 5);

    oled_display.setCursor(data_x + 24 + 9, menu_y + 18);
    mcl_print_P(mclstr_grid);
  }
}

void GridSavePage::save() {
  oled_display.textbox_P(mclstr_save, mclstr_tracks);
  oled_display.display();

  uint8_t save_mode = SAVE_SEQ;
  uint8_t track_select_array[NUM_SLOTS] = {0};

  populate_track_select_from_notes(track_select_array);

  mcl_actions.save_tracks(grid_page.getRow(), track_select_array, save_mode);
  mcl.setPage(GRID_PAGE);
}

void GridSavePage::group_select() {
  show_group_select_ui(mclstr_save_groups);
}

bool GridSavePage::handleEvent(gui_event_t *event) {
  if (GridIOPage::handleEvent(event)) {
    return true;
  }

  if (EVENT_CMD(event)) {
    uint8_t key = event->source;

    if (event->mask == EVENT_BUTTON_PRESSED) {
      switch (key) {
      default: {
        mcl.setPage(GRID_PAGE);
        return false;
      }
      case MDX_KEY_BANKA:
      case MDX_KEY_BANKB:
      case MDX_KEY_BANKC: {
        encoders[kSaveModeEncoder]->cur = key - MDX_KEY_BANKA;
        return true;
      }
      }
    }

    if (event->mask == EVENT_BUTTON_RELEASED) {
      switch (key) {
      case MDX_KEY_YES:
        if (show_track_type) {
          goto save_groups;
        }
        return true;
      }
    }
  }
  if (EVENT_BUTTON(event)) {
    if (EVENT_RELEASED(event, Buttons.BUTTON3)) {
    save_groups:
      key_interface.off();

      uint8_t track_select_array[NUM_SLOTS] = {0};

      track_select_array_from_type_select(track_select_array);

      oled_display.textbox_P(mclstr_save, mclstr_groups);
      //oled_display.display();

      uint8_t save_mode = SAVE_SEQ;

      mcl_actions.save_tracks(grid_page.getRow(), track_select_array, save_mode);
      mcl.setPage(GRID_PAGE);
      return true;
    }
  }
  return false;
}
