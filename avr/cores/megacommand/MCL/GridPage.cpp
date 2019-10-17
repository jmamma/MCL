#include "GridPage.h"
#include "GUI.h"
#include "GridPages.h"
#include "MCL.h"
void GridPage::init() {
  show_slot_menu = false;
  reload_slot_models = false;
  md_exploit.off();
  load_slot_models();
#ifdef OLED_DISPLAY
  oled_display.clearDisplay();
#endif
}

void GridPage::setup() {
  uint8_t charmap[8] = {10, 10, 10, 10, 10, 10, 10, 00};
  LCD.createChar(1, charmap);
  frames_startclock = slowclock;
  encoders[0]->cur = encoders[0]->old = mcl_cfg.col;
  encoders[1]->cur = encoders[1]->old = mcl_cfg.row;
  cur_col = mcl_cfg.cur_col;
  cur_row = mcl_cfg.cur_row;
  /*
  cur_col = 0;
  if (mcl_cfg.row < MAX_VISIBLE_ROWS) { cur_row = mcl_cfg.row; }
  else { cur_row = MAX_VISIBLE_ROWS - 1; }
  */
  for (uint8_t n = 0; n < NUM_TRACKS; n++) {
    active_slots[n] = -1;
  }
}
void GridPage::cleanup() {
#ifdef OLED_DISPLAY
  oled_display.setFont();
  oled_display.setTextColor(WHITE, BLACK);
#endif
}

void GridPage::loop() {
  midi_active_peering.check();
  int8_t diff, new_val;
#ifdef OLED_DISPLAY
  if (show_slot_menu) {
    grid_slot_page.loop();
    return;
  } else {
  }

  if (encoders[0]->hasChanged()) {
    diff = encoders[0]->cur - encoders[0]->old;
    new_val = cur_col + diff;
    if (new_val > MAX_VISIBLE_COLS - 1) {
      new_val = MAX_VISIBLE_COLS - 1;
    }
    if (new_val < 0) {
      new_val = 0;
    }
    cur_col = new_val;
  }

  if (encoders[1]->hasChanged()) {
    diff = encoders[1]->cur - encoders[1]->old;
    new_val = cur_row + diff;

    if (new_val > MAX_VISIBLE_ROWS - 1) {
      new_val = MAX_VISIBLE_ROWS - 1;
    }
    if (new_val < 0) {
      new_val = 0;
    }
    // MD.assignMachine(0, encoders[1]->cur);
    cur_row = new_val;
    if ((cur_row == MAX_VISIBLE_ROWS - 1) || (cur_row == 0)) {
      load_slot_models();
    }
    reload_slot_models = true;
    grid_lastclock = slowclock;

    volatile uint8_t *ptr;
    write_cfg = true;
  }

#else
  cur_col = encoders[0]->cur;
  // cur_row = encoders[1]->cur;
  cur_row = 0;
  if (encoders[1]->hasChanged()) {
    grid_lastclock = slowclock;
    reload_slot_models = false;
    write_cfg = true;
  }
#endif

  if (!reload_slot_models) {
    load_slot_models();
    reload_slot_models = true;
  }
  /*
    if (_DOWN(Buttons.BUTTON3) && (encoders[2]->hasChanged())) {
      toggle_fx1();
    }

    if (_DOWN(Buttons.BUTTON3) && (encoders[3]->hasChanged())) {
      toggle_fx2();
    }
    */

  if (slowclock < grid_lastclock) {
    grid_lastclock = slowclock + GUI_NAME_TIMEOUT;
  }

  if (clock_diff(grid_lastclock, slowclock) > GUI_NAME_TIMEOUT) {
    ///   DEBUG_DUMP(grid_lastclock);
    //   DEBUG_DUMP(slowclock);
    //   display_name = 1;
    if ((write_cfg) && (MidiClock.state != 2)) {
      mcl_cfg.cur_col = cur_col;
      mcl_cfg.cur_row = cur_row;

      mcl_cfg.col = encoders[0]->cur;
      mcl_cfg.row = encoders[1]->cur;

      mcl_cfg.tempo = MidiClock.get_tempo();
      DEBUG_PRINTLN("write cfg");
      mcl_cfg.write_cfg();
      grid_lastclock = slowclock;
      write_cfg = false;
      // }
    }
    // display_name = 0;
  }
}
void GridPage::displayScroll(uint8_t i) {
  if (encoders[i] != NULL) {

    if (((encoders[0]->getValue() + i + 1) % 4) == 0) {
      char strn[2] = "I";
      strn[0] = (char)001;
      //           strn[0] = (char) 219;
      GUI.setLine(GUI.LINE1);

      GUI.put_string_at_noterminator((2 + (i * 3)), strn);

      GUI.setLine(GUI.LINE2);
      GUI.put_string_at_noterminator((2 + (i * 3)), strn);
    }

    else {
      char strn_scroll[2] = "|";
      GUI.setLine(GUI.LINE1);

      GUI.put_string_at_noterminator((2 + (i * 3)), strn_scroll);

      GUI.setLine(GUI.LINE2);
      GUI.put_string_at_noterminator((2 + (i * 3)), strn_scroll);
    }
  }
}

