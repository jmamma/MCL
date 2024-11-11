#include "MCL_impl.h"

void GridLoadPage::init() {
  GridIOPage::init();
  note_interface.init_notes();
  trig_interface.send_md_leds(TRIGLED_OVERLAY);
  trig_interface.on();
  // GUI.display();
  encoders[0]->cur = mcl_cfg.load_mode;
  encoders[1]->cur = mcl_cfg.chain_queue_length;
  encoders[3]->cur = mcl_cfg.chain_load_quant;

  draw_popup_title(mcl_cfg.load_mode);
}

void GridLoadPage::setup() {}

void GridLoadPage::get_mode_str(char *str, uint8_t mode) {
  switch (mode) {
  case LOAD_MANUAL: {
    strcpy(str, "MANUAL");
    break;
  }

  case LOAD_QUEUE: {
    strcpy(str, "QUEUE");
    break;
  }

  case LOAD_AUTO: {
    strcpy(str, "AUTO");
    break;
  }
  }
}
void GridLoadPage::draw_popup_title(uint8_t mode, bool persistent) {
  char modestr[16] = "LOAD ";
  get_mode_str(modestr + 5, mode);
  MD.popup_text(modestr, persistent);
}

void GridLoadPage::draw_popup() {
  char str[16];
  strcpy(str, "GROUP LOAD");

  if (!show_track_type) {
    strcpy(str, "LOAD TRACKS");
    //str[10] = 'X' + proj.get_grid();
  }
  mcl_gui.draw_popup(str, true);
}

void GridLoadPage::display_load() {
  const char *str2 = " SLOTS";
  const char *str1 = "LOAD";
  if (mcl_cfg.load_mode == LOAD_QUEUE) {
    str1 = "QUEUE";
  }
  char str3[16] = "";
  strcat(str3,str1);
  strcat(str3,str2);
  MD.popup_text(str3);
  oled_display.textbox(str1, str2);
}

void GridLoadPage::loop() {
  if (encoders[0]->hasChanged()) {
    mcl_cfg.load_mode = encoders[0]->cur;
    draw_popup_title(mcl_cfg.load_mode);
  }
  if (encoders[1]->hasChanged()) {
    if (encoders[0]->cur == LOAD_QUEUE) {
      mcl_cfg.chain_queue_length = encoders[1]->cur;
    } else {
      // Lock encoder
      encoders[1]->cur = encoders[1]->old;
    }
  }
  if (encoders[3]->hasChanged()) {
      mcl_cfg.chain_load_quant = encoders[3]->cur;
  }


}

void GridLoadPage::get_modestr(char *modestr) {
  switch (encoders[0]->cur) {
  case LOAD_MANUAL: {
    strcpy(modestr, "MAN");
    break;
  }

  case LOAD_QUEUE: {
    strcpy(modestr, "QUE");
    break;
  }

  case LOAD_AUTO: {
    strcpy(modestr, "AUT");
    break;
  }
  }
}

