#include "MCL_impl.h"

void GridLoadPage::init() {
  GridIOPage::init();
  MD.currentKit = MD.getCurrentKit(CALLBACK_TIMEOUT);
  // MD.requestKit(MD.currentKit);
  note_interface.init_notes();
  trig_interface.send_md_leds(TRIGLED_OVERLAY);
  trig_interface.on();
  draw_popup_title();
  note_interface.state = true;
  // GUI.display();
  encoders[0]->cur = mcl_cfg.chain_mode;
  encoders[1]->cur = mcl_cfg.chain_queue_length;
  encoders[3]->cur = mcl_cfg.chain_load_quant;
}

void GridLoadPage::setup() {}

void GridLoadPage::get_mode_str(char *str) {
  switch (encoders[0]->cur) {
  case CHAIN_MANUAL: {
    strcpy(str, "MANUAL");
    break;
  }

  case CHAIN_QUEUE: {
    strcpy(str, "QUEUE");
    break;
  }

  case CHAIN_AUTO: {
    strcpy(str, "AUTO");
    break;
  }
  }
}
void GridLoadPage::draw_popup_title() {
  char modestr[16] = "LOAD ";
  get_mode_str(modestr + 5);
  MD.popup_text(modestr, true);
}

void GridLoadPage::draw_popup() {
  char str[16];
  strcpy(str, "GROUP LOAD");

  if (!show_track_type) {
    strcpy(str, "LOAD FROM  ");
    str[10] = 'X' + proj.get_grid();
  }
  mcl_gui.draw_popup(str, true, 28);
}

void GridLoadPage::get_modestr(char *modestr) {
  if (encoders[0]->hasChanged()) {
    mcl_cfg.chain_mode = encoders[0]->cur;
    draw_popup_title();
  }
  if (encoders[1]->hasChanged()) {
    if (encoders[0]->cur == CHAIN_QUEUE) {
      mcl_cfg.chain_queue_length = encoders[1]->cur;
    } else {
      // Lock encoder
      encoders[1]->cur = encoders[1]->old;
    }
  }
  if (encoders[3]->hasChanged()) {
    mcl_cfg.chain_load_quant = encoders[3]->cur;
  }

  switch (encoders[0]->cur) {
  case CHAIN_MANUAL: {
    strcpy(modestr, "MAN");
    break;
  }

  case CHAIN_QUEUE: {
    strcpy(modestr, "QUE");
    break;
  }

  case CHAIN_AUTO: {
    strcpy(modestr, "AUT");
    break;
  }
  }
}

void GridLoadPage::display() {

  draw_popup();

  const uint64_t mute_mask = 0, slide_mask = 0;

  auto oldfont = oled_display.getFont();
  oled_display.setFont(&TomThumb);

  if (show_track_type) {
    mcl_gui.draw_track_type_select(36, MCLGUI::s_menu_y + 12,
                                   mcl_cfg.track_type_select);
  } else {
    mcl_gui.draw_trigs(MCLGUI::s_menu_x + 4, MCLGUI::s_menu_y + 21, 0, 0, 0, 16,
                       mute_mask, slide_mask);

    char K[4] = {'\0'};

    //    mcl_gui.draw_text_encoder(MCLGUI::s_menu_x + 4, MCLGUI::s_menu_y + 4,
    //                              "STEP", K);

    char modestr[7];
    get_modestr(modestr);

    mcl_gui.draw_text_encoder(MCLGUI::s_menu_x + 4, MCLGUI::s_menu_y + 4,
                              "CHAIN", modestr);

    if (encoders[0]->getValue() == CHAIN_QUEUE) {
      if (encoders[1]->getValue() == 1) {
        strcpy(K, "--");
      } else {
        uint8_t x = 1 << encoders[1]->getValue();
        mcl_gui.put_value_at(x, K);
      }
      mcl_gui.draw_text_encoder(MCLGUI::s_menu_x + 28, MCLGUI::s_menu_y + 4,
                                "LEN", K);
    }
    // draw quantize
    if (encoders[3]->getValue() == 1) {
      strcpy(K, "--");
    } else {
      uint8_t x = 1 << encoders[3]->getValue();
      mcl_gui.put_value_at(x, K);
    }
    mcl_gui.draw_text_encoder(MCLGUI::s_menu_x + MCLGUI::s_menu_w - 38,
                              MCLGUI::s_menu_y + 4, "QUANT", K);

    oled_display.setFont(&TomThumb);
    // draw step count
    uint8_t step_count =
        (MidiClock.div16th_counter - mcl_actions.start_clock32th / 2) -
        (64 *
         ((MidiClock.div16th_counter - mcl_actions.start_clock32th / 2) / 64));
    oled_display.setCursor(MCLGUI::s_menu_x + MCLGUI::s_menu_w - 11,
                           MCLGUI::s_menu_y + 18);
    oled_display.print(step_count);

    // draw data flow in the center
    /*
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
    */
  }
  oled_display.display();
  oled_display.setFont(oldfont);
}
void GridLoadPage::load() {
  oled_display.textbox("LOAD SLOTS", "");
  oled_display.display();
  /// !Note, note_off_event has reentry issues, so we have to first set
  /// the page to avoid driving this code path again.

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
  GUI.setPage(&grid_page);
  trig_interface.off();
  mcl_actions.load_tracks(grid_page.encoders[1]->getValue(),
                          track_select_array);
}