void encoder_fx_handle(Encoder *enc) {
  GridEncoder *mdEnc = (GridEncoder *)enc;

  /*Scale delay feedback for safe ranges*/

  if (mdEnc->fxparam == MD_ECHO_FB) {
    if (mdEnc->getValue() > 68) {
      mdEnc->setValue(68);
    }
  }
  USE_LOCK();
  SET_LOCK();
  MD.sendFXParam(mdEnc->fxparam, mdEnc->getValue(), mdEnc->effect);
  CLEAR_LOCK();
}

uint8_t GridPage::getRow() { return param2.cur; }

uint8_t GridPage::getCol() { return param1.cur; }

void GridPage::load_slot_models() {
#ifdef OLED_DISPLAY
  for (uint8_t n = 0; n < MAX_VISIBLE_ROWS; n++) {
    row_headers[n].read(getRow() - cur_row + n);
  }
#else

  row_headers[0].read(getRow());

#endif
}
void GridPage::tick_frames() {
  uint16_t current_clock = slowclock;

  frames += 1;
  if (clock_diff(frames_startclock, current_clock) >= 400) {
    frames_startclock = slowclock;
    frames = 0;
  }
  if (clock_diff(frames_startclock, current_clock) >= 250) {
    frames_fps = frames;
    // DEBUG_DUMP((float)frames * (float)4);
    // frames_fps = ((frames + frames_fps)/ 2);
    frames = 0;
    frames_startclock = slowclock;
  }
}

void GridPage::toggle_fx1() {
  dispeffect = 1;
  GridEncoder *enc = (GridEncoder *)encoders[2];
  if (enc->effect == MD_FX_REV) {
    fx_dc = enc->getValue();
    enc->setValue(fx_tm);

    enc->effect = MD_FX_ECHO;
    enc->fxparam = MD_ECHO_TIME;
  } else {
    fx_tm = enc->getValue();
    enc->setValue(fx_dc);
    enc->effect = MD_FX_REV;
    enc->fxparam = MD_REV_DEC;
  }
}

void GridPage::toggle_fx2() {
  dispeffect = 1;

  GridEncoder *enc = (GridEncoder *)encoders[3];
  if (enc->effect == MD_FX_REV) {
    fx_lv = enc->getValue();
    enc->setValue(fx_fb);
    enc->effect = MD_FX_ECHO;
    enc->fxparam = MD_ECHO_FB;
  }

  else {
    fx_fb = enc->getValue();
    enc->setValue(fx_lv);
    enc->effect = MD_FX_REV;
    enc->fxparam = MD_REV_LEV;
  }
}

void GridPage::display_counters() {
#ifdef OLED_DISPLAY
  uint8_t y_offset = 8;
  uint8_t x_offset = 20;

  char val[4];
  // val[0] = (MidiClock.bar_counter / 100) + '0';
  val[0] = (MidiClock.bar_counter % 100) / 10 + '0';
  val[1] = (MidiClock.bar_counter % 10) + '0';
  val[2] = '\0';
  if (val[0] == '0') {
    val[0] = (char)0x60;
    //  if (val[1] == '0') {
    //    val[1] = (char)0x60;
    // }
  }

  oled_display.setFont(&TomThumb);
  oled_display.setCursor(24, y_offset);
  oled_display.print(val);

  oled_display.print(":");
  oled_display.print(MidiClock.beat_counter);

  if ((mcl_cfg.chain_mode > 0) &&
      (mcl_actions.next_transition != (uint16_t)-1) &&
      (MidiClock.bar_counter <= mcl_actions.nearest_bar)) {
    // val[0] = (mcl_actions.nearest_bar / 100) + '0';
    val[0] = (mcl_actions.nearest_bar % 100) / 10 + '0';
    val[1] = (mcl_actions.nearest_bar % 10) + '0';

    if (val[0] == '0') {
      val[0] = (char)0x60;
      if (val[1] == '0') {
        val[1] = (char)0x60;
      }
    }
    oled_display.setCursor(24, y_offset + 8);
    oled_display.print(val);
    oled_display.print(":");
    oled_display.print(mcl_actions.nearest_beat);
  }
#endif
}