void GridLoadPage::display() {

  draw_popup();

  const uint64_t mute_mask = 0, slide_mask = 0;

  if (show_track_type) {
    mcl_gui.draw_track_type_select(mcl_cfg.track_type_select);
  }
  else {
    uint16_t trig_mask = note_interface.notes_off | note_interface.notes_on;
    //    mcl_gui.draw_text_encoder(MCLGUI::s_menu_x + 4, MCLGUI::s_menu_y + 8,
    //                              "STEP", K);
    if (show_offset) {
      oled_display.setCursor(MCLGUI::s_menu_x + 18, 12);
      oled_display.print("DESTINATION");
      trig_mask = 0;
      SET_BIT16(trig_mask,offset);
      //if (offset < 16) {
       //  oled_display.setCursor(MCLGUI::s_menu_x + 4 + offset * MCLGUI::seq_w + 1, 16);
        // oled_display.print(">");
     // }
    }
    else {


    oled_display.setFont(&Elektrothic);
    oled_display.setCursor(MCLGUI::s_menu_x + 4, 21);
    oled_display.print((char) (0x3A +  proj.get_grid()));

    oled_display.setFont(&TomThumb);
    char K[4] = {'\0'};


    char modestr[7];
    get_modestr(modestr);

    mcl_gui.draw_text_encoder(MCLGUI::s_menu_x + 4 + 9, MCLGUI::s_menu_y + 7,
                              "MODE", modestr);


    if (encoders[0]->getValue() == LOAD_QUEUE) {
      if (encoders[1]->getValue() == 1) {
        strcpy(K, "--");
      } else {
        mcl_gui.put_value_at(encoders[1]->cur, K);
      }
      mcl_gui.draw_text_encoder(MCLGUI::s_menu_x + 28 + 9, MCLGUI::s_menu_y + 7,
                                "LEN", K);
    }
    // draw quantize
    if (mcl_cfg.chain_load_quant == 1) {
      strcpy(K, "--");
    } else {
      mcl_gui.put_value_at(mcl_cfg.chain_load_quant, K);
    }
    mcl_gui.draw_text_encoder(MCLGUI::s_menu_x + MCLGUI::s_menu_w - 38,
                              MCLGUI::s_menu_y + 7, "QUANT", K);

    // draw step count
    }
    oled_display.setFont(&TomThumb);
    uint8_t step_count =
        (MidiClock.div16th_counter - mcl_actions.start_clock32th / 2) -
        (64 *
         ((MidiClock.div16th_counter - mcl_actions.start_clock32th / 2) / 64));
    oled_display.setCursor(MCLGUI::s_menu_x + MCLGUI::s_menu_w - 11,
                           MCLGUI::s_menu_y + 4 + 17);
    oled_display.print(step_count);


    mcl_gui.draw_trigs(MCLGUI::s_menu_x + 4, MCLGUI::s_menu_y + 4 + 20, trig_mask );
    // draw data flow in the center
    /*
    oled_display.setCursor(48, MCLGUI::s_menu_y + 4 + 12);
    oled_display.print(F("SND"));
    oled_display.setCursor(46, MCLGUI::s_menu_y + 4 + 19);
    oled_display.print(F("GRID"));

    mcl_gui.draw_horizontal_arrow(63, MCLGUI::s_menu_y + 4 + 8, 5);
    mcl_gui.draw_horizontal_arrow(63, MCLGUI::s_menu_y + 4 + 15, 5);

    oled_display.setCursor(74, MCLGUI::s_menu_y + 4 + 12);
    oled_display.print(F("MD"));
    oled_display.setCursor(74, MCLGUI::s_menu_y + 4 + 19);
    oled_display.print(F("SEQ"));
    */
  }
}
void GridLoadPage::load() {

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
  grid_task.load_queue.put(mcl_cfg.load_mode, grid_page.getRow(), track_select_array, offset);
  mcl.setPage(GRID_PAGE);
}

void GridLoadPage::group_select() {
  show_track_type = true;
  char str[] = "LOAD GROUPS";
  MD.popup_text(str, true);
  MD.set_trigleds(mcl_cfg.track_type_select, TRIGLED_EXCLUSIVE);
}

void GridLoadPage::group_load(uint8_t row, uint8_t offset_) {

  if (row >= GRID_LENGTH) { return; }
  uint8_t track_select_array[NUM_SLOTS] = {0};
  track_select_array_from_type_select(track_select_array);
  //   load_tracks_to_md(-1);
 // if (!silent) {
 //   oled_display.textbox("LOAD GROUPS", "");
 // }
  //oled_display.display();

  mcl_actions.write_original = 1;
  grid_task.load_queue.put(mcl_cfg.load_mode, row, track_select_array, offset);
}

bool GridLoadPage::handleEvent(gui_event_t *event) {
  // Call parent GUI handler first.
  if (GridIOPage::handleEvent(event)) {
    return true;
  }
  DEBUG_DUMP(event->source);
  if (EVENT_CMD(event)) {
    uint8_t key = event->source - 64;
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
          if (show_offset) { offset = 255; }
        }
        return true;
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
        if (show_track_type) {
        goto load_groups;
        }
        return true;
      }
    }
    return false;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON3)) {
  //  write the whole row
  load_groups:
    trig_interface.off();

    group_load(grid_page.getRow(), offset);
    grid_task.load_queue_handler();
    grid_task.transition_handler();
    mcl.setPage(GRID_PAGE);
    return true;
  }
  return false;
}