void GridLoadPage::group_select() {
  show_track_type = true;
  MD.popup_text("LOAD GROUPS", true);
  MD.set_trigleds(mcl_cfg.track_type_select, TRIGLED_EXCLUSIVE);
}

void GridLoadPage::group_load(uint8_t row) {

  uint8_t track_select_array[NUM_SLOTS] = {0};
  track_select_array_from_type_select(track_select_array);
  //   load_tracks_to_md(-1);
  oled_display.textbox("LOAD GROUPS", "");
  oled_display.display();
  mcl_actions.write_original = 1;
  mcl_actions.load_tracks(row, track_select_array);
}

bool GridLoadPage::handleEvent(gui_event_t *event) {
  // Call parent GUI handler first.
  if (GridIOPage::handleEvent(event)) {
    return true;
  }
  DEBUG_DUMP(event->source);
  if (note_interface.is_event(event)) {
    uint8_t track = event->source - 128;
    if (event->mask == EVENT_BUTTON_PRESSED) {
      if (show_track_type) {
        if (track < 4) {
          TOGGLE_BIT16(mcl_cfg.track_type_select, track);
          MD.set_trigleds(mcl_cfg.track_type_select, TRIGLED_EXCLUSIVE);
        }
      } else {
        trig_interface.send_md_leds(TRIGLED_OVERLAY);
      }
    } else {
      if (!show_track_type) {
        trig_interface.send_md_leds(TRIGLED_OVERLAY);
        if (note_interface.notes_all_off()) {
          DEBUG_PRINTLN(F("notes all off"));
          if (BUTTON_DOWN(Buttons.BUTTON2)) {
            return true;
          } else {
            load();
          }
        }
      }
    }
    return true;
  }

  if (EVENT_CMD(event)) {
    uint8_t key = event->source - 64;
    if (event->mask == EVENT_BUTTON_PRESSED) {
      switch (key) {
      case MDX_KEY_YES: {
        group_select();
        return true;
      }
      case MDX_KEY_BANKA:
      case MDX_KEY_BANKB:
      case MDX_KEY_BANKC: {
        if (!trig_interface.is_key_down(MDX_KEY_FUNC)) {
        encoders[0]->cur = key - MDX_KEY_BANKA + 1;
        return true;
        }
      }
      }
    }

    if (event->mask == EVENT_BUTTON_RELEASED) {
      switch (key) {
      case MDX_KEY_YES:
        goto load_groups;
      }
    }
    return false;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON3)) {
  //  write the whole row
  load_groups:
    trig_interface.off();

    group_load(grid_page.encoders[1]->getValue());

    GUI.setPage(&grid_page);
    return true;
  }
}