void GridPage::display_grid_info() {
#ifdef OLED_DISPLAY
  uint8_t x_offset = 43;
  uint8_t y_offset = 8;

  oled_display.setFont(&Elektrothic);
  oled_display.setCursor(0, 10);
  oled_display.print(round(MidiClock.get_tempo()));

  display_counters();
  oled_display.setFont(&TomThumb);
  //  oled_display.print(":");
  // oled_display.print(MidiClock.step_counter);

  oled_display.setCursor(22, y_offset + 1 * 8);

  uint8_t tri_x = 10, tri_y = 12;
  if (MidiClock.state == 2) {

    oled_display.drawLine(tri_x, tri_y, tri_x, tri_y + 4, WHITE);
    oled_display.fillTriangle(tri_x + 1, tri_y, tri_x + 3, tri_y + 2, tri_x + 1,
                              tri_y + 4, WHITE);
  }
  if (MidiClock.state == 0) {
    oled_display.fillRect(tri_x - 1, tri_y, 2, 5, WHITE);
    oled_display.fillRect(tri_x + 2, tri_y, 2, 5, WHITE);
  }

  oled_display.setCursor(0, y_offset + 1 + 1 * 8);
  char dev[3] = "  ";

  MidiUart.device.get_name(dev);
  dev[2] = '\0';
  oled_display.print(dev);

  oled_display.setCursor(0, y_offset + 3 * 8);
  char dev2[3] = "  ";
  MidiUart2.device.get_name(dev2);
  dev2[2] = '\0';
  oled_display.print(dev2);

  oled_display.setCursor(16, y_offset + (MAX_VISIBLE_ROWS - 1) * 8);

  char val[4];
  val[0] = (encoders[0]->cur % 100) / 10 + '0';
  val[1] = (encoders[0]->cur % 10) + '0';
  val[2] = '\0';
  oled_display.print(val);
  oled_display.print(" ");
  val[0] = encoders[1]->cur / 100 + '0';
  val[1] = (encoders[1]->cur % 100) / 10 + '0';
  val[2] = (encoders[1]->cur % 10) + '0';
  val[3] = '\0';
  oled_display.print(val);

  oled_display.setCursor(1, y_offset + 2 * 8);
  oled_display.fillRect(oled_display.getCursorX() - 1,
                        oled_display.getCursorY() - 6, 37, 7, WHITE);

  oled_display.setTextColor(BLACK, WHITE);
  if (row_headers[cur_row].active) {
    char rowname[10];
    m_strncpy(rowname, row_headers[cur_row].name, 9);
    rowname[9] = '\0';

    oled_display.print(rowname);
  }

  oled_display.setTextColor(WHITE, BLACK);
#endif
}

