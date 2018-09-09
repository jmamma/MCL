#include "GUI.h"
#include "GridPage.h"
#include "GridPages.h"
#include "MCL.h"

void GridPage::init() {
  reload_slot_models = false;
  md_exploit.off();
#ifdef OLED_DISPLAY
  oled_display.clearDisplay();
#endif
}

void GridPage::setup() {
  uint8_t charmap[8] = {10, 10, 10, 10, 10, 10, 10, 00};
  LCD.createChar(1, charmap);
  frames_startclock = slowclock;
  encoders[1]->handler = encoder_param2_handle;
  encoders[2]->handler = encoder_fx_handle;
  ((GridEncoder *)encoders[2])->effect = MD_FX_ECHO;
  ((GridEncoder *)encoders[2])->fxparam = MD_ECHO_TIME;
  encoders[3]->handler = encoder_fx_handle;
  ((GridEncoder *)encoders[3])->effect = MD_FX_ECHO;
  ((GridEncoder *)encoders[3])->fxparam = MD_ECHO_FB;
  encoders[0]->cur = mcl_cfg.col;
  encoders[1]->cur = mcl_cfg.row;
  cur_col = mcl_cfg.cur_col;
  cur_row = mcl_cfg.cur_row;
}
void GridPage::cleanup() {
#ifdef OLED_DISPLAY
   oled_display.setFont();
#endif
}
void GridPage::loop() { midi_active_peering.check(); }
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

void encoder_param2_handle(Encoder *enc) {

  if (enc->hasChanged()) {
    grid_page.grid_lastclock = slowclock;

    grid_page.reload_slot_models = false;
  }
}

A4Track track_bufx;

uint8_t GridPage::getRow() { return encoders[1]->cur; }

