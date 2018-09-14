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
void GridPage::loop() {
  midi_active_peering.check();
  int8_t diff, new_val;
#ifdef OLED_DISPLAY
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
    cur_row = new_val;
    load_slot_models();
    reload_slot_models = false;
    grid_lastclock = slowclock;
  }
#else
  cur_col = encoders[0]->cur;
  //cur_row = encoders[1]->cur;
  cur_row = 0;
  if (encoders[1]->hasChanged()) {
    grid_lastclock = slowclock;
    reload_slot_models = false;
  }
#endif

  if (!reload_slot_models) {
    load_slot_models();
    reload_slot_models = true;
  }

  if (BUTTON_DOWN(Buttons.BUTTON3) && (encoders[2]->hasChanged())) {
    toggle_fx1();
  }

  if (BUTTON_DOWN(Buttons.BUTTON3) && (encoders[3]->hasChanged())) {
    toggle_fx2();
  }
  if (slowclock < grid_lastclock) {
    grid_lastclock = slowclock + GUI_NAME_TIMEOUT;
  }

  if (clock_diff(grid_lastclock, slowclock) < GUI_NAME_TIMEOUT) {
    DEBUG_PRINTLN(grid_lastclock);
    DEBUG_PRINTLN(slowclock);
    display_name = 1;
    } else {
   if (display_name == 1) {
      mcl_cfg.cur_col = cur_col;
      mcl_cfg.cur_row = cur_row;

      mcl_cfg.col = encoders[0]->cur;
      mcl_cfg.row = encoders[1]->cur;

      mcl_cfg.tempo = MidiClock.tempo;
      DEBUG_PRINTLN("write cfg");
      mcl_cfg.write_cfg();
    }

  display_name = 0;
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

uint8_t GridPage::getRow() { return encoders[1]->cur; }

uint8_t GridPage::getCol() { return encoders[0]->cur; }

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
    // DEBUG_PRINTLN((float)frames * (float)4);
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
  oled_display.setTextWrap(false);
  for (uint8_t y = 0; y < MAX_VISIBLE_ROWS; y++) {
    if ((y == cur_row)) {
      oled_display.setCursor(x_offset - 5, y_offset + y * 8);
      oled_display.print(">");
    }

    oled_display.setCursor(x_offset, y_offset + y * 8);
    for (uint8_t x = 0; x < MAX_VISIBLE_COLS; x++) {
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

  tick_frames();
#ifdef OLED_DISPLAY
  display_oled();
  return;
#endif;

  // Rendering code for HD44780 below
  char str[3];
  char str2[3];
  PGM_P tmp;
  uint8_t y = 0;
  for (uint8_t x = 0; x < MAX_VISIBLE_COLS; x++) {
    uint8_t device = row_headers[y].device[x + encoders[0]->cur];
    uint8_t model = row_headers[y].track_type[x + encoders[0]->cur];
    str[0] = '-';
    str[1] = '-';
    str2[0] = '-';
    str2[1] = '-';

    if (device == DEVICE_MD) {
      tmp = getMachineNameShort(model, 1);
      m_strncpy_p(str, tmp, 3);
      tmp = getMachineNameShort(model, 2);
      m_strncpy_p(str2, tmp, 3);
    }
    if (device == DEVICE_A4) {
      str[0] = 'A';
      str[1] = '4';
      str2[0] = 'T';
      str2[1] = model + '0';
    }
    if (device == DEVICE_MIDI) {
      str[0] = 'M';
      str[1] = 'I';
      str2[0] = 'T';
      str2[1] = model + '0';
    }
    GUI.setLine(GUI.LINE1);
    GUI.put_string_at(x * 3, str);
    GUI.setLine(GUI.LINE2);
    GUI.put_string_at(x * 3, str2);
    displayScroll(x);
  }

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

      if (row_headers[cur_row].active) {
        GUI.put_string_at(0, row_headers[0].name);
      }
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