void GridPage::display_grid() {
#ifdef OLED_DISPLAY
  uint8_t x_offset = 43;
  uint8_t y_offset = 8;

  oled_display.setFont(&TomThumb);

  classic_display = false;
  //   if (MidiClock.state == 1) {
  //  oled_display.print('>');
  //  }
  char str[3];
  PGM_P tmp;
  encoders[1]->handler = NULL;
  //  oled_display.setFont(&Org_01);

  oled_display.setCursor(x_offset - 6, y_offset + cur_row * 8);
  oled_display.print(">");

  for (uint8_t y = 0; y < MAX_VISIBLE_ROWS; y++) {

    auto cur_posx = x_offset;
    auto cur_posy = y_offset + y * 8;
    for (uint8_t x = 0; x < MAX_VISIBLE_COLS; x++) {
      oled_display.setCursor(cur_posx, cur_posy);

      auto track_idx = x + getCol() - cur_col;
      auto row_idx = y + getRow() - cur_row;
      uint8_t track_type = row_headers[y].track_type[track_idx];
      uint8_t model = row_headers[y].model[track_idx];

      bool blink = false;
      auto active_cue_color = WHITE;

      //  Set cell label
      switch (track_type) {
      case MD_TRACK_TYPE:
        tmp = getMachineNameShort(model, 2);
        m_strncpy_p(str, tmp, 3);
        break;
      case A4_TRACK_TYPE:
        str[0] = 'A';
        str[1] = (x + getCol() - cur_col - 16) + '0';
        break;
      case EXT_TRACK_TYPE:
        str[0] = 'M';
        str[1] = (x + getCol() - cur_col - 16) + '0';
        break;
      }

      //  Highlight the current cursor position + slot menu apply range
      if (x >= cur_col && x < cur_col + max(1, slot_apply) && y == cur_row) {
        oled_display.fillRect(cur_posx - 1, cur_posy - 6, 9, 7, WHITE);
        oled_display.setTextColor(BLACK, WHITE);
        active_cue_color = BLACK;
      } else {
        oled_display.setTextColor(WHITE, BLACK);
      }

      if ((MidiClock.step_counter == 2 || MidiClock.step_counter == 3) &&
          MidiClock.state == 2 && row_idx == active_slots[track_idx]) {
        // blink, don't print
        blink = true;
      } else if (model == 0) {
        oled_display.print("--");
      } else {
        oled_display.print(str);
      }

      if (row_idx == active_slots[track_idx] && !blink) {
        // a gentle visual cue for active slots
        oled_display.drawPixel(cur_posx - 1, cur_posy - 6, active_cue_color);
      }

      // tomThumb is 4x6
      if (track_idx % 4 == 3) {
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
  if ((getCol() - cur_col) % 4 == 0) {
    mcl_gui.draw_vertical_dashline(x_offset - 2, 3);
  }
#endif
}
void GridPage::display_slot_menu() {
  uint8_t x_offset = 43;
  uint8_t y_offset = 8;
  grid_slot_page.draw_menu(1, y_offset, 39);
  // grid_slot_page.draw_scrollbar(36);
}

void GridPage::display_oled() {
#ifdef OLED_DISPLAY
  uint8_t x_offset = 43;
  uint8_t y_offset = 8;

  oled_display.clearDisplay();
  oled_display.setTextWrap(false);
  oled_display.setFont(&TomThumb);
  oled_display.setTextColor(WHITE, BLACK);
  if (!show_slot_menu) {
    display_grid_info();
  } else {
    display_slot_menu();
  }
  display_grid();

  oled_display.display();
#endif
}

void GridPage::display() {

  tick_frames();
#ifdef OLED_DISPLAY
  display_oled();
  return;
#endif;

  // Rendering code for HD44780 below
  char str[3] = "  ";
  char str2[3] = "  ";
  PGM_P tmp;
  uint8_t y = 0;
  for (uint8_t x = 0; x < MAX_VISIBLE_COLS; x++) {
    uint8_t track_type = row_headers[y].track_type[x + encoders[0]->cur];
    uint8_t model = row_headers[y].model[x + encoders[0]->cur];
    if (((MidiClock.step_counter == 1) && (MidiClock.state == 2)) &&
        ((y + getRow() - cur_row) == active_slots[x + getCol() - cur_col])) {

    } else {
      str[0] = '-';
      str[1] = '-';
      str2[0] = '-';
      str2[1] = '-';

      if (track_type == MD_TRACK_TYPE) {
        tmp = getMachineNameShort(model, 1);
        m_strncpy_p(str, tmp, 3);
        tmp = getMachineNameShort(model, 2);
        m_strncpy_p(str2, tmp, 3);
      }
      if (track_type == A4_TRACK_TYPE) {
        str[0] = 'A';
        str[1] = '4';
        str2[0] = 'T';
        str2[1] = model + '0';
      }
      if (track_type == EXT_TRACK_TYPE) {
        str[0] = 'M';
        str[1] = 'I';
        str2[0] = 'T';
        str2[1] = model + '0';
      }
    }
    GUI.setLine(GUI.LINE1);
    GUI.put_string_at(x * 3, str);
    GUI.setLine(GUI.LINE2);
    GUI.put_string_at(x * 3, str2);
    displayScroll(x);
  }

  GUI.setLine(GUI.LINE1);
  /*Displays the kit name of the left most Grid on the first line at position
   * 12*/
  if (display_name == 1) {
    GUI.put_string_at(0, "                ");

    if (row_headers[cur_row].active) {
      GUI.put_string_at(0, row_headers[0].name);
    }
    GUI.setLine(GUI.LINE2);

    GUI.put_string_at(0, "                ");
    // temptrack.patternOrigPosition;
    char str[5];
  }
  // if (gridio_encoders[1]->getValue() < 8) {
  // if (temptrack.active != EMPTY_TRACK_TYPE) {
  //   MD.getPatternName(temptrack.patternOrigPosition , str);
  // }
  // }
  GUI.setLine(GUI.LINE2);

  /*Displays the value of the current Row on the screen.*/
  GUI.put_value_at2(12, (encoders[0]->getValue()));
  /*Displays the value of the current Column on the screen.*/
  GUI.put_value_at2(14, (encoders[1]->getValue()));
}

void GridPage::prepare() {
  MD.getCurrentTrack(CALLBACK_TIMEOUT);
  MD.currentKit = MD.getCurrentKit(CALLBACK_TIMEOUT);
  if ((mcl_cfg.auto_save == 1)) {
    MD.saveCurrentKit(MD.currentKit);
    MD.getBlockingKit(MD.currentKit);
    if (MidiClock.state == 2) {
      for (uint8_t n = 0; n < NUM_MD_TRACKS; n++) {
        mcl_seq.md_tracks[n].update_kit_params();
      }
    }
  }
}

void rename_row() {
  char *my_title = "Row Name:";
  if (mcl_gui.wait_for_input(grid_page.row_headers[grid_page.cur_row].name,
                             my_title, 8)) {
    grid_page.row_headers[grid_page.cur_row].write(grid_page.encoders[1]->cur);
    proj.file.sync();
  }
}

void apply_slot_changes_cb() { grid_page.apply_slot_changes(); }

void GridPage::apply_slot_changes() {
  show_slot_menu = false;
  encoders[0] = &param1;
  encoders[1] = &param2;
  uint8_t count;
  void (*row_func)() =
      grid_slot_page.menu.get_row_function(grid_slot_page.encoders[1]->cur);
  if (row_func != NULL) {
    (*row_func)();
    return;
  }
  if (slot_apply == 0) {
    count = 1;
  } else {
    count = slot_apply;
  }
  if (slot_copy == 1) {
    slot_buffer_mask = 0;
    slot_buffer_row = getRow();
  }

  else if (slot_paste == 1) {
    uint8_t first = 255;
    uint8_t n = 0;
    uint8_t count = 0;
    for (n = 0; n < GRID_WIDTH; n++) {
      if (IS_BIT_SET32(slot_buffer_mask, n)) {
        if (first == 255) {
          first = n;
        }
        count++;
      }
    }
    bool destination_same = false;
    if (count == 1) {
      destination_same = true;
    }
    for (n = 0; n + first < GRID_WIDTH && getCol() + n < GRID_WIDTH; n++) {
      if (IS_BIT_SET32(slot_buffer_mask, n + first)) {
        grid.copy_slot(n + first, slot_buffer_row, getCol() + n, getRow(),
                       destination_same);
      }
    }
    row_headers[cur_row].write(getRow());
  }
  if (slot_paste != 1) {
    MDTrack md_track;
    MDSeqTrack md_seq_track;
    for (uint8_t track = 0; track < count && track + getCol() < NUM_TRACKS;
         track++) {
      if (slot_clear == 1) {
        // Delete slot(s)
        grid.clear_slot(track + getCol(), getRow());
        row_headers[cur_row].update_model(track + getCol(), EMPTY_TRACK_TYPE,
                                          DEVICE_NULL);
        if (row_headers[cur_row].is_empty()) {
          char *str_tmp = "\0";
          strcpy(row_headers[cur_row].name, str_tmp);
          row_headers[cur_row].write(getRow());
        }
        reload_slot_models = true;
        proj.file.sync();
      } else if (slot_copy == 1) {
        // Mark slot for copy
        SET_BIT32(slot_buffer_mask, track + getCol());
      } else {
        // Save slot chain data
        slot.active = row_headers[cur_row].track_type[track + getCol()];
        //  if (slot.active != EMPTY_TRACK_TYPE) {
        slot.store_track_in_grid(track + getCol(), getRow());
        proj.file.sync();
      }
      /*
      Merge sequencer data. Deprecate, now controlled in save page.

      if ((merge_md > 0) && (slot.active != EMPTY_TRACK_TYPE)) {
        md_track.load_track_from_grid(track + getCol(), getRow());

        memcpy(&(md_seq_track), &(md_track.seq_data), sizeof(MDSeqTrackData));
        md_seq_track.merge_from_md(&md_track);
        md_track.clear_track();
        memcpy(&(md_track.seq_data), &(md_seq_track), sizeof(MDSeqTrackData));
        md_track.store_track_in_grid(track + getCol(), getRow());
      }
      */
    }
  }
  if ((slot_clear == 1) || (slot_paste == 1)) {
    load_slot_models();
  }

  proj.file.sync();
  slot_apply = 0;
  slot_clear = 0;
  slot_copy = 0;
  slot_paste = 0;
}

bool GridPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {

    return true;
  }
  // TRACK READ PAGE

  if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
    grid_save_page.isSetup = false;
    GUI.setPage(&grid_save_page);

    return true;
  }

  // TRACK WRITE PAGE

  if (EVENT_RELEASED(event, Buttons.BUTTON4)) {
    grid_write_page.isSetup = false;
    GUI.setPage(&grid_write_page);

    return true;
  }

#ifdef OLED_DISPLAY
  if (EVENT_PRESSED(event, Buttons.BUTTON3)) {

    show_slot_menu = true;
    DEBUG_DUMP(getCol());
    DEBUG_DUMP(getRow());
    slot.load_track_from_grid(getCol(), getRow());
    DEBUG_PRINTLN("what's in the slot");
    DEBUG_DUMP(slot.chain.loops);
    DEBUG_DUMP(slot.chain.row);
    /*
          if (slot.active == EMPTY_TRACK_TYPE) {
            DEBUG_PRINTLN("empty track");
            slot.chain.loops = 1;
            if (getRow() > 128) {
              slot.chain.row = 0;
            } else {
              slot.chain.row = getRow() + 1;
            }
          }*/
    encoders[0] = &grid_slot_param1;
    encoders[1] = &grid_slot_param2;
    grid_slot_page.init();
    slot_apply = 0;
    merge_md = 0;
    return true;
  }
  if (EVENT_RELEASED(event, Buttons.BUTTON3)) {
    apply_slot_changes();
    return true;
  }
#else
  if (EVENT_RELEASED(event, Buttons.BUTTON3)) {
    display_name = 0;
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
    display_name = 1;
    return;
  }
  if (BUTTON_DOWN(Buttons.BUTTON3)) {
    if (EVENT_PRESSED(event, Buttons.ENCODER1)) {
      slot.load_track_from_grid(getCol(), getRow());
      display_name = 0;
      slot_apply = 0;
      GUI.pushPage(&grid_slot_page);
    }
    if (EVENT_PRESSED(event, Buttons.ENCODER2)) {
      slot.load_track_from_grid(getCol() + 1, getRow());
      display_name = 0;
      slot_apply = 0;
      GUI.pushPage(&grid_slot_page);
    }

    if (EVENT_PRESSED(event, Buttons.ENCODER3)) {
      slot.load_track_from_grid(getCol() + 2, getRow());
      display_name = 0;
      slot_apply = 0;
      GUI.pushPage(&grid_slot_page);
    }

    if (EVENT_PRESSED(event, Buttons.ENCODER4)) {
      slot.load_track_from_grid(getCol() + 3, getRow());
      display_name = 0;
      slot_apply = 0;
      GUI.pushPage(&grid_slot_page);
    }
    return true;
  }
#endif
  if (EVENT_PRESSED(event, Buttons.ENCODER1)) {
    seq_step_page.isSetup = false;
    prepare();
    GUI.setPage(&seq_step_page);

    return true;
  }
  if (EVENT_PRESSED(event, Buttons.ENCODER2)) {
    seq_rtrk_page.isSetup = false;
    prepare();
    GUI.setPage(&seq_rtrk_page);

    return true;
  }
  if (EVENT_PRESSED(event, Buttons.ENCODER3)) {
    seq_param_page[0].isSetup = false;
    prepare();
    GUI.setPage(&seq_param_page[0]);

    return true;
  }
  if (EVENT_PRESSED(event, Buttons.ENCODER4)) {
    seq_ptc_page.isSetup = false;
    prepare();
    GUI.setPage(&seq_ptc_page);

    return true;
  }

  if ((EVENT_PRESSED(event, Buttons.BUTTON1) && BUTTON_DOWN(Buttons.BUTTON4)) ||
      (EVENT_PRESSED(event, Buttons.BUTTON4) && BUTTON_DOWN(Buttons.BUTTON1))) {

    system_page.isSetup = false;
    GUI.pushPage(&system_page);

    return true;
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
    prepare();
    GUI.setPage(&page_select_page);
    return true;
  }

  return false;
}