uint8_t GridPage::getCol() { return encoders[0]->cur; }
void GridPage::shift_slot_models(uint8_t count, bool direction) {
  //  count = 1;
  /*
          if (direction) {
      for (int8_t a = 0; a < MAX_VISIBLE_ROWS - count; a++) {
        for (uint8_t i = 0; i < 22; i++) {
          grid_models[a][i] = grid_models[a + count][i];
        }
      }
    } else {
      for (int8_t a = MAX_VISIBLE_ROWS - 1 - count; a >= 0; a--) {
        for (uint8_t i = 0; i < 22; i++) {
          grid_models[a + count][i] = grid_models[a][i];
        }
      }
    }
  */
}
void GridPage::load_slot_models() {

  for (uint8_t n = 0; n < MAX_VISIBLE_ROWS; n++) {
    row_headers[n].read(getRow() - cur_row + n);
  }

  /*  for (uint8_t a = 0; a < MAX_VISIBLE_ROWS; a++) {
      for (uint8_t i = 0; i < 22; i++) {
        grid_models[a][i] =
            grid.get_slot_model(i, a + encoders[1]->cur - cur_row, true,
    (A4Track *)&track_bufx); if ((i == 0) && (a == encoders[1]->cur)) { if
    (temptrack.active != EMPTY_TRACK_TYPE) { for (uint8_t c = 0; c < 16; c++) {
              currentkitName[c] = temptrack.kitName[c];
            }
          } else {
            for (uint8_t c = 0; c < 16; c++) {
              currentkitName[c] = ' ';
            }
          }
        }
      }
    } */
}
void GridPage::load_slot_model_row(uint8_t y, uint8_t row) {
  /*
          DEBUG_PRINT_FN(x);

    DEBUG_PRINT("Row: ");
    DEBUG_PRINTLN(row);
    DEBUG_PRINTLN(y);
    for (uint8_t i = 0; i < 22; i++) {
      grid_models[y][i] =
          grid.get_slot_model(i, row, true, (A4Track *)&track_bufx);
      DEBUG_PRINTLN(grid_models[y][i]);
    }
  */
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
    //DEBUG_PRINTLN((float)frames * (float)4);
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

void GridPage::displaySlot(uint8_t i) {
  const char *str;
  /*Calculate the position of the Grid to be displayed based on the Current Row,
   * Column and Encoder*/
  // int value = displayx + (displayy * 16) + i;

  GUI.setLine(GUI.LINE1);

  char a4_name2[3] = "TK";

  char strn[3] = "--";
  // A4Track track_buf;
  uint8_t model =
      grid_page.row_headers[cur_row].track_type[encoders[0]->cur + i];

  // getGridModel(encoders[1]->getValue() + i, encoders[2]->getValue(), true,
  // (A4Track*) &track_buf);

  /*Retrieve the first 2 characters of Maching Name associated with the Track at
   * the current Grid. First obtain the Model object from the Track object, then
   * convert the MachineType into a string*/
  if (encoders[0]->cur + i < 16) {
    str = getMachineNameShort(model, 1);

    if (str == NULL) {
      GUI.put_string_at((0 + (i * 3)), strn);
    } else {
      GUI.put_p_string_at((0 + (i * 3)), str);
    }
  } else {
    if (model == EMPTY_TRACK_TYPE) {
      GUI.put_string_at((0 + (i * 3)), strn);
    } else {
      if (model == A4_TRACK_TYPE) {
        char a4_name1[3] = "A4";
        GUI.put_string_at((0 + (i * 3)), a4_name1);
      }
      if (model == EXT_TRACK_TYPE) {
        char ex_name1[3] = "EX";
        GUI.put_string_at((0 + (i * 3)), ex_name1);
      }
    }
  }

  GUI.setLine(GUI.LINE2);
  str = NULL;

  if (encoders[0]->cur + i < 16) {

    str = getMachineNameShort(model, 2);

    if (str == NULL) {
      GUI.put_string_at((0 + (i * 3)), strn);
    } else {
      GUI.put_p_string_at((0 + (i * 3)), str);
    }
  }

  else {
    if (model == EMPTY_TRACK_TYPE) {
      GUI.put_string_at((0 + (i * 3)), strn);
    } else {
      GUI.put_string_at((0 + (i * 3)), a4_name2);
      GUI.put_value_at1(1 + (i * 3), encoders[0]->getValue() + i - 15);
    }
  }
  redisplay = false;
}

void GridPage::display_slot_oled(uint8_t x, uint8_t y, char *strn) {
  const char *str;
  uint8_t model;

  //  if (encoders[0]->cur + i < 16) {

  str = getMachineNameShort(model, 2);

  if (str == NULL) {
    GUI.put_string_at(x * 3, strn);
  } else {
    GUI.put_p_string_at((0 + (x * 3)), str);
  }
  // }

  //  else {
  if (model == EMPTY_TRACK_TYPE) {
    GUI.put_string_at((0 + (x * 3)), strn);
  } else {
    //      GUI.put_string_at((0 + (x * 3)), a4_name2);
    GUI.put_value_at1(1 + (x * 3), encoders[0]->getValue() + x - 15);
  }
  // }
  redisplay = false;
}

void GridPage::display_oled() {
  uint8_t x_offset = 43;
  uint8_t y_offset = 8;

  classic_display = false;
  oled_display.clearDisplay();
  oled_display.setFont(&Elektrothic);
  oled_display.setCursor(0, 10);
  oled_display.print(round(MidiClock.tempo));

  oled_display.setCursor(22, y_offset + 1 * 8);

  uint8_t tri_x = 9, tri_y = 12;
  if (MidiClock.state == 2) {

    oled_display.drawLine(tri_x, tri_y, tri_x, tri_y + 4, WHITE);
    oled_display.fillTriangle(tri_x + 1, tri_y, tri_x + 3, tri_y + 2, tri_x + 1,
                              tri_y + 4, WHITE);
  }
  if (MidiClock.state == 0) {
    oled_display.fillRect(tri_x + 6, tri_y + 1, 2, 4, WHITE);
    oled_display.fillRect(tri_x + 9, tri_y + 1, 2, 4, WHITE);
  }

  oled_display.setFont(&TomThumb);

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

  //   if (MidiClock.state == 1) {
  //  oled_display.print('>');
  //  }
  char str[3];
  PGM_P tmp;
  encoders[1]->handler = NULL;
  //  oled_display.setFont(&Org_01);
  int8_t diff, new_val;
  if (encoders[0]->hasChanged()) {
    DEBUG_PRINTLN("yep");
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
    /*
    if (abs(diff) == 1) {
    if ((cur_row == 0) && (diff < 0)) {
      shift_slot_models(abs(diff), false);
      DEBUG_PRINTLN("scroll down");
      DEBUG_PRINTLN(diff);
     // for (uint8_t a = 0; a < abs(diff); a++) {
      load_slot_model_row(0, encoders[1]->cur);
     // }
    }
    if ((cur_row == MAX_VISIBLE_ROWS - 1) && (diff > 0)) {
      DEBUG_PRINTLN("scroll up");
      DEBUG_PRINTLN(diff);
      */
    // load_slot_model_row(MAX_VISIBLE_ROWS - 1, encoders[1]->cur);

    //  for (uint8_t a = 0; a < abs(diff); a++) {
    // load_slot_model_row(MAX_VISIBLE_ROWS - 1 - a, encoders[1]->cur - a);
    //  }

    cur_row = new_val;
    load_slot_models();
  }
  oled_display.setTextWrap(false);
  for (uint8_t y = 0; y < MAX_VISIBLE_ROWS; y++) {
    if ((y == cur_row)) {
      oled_display.setCursor(x_offset - 5, y_offset + y * 8);
      oled_display.print(">");
    }

    oled_display.setCursor(x_offset, y_offset + y * 8);
    for (uint8_t x = 0; x < 8; x++) {
      uint8_t device = row_headers[y].device[x + encoders[0]->cur - cur_col];
      uint8_t model = row_headers[y].track_type[x + encoders[0]->cur - cur_col];
      if (device == DEVICE_MD) {
        tmp = getMachineNameShort(model, 2);
        m_strncpy_p(str, tmp, 3);
      }
      if (device == DEVICE_A4) {
        str[0] = 'A';
        str[1] = model + '0';
      }
      if (device == DEVICE_MIDI) {
        str[0] = 'M';
        str[1] = model + '0';
      }
      if ((x == cur_col) && (y == cur_row)) {
        oled_display.fillRect(oled_display.getCursorX() - 1,
                              oled_display.getCursorY() - 6, 9, 7, WHITE);
        oled_display.setTextColor(BLACK, WHITE);
      } else {
        oled_display.setTextColor(WHITE, BLACK);
      }
      if (model == 0) {
        oled_display.print("--");
      } else {
        oled_display.print(str);
      }
      if ((x + 1 + encoders[0]->cur - cur_col) % 4 == 0) {
        oled_display.setTextColor(WHITE, BLACK);
        oled_display.print(" | ");
      } else {
        oled_display.print(" ");
      }

      if ((x == encoders[0]->cur) && (y == encoders[1]->cur)) {
        //   oled_display.drawRect(xpos_old, y_offset + 2 + (y - 1) * 8, 8, 6,
        //                       WHITE);
      }
    }
  }

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

  oled_display.display();
}
void GridPage::display() {
  if (!reload_slot_models) {
    load_slot_models();
    reload_slot_models = true;
  }
  tick_frames();
  display_oled();
  //        for (uint8_t i = 0; i < 4; i++) {
  // GUI.put_value_at2(i * 4, encoders[i]->cur);
  //           }
  //         return;
  return;
  // GUI.put_value16_at(0, MidiClock.div192th_counter);
  //  GUI.put_value16_at(5, MidiClock.div96th_counter);
  //   GUI.put_value_at(12, (uint8_t)MidiClock.div192th_time);
  // if (MidiClock.mod12_counter > 10) { GUI.put_value16_at(0,
  // MidiClock.mod12_counter); } GUI.put_value16_at(5, MidiClock.mod6_counter);

  //  return;

  // grid.row_name_offset += (float)1 / frames_fps * 1.5;

  if (BUTTON_DOWN(Buttons.BUTTON3) && (encoders[2]->hasChanged())) {
    toggle_fx1();
  }

  if (BUTTON_DOWN(Buttons.BUTTON3) && (encoders[3]->hasChanged())) {
    toggle_fx2();
  }
  uint8_t display_name = 0;
  if (slowclock < grid_lastclock) {
    grid_lastclock = slowclock + GUI_NAME_TIMEOUT;
  }

  if (clock_diff(grid_lastclock, slowclock) < GUI_NAME_TIMEOUT) {
    DEBUG_PRINTLN(grid_lastclock);
    DEBUG_PRINTLN(slowclock);
    display_name = 1;
    if (clock_diff(mcl_cfg.cfg_save_lastclock, slowclock) > GUI_NAME_TIMEOUT) {
      mcl_cfg.cur_col = cur_col;
      mcl_cfg.cur_row = cur_row;

      mcl_cfg.col = encoders[0]->cur;
      mcl_cfg.row = encoders[1]->cur;

      mcl_cfg.tempo = MidiClock.tempo;
      mcl_cfg.write_cfg();
    }
  } else {

    /*For each of the 4 encoder objects, ie 4 Grids to be displayed on screen*/
    for (uint8_t i = 0; i < 4; i++) {

      /*Display the encoder, ie the Grid i*/
      displaySlot(i);
      //      GUI.put_value_at2(2 + i * 4,encoders[i]->getValue());
      /*Display the scroll animation. (Scroll animation draws a || at every 4
       * Grids in a row, making it easier to separate and visualise the track
       * Grids)*/
      displayScroll(i);
      /*value = the grid position of the left most Grid (displayed on screen).*/
    }
  }
  // int value = encoders[0]->getValue() + (encoders[1]->getValue() * 16);

  /*If the effect encoders have been changed, then set a switch to indicate that
   * the effects values should be displayed*/

  if (encoders[2]->hasChanged() || encoders[3]->hasChanged()) {
    dispeffect = 1;
  }

  /*If the grid encoders have been changed, set a switch to indicate that the
   * row/col values should be displayed*/
  if (encoders[0]->hasChanged() || encoders[1]->hasChanged()) {
    dispeffect = 0;
  }

  if (dispeffect == 1) {
    GUI.setLine(GUI.LINE1);
    /*Displays the kit name of the left most Grid on the first line at position
     * 12*/
    if (((GridEncoder *)encoders[2])->effect == MD_FX_ECHO) {
      GUI.put_string_at(12, "TM");
    } else {
      GUI.put_string_at(12, "DC");
    }

    if (((GridEncoder *)encoders[3])->effect == MD_FX_ECHO) {
      GUI.put_string_at(14, "FB");
    } else {
      GUI.put_string_at(14, "LV");
    }

    GUI.setLine(GUI.LINE2);
    /*Displays the value of the current Row on the screen.*/
    GUI.put_value_at2(12, (encoders[2]->getValue()));
    GUI.put_value_at2(14, (encoders[3]->getValue()));

    // mdEnc1->dispnow = 0;
    //  mdEnc2->dispnow = 0;

  } else {
    GUI.setLine(GUI.LINE1);
    /*Displays the kit name of the left most Grid on the first line at position
     * 12*/
    if (display_name == 1) {
      GUI.put_string_at(0, "                ");

      GUI.put_string_at(0, row_headers[cur_row].name);
      GUI.setLine(GUI.LINE2);

      GUI.put_string_at(0, "                ");
      // temptrack.patternOrigPosition;
      char str[5];

      // if (gridio_encoders[1]->getValue() < 8) {
      // if (temptrack.active != EMPTY_TRACK_TYPE) {
      //   MD.getPatternName(temptrack.patternOrigPosition , str);
      // }
      // }
    } else {

      //      GUI.put_string_at(12, grid.get_slot_kit(encoders[0]->getValue(),
      //                                            encoders[1]->getValue(),
      //                                            false,
      //                                          true));
    }
    GUI.setLine(GUI.LINE2);

    /*Displays the value of the current Row on the screen.*/
    GUI.put_value_at2(12, (encoders[0]->getValue()));
    /*Displays the value of the current Column on the screen.*/
    GUI.put_value_at2(14, (encoders[1]->getValue()));
  }
}

void GridPage::prepare() {
  MD.getCurrentTrack(CALLBACK_TIMEOUT);
  MD.currentKit = MD.getCurrentKit(CALLBACK_TIMEOUT);
  if ((mcl_cfg.auto_save == 1) && (MidiClock.state != 2)) {
    MD.saveCurrentKit(MD.currentKit);
  }
  MD.getBlockingKit(MD.currentKit);
}

bool GridPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {

    return true;
  }

  if (BUTTON_RELEASED(Buttons.BUTTON1) && BUTTON_DOWN(Buttons.BUTTON3)) {
    grid.clear_row(grid_page.encoders[1]->getValue());
    reload_slot_models = false;
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

  if (BUTTON_DOWN(Buttons.BUTTON2) && BUTTON_DOWN(Buttons.BUTTON4)) {
    setLed();
    int curtrack = MD.getCurrentTrack(CALLBACK_TIMEOUT);

    grid_page.encoders[0]->cur = curtrack;

    clearLed();
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
    mixer_page.isSetup = false;
    //    GUI.setPage(&mixer_page);
    //   draw_levels();
  }
  /*IF button1 and encoder buttons are pressed, store current track selected on
   * MD into the corresponding Grid*/

  //  if (BUTTON_PRESSED(Buttons.BUTTON3)) {
  //      MD.getBlockingGlobal(1);
  //          MD.global.baseChannel = 9;
  //        //global_new.baseChannel;
  //          for (int i=0; i < 16; i++) {
  //            MD.muteTrack(i,true);
  //          }
  //           setLevel(8,100);
  //  }

  if (BUTTON_PRESSED(Buttons.ENCODER1)) {
    seq_step_page.isSetup = false;
    prepare();
    GUI.setPage(&seq_step_page);

    return true;
  }
  if (BUTTON_PRESSED(Buttons.ENCODER2)) {
    seq_rtrk_page.isSetup = false;
    prepare();
    GUI.setPage(&seq_rtrk_page);

    return true;
  }
  if (BUTTON_PRESSED(Buttons.ENCODER3)) {
    seq_param_page[0].isSetup = false;
    prepare();
    GUI.setPage(&seq_param_page[0]);

    return true;
  }
  if (BUTTON_PRESSED(Buttons.ENCODER4)) {
    seq_ptc_page.isSetup = false;
    prepare();
    GUI.setPage(&seq_ptc_page);

    return true;
  }

  if ((EVENT_PRESSED(event, Buttons.BUTTON1) && BUTTON_DOWN(Buttons.BUTTON4)) ||
      (EVENT_PRESSED(event, Buttons.BUTTON4) && BUTTON_DOWN(Buttons.BUTTON1))) {

    system_page.isSetup = false;
    GUI.setPage(&system_page);

    return true;
  }
  if (BUTTON_PRESSED(Buttons.BUTTON2)) {
    prepare();
    GUI.setPage(&page_select_page);
    return true;
  }

  return false;
}
