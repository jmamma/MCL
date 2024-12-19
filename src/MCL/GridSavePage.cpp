#include "GridSavePage.h"
#include "GridIOPage.h"
#include "MCLGUI.h"
#include "Project.h"
#include "MidiClock.h"
#include "GridPages.h"
#include "MCLActions.h"
#include "MD.h"
#include "MDTrack.h"

#define S_PAGE 3

void GridSavePage::init() {
  GridIOPage::init();
  MD.getCurrentPattern(CALLBACK_TIMEOUT);
  trig_interface.send_md_leds(TRIGLED_OVERLAY);
  trig_interface.on();
  grid_page.reload_slot_models = false;
  char str[] = "SAVE SLOTS";
  MD.popup_text(str, true);
  draw_popup();
}

void GridSavePage::setup() {}

void GridSavePage::draw_popup() {
  char str[16] = "GROUP SAVE";

  if (!show_track_type) {
    strcpy(str, "SAVE TRACKS");
  }
  mcl_gui.draw_popup(str, true);
}

void GridSavePage::loop() {}
void GridSavePage::display() {

  draw_popup();

  const uint64_t slide_mask = 0;
  const uint64_t mute_mask = 0;
  if (show_track_type) {
    mcl_gui.draw_track_type_select(mcl_cfg.track_type_select);
  } else {
    mcl_gui.draw_trigs(MCLGUI::s_menu_x + 4, MCLGUI::s_menu_y + 24, note_interface.notes_off | note_interface.notes_on );
    oled_display.setFont(&Elektrothic);
    oled_display.setCursor(MCLGUI::s_menu_x + 4, 21);
    oled_display.print((char)(0x3A + proj.get_grid()));

    oled_display.setFont(&TomThumb);

    mcl_gui.draw_text_encoder(MCLGUI::s_menu_x + 4 + 9, MCLGUI::s_menu_y + 7,
                              "MODE", "SAVE");

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

    oled_display.setCursor(data_x + 9, MCLGUI::s_menu_y + 15);
    oled_display.print(F("SND"));
    oled_display.setCursor(data_x + 9, MCLGUI::s_menu_y + 22);
    oled_display.print(F("SEQ"));

    oled_display.drawFastHLine(data_x + 13 + 9, MCLGUI::s_menu_y + 11, 2, WHITE);
    oled_display.drawFastHLine(data_x + 13 + 9, MCLGUI::s_menu_y + 18, 2,
                               WHITE);
    oled_display.drawFastVLine(data_x + 15 + 9, MCLGUI::s_menu_y + 11, 8, WHITE);
    mcl_gui.draw_horizontal_arrow(data_x + 16 + 9, MCLGUI::s_menu_y + 15, 5);

    oled_display.setCursor(data_x + 24 + 9, MCLGUI::s_menu_y + 18);
    oled_display.print(F("GRID"));
  }
}

void GridSavePage::save() {
  oled_display.textbox("SAVE TRACKS", "");
  oled_display.display();

  uint8_t save_mode = SAVE_SEQ;
  uint8_t track_select_array[NUM_SLOTS] = {0};

  for (uint8_t n = 0; n < GRID_WIDTH; n++) {
    if (note_interface.is_note(n)) {
      SET_BIT32(track_select, n + proj.get_grid() * 16);
    }
  }

  for (uint8_t n = 0; n < NUM_SLOTS; n++) {
    if (IS_BIT_SET32(track_select, n)) {
      track_select_array[n] = 1;
    }
  }

  mcl_actions.save_tracks(grid_page.getRow(), track_select_array, save_mode);
  mcl.setPage(GRID_PAGE);
}

void GridSavePage::group_select() {
  show_track_type = true;
  char str[] = "SAVE GROUPS";
  MD.popup_text(str, true);
  MD.set_trigleds(mcl_cfg.track_type_select, TRIGLED_EXCLUSIVE);
}

bool GridSavePage::handleEvent(gui_event_t *event) {
  if (GridIOPage::handleEvent(event)) {
    return true;
  }

  if (EVENT_CMD(event)) {
    uint8_t key = event->source - 64;

    if (event->mask == EVENT_BUTTON_PRESSED) {
      switch (key) {
      default: {
        mcl.setPage(GRID_PAGE);
        return false;
      }
      case MDX_KEY_YES: {
        group_select();
        return true;
      }
      case MDX_KEY_EXTENDED: {
        return false;
      }
      case MDX_KEY_BANKA:
      case MDX_KEY_BANKB:
      case MDX_KEY_BANKC: {
        encoders[0]->cur = key - MDX_KEY_BANKA;
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

  if (EVENT_RELEASED(event, Buttons.BUTTON3)) {
  save_groups:
    trig_interface.off();
    uint8_t offset = proj.get_grid() * 16;

    uint8_t track_select_array[NUM_SLOTS] = {0};

    track_select_array_from_type_select(track_select_array);

    oled_display.textbox("SAVE GROUPS", "");
    //oled_display.display();

    uint8_t save_mode = SAVE_SEQ;

    mcl_actions.save_tracks(grid_page.getRow(), track_select_array, save_mode);
    mcl.setPage(GRID_PAGE);
    return true;
  }
  return false;
}
