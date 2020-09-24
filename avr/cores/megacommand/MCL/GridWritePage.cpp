#include "MCL_impl.h"

void GridWritePage::setup() {
  MD.getCurrentTrack(CALLBACK_TIMEOUT);
  MD.getCurrentPattern(CALLBACK_TIMEOUT);
  MD.currentKit = MD.getCurrentKit(CALLBACK_TIMEOUT);
  encoders[1]->cur =
      MD.currentPattern - 16 * ((int)MD.currentPattern / (int)16);

  patternswitch = 1;
  ((MCLEncoder *)encoders[3])->max = 6;
  if (mode == WRITE_PAGE) {
    encoders[3]->cur = 4;
    mode = CHAIN_PAGE;
  }
  ((MCLEncoder *)encoders[2])->max = 1;

  // MD.requestKit(MD.currentKit);
  trig_interface.on();
  note_interface.state = true;
  // GUI.display();
  curpage = W_PAGE;
}

void GridWritePage::init() {
  show_track_type_select = false;
#ifdef OLED_DISPLAY
  mcl_gui.draw_popup("CHAIN FROM GRID", true, 28);
#endif
}

void GridWritePage::cleanup() {}

void GridWritePage::display() {

  mcl_gui.clear_popup(28);

  const uint64_t mute_mask = 0, slide_mask = 0;

  auto oldfont = oled_display.getFont();
  oled_display.setFont(&TomThumb);

  if (show_track_type_select) {
    mcl_gui.draw_track_type_select(42, MCLGUI::s_menu_y + 16,
                                   track_type_select);
  } else {
    mcl_gui.draw_trigs(MCLGUI::s_menu_x + 4, MCLGUI::s_menu_y + 21, 0, 0, 0, 16,
                       mute_mask, slide_mask);

    char K[4] = {'\0'};

    // draw step count
    uint8_t step_count =
        (MidiClock.div16th_counter - mcl_actions.start_clock32th / 2) -
        (64 *
         ((MidiClock.div16th_counter - mcl_actions.start_clock32th / 2) / 64));
    itoa(step_count, K, 10);
    mcl_gui.draw_text_encoder(MCLGUI::s_menu_x + 4, MCLGUI::s_menu_y + 4,
                              "STEP", K);

    // draw quantize
    strcpy(K, "---");
    if ((encoders[3]->getValue() < 7) && (encoders[3]->getValue() > 0)) {
      uint8_t x = 1 << encoders[3]->getValue();
      itoa(x, K, 10);
    }
    mcl_gui.draw_text_encoder(MCLGUI::s_menu_x + MCLGUI::s_menu_w - 26,
                              MCLGUI::s_menu_y + 4, "QUANT", K);

    oled_display.setFont(&TomThumb);
    // draw data flow in the center
    oled_display.setCursor(48, MCLGUI::s_menu_y + 12);
    oled_display.print("SND");
    oled_display.setCursor(46, MCLGUI::s_menu_y + 19);
    oled_display.print("GRID");

    mcl_gui.draw_horizontal_arrow(63, MCLGUI::s_menu_y + 8, 5);
    mcl_gui.draw_horizontal_arrow(63, MCLGUI::s_menu_y + 15, 5);

    oled_display.setCursor(74, MCLGUI::s_menu_y + 12);
    oled_display.print("MD");
    oled_display.setCursor(74, MCLGUI::s_menu_y + 19);
    oled_display.print("SEQ");
  }

  oled_display.display();
  oled_display.setFont(oldfont);
}

bool GridWritePage::handleEvent(gui_event_t *event) {
  // Call parent GUI handler first.
  if (GridIOPage::handleEvent(event)) {
    return true;
  }
  DEBUG_DUMP(event->source);
  if (note_interface.is_event(event)) {
    DEBUG_PRINTLN(F("note event"));
    uint8_t track = event->source - 128;
    if (show_track_type_select) {
      if ((event->mask == EVENT_BUTTON_PRESSED) && (track < 3)) {
        TOGGLE_BIT(track_type_select, track);
      }
    } else {
      trig_interface.send_md_leds();
      if (note_interface.notes_all_off()) {
        DEBUG_PRINTLN(F("notes all off"));
        if (BUTTON_DOWN(Buttons.BUTTON2)) {
          return true;
        } else {
          oled_display.textbox("CHAIN SLOTS", "");
          oled_display.display();
          /// !Note, note_off_event has reentry issues, so we have to first set
          /// the page to avoid driving this code path again.

          uint8_t offset = proj.get_grid() * 16;

          uint8_t track_select_array[NUM_SLOTS] = {0};

          for (uint8_t n = 0; n < GRID_WIDTH; n++) {
            if (note_interface.notes[n] == 3) {
              track_select_array[n + offset] = 1;
            }
          }
          GUI.setPage(&grid_page);
          trig_interface.off();
          mcl_actions.write_tracks(0, grid_page.encoders[1]->getValue(),
                                   track_select_array);
        }
        curpage = 0;
      }
    }
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
  mcl_gui.draw_popup("GROUP CHAIN", true, 28);
  show_track_type_select = true;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON3)) {
    //  write the whole row

    trig_interface.off();
    uint8_t offset = proj.get_grid() * 16;

    uint8_t track_select_array[NUM_SLOTS] = {0};

    uint8_t grid_idx, track_idx, track_type, dev_idx;
    bool is_aux;

    for (uint8_t n = 0; n < NUM_SLOTS; n++) {
      SeqTrack *seq_track =
          mcl_actions.get_dev_slot_info(n, &grid_idx, &track_idx, &track_type, &dev_idx, &is_aux);
          if (track_type == 255)
              continue;
          DEBUG_DUMP(n);
          DEBUG_DUMP(dev_idx);
          if (!is_aux && IS_BIT_SET(track_type_select, dev_idx)) {
          track_select_array[n] = 1;
          }
          //AUX tracks
          if (is_aux && IS_BIT_SET(track_type_select, 2)) {
          track_select_array[n] = 1;
          }
    }
    //   write_tracks_to_md(-1);
#ifdef OLED_DISPLAY
    if (MidiClock.state != 2) {
      oled_display.textbox("CHAIN GROUPS", "");
      oled_display.display();
    } else {
      oled_display.textbox("CHAIN PAT", "");
      oled_display.display();
    }
#endif
    mcl_actions.write_original = 1;
    GUI.setPage(&grid_page);
    mcl_actions.write_tracks(0, grid_page.encoders[1]->getValue(),
                             track_select_array);
    curpage = 0;
    return true;
  }
}
