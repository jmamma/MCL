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

void GridPage::load_slot_models() {

  DEBUG_PRINT_FN(x);

  DEBUG_PRINT("Row: ");
  DEBUG_PRINTLN(encoders[1]->getValue());
  for (uint8_t i = 0; i < 22; i++) {
    grid_models[i] = grid.get_slot_model(i, encoders[1]->getValue(), true,
                                         (A4Track *)&track_bufx);
    if (i == 0) {
      if (temptrack.active != EMPTY_TRACK_TYPE) {
        for (uint8_t c = 0; c < 16; c++) {
          currentkitName[c] = temptrack.kitName[c];
        }
      } else {
        for (uint8_t c = 0; c < 16; c++) {
          currentkitName[c] = ' ';
        }
      }
    }
  }
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
  uint8_t model = grid_page.grid_models[encoders[0]->cur + i];

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

void GridPage::display() {

  //        for (uint8_t i = 0; i < 4; i++) {
  // GUI.put_value_at2(i * 4, encoders[i]->cur);
  //           }
  //         return;
  tick_frames();
  // GUI.put_value16_at(0, MidiClock.div192th_counter);
  //  GUI.put_value16_at(5, MidiClock.div96th_counter);
  //   GUI.put_value_at(12, (uint8_t)MidiClock.div192th_time);
  // if (MidiClock.mod12_counter > 10) { GUI.put_value16_at(0,
  // MidiClock.mod12_counter); } GUI.put_value16_at(5, MidiClock.mod6_counter);

  //  return;

  grid.row_name_offset += (float)1 / frames_fps * 1.5;

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
  if (!reload_slot_models) {
    load_slot_models();
    reload_slot_models = true;
  }

  if (clock_diff(grid_lastclock, slowclock) < GUI_NAME_TIMEOUT) {
    DEBUG_PRINTLN(grid_lastclock);
    DEBUG_PRINTLN(slowclock);
    display_name = 1;
    if (clock_diff(mcl_cfg.cfg_save_lastclock, slowclock) > GUI_NAME_TIMEOUT) {
      mcl_cfg.cur_col = encoders[1]->cur;
      mcl_cfg.cur_row = encoders[2]->cur;
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

      GUI.put_string_at(0, currentkitName);
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

      GUI.put_string_at(12, grid.get_slot_kit(encoders[0]->getValue(),
                                              encoders[1]->getValue(), false,
                                              true));
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
  if ((mcl_cfg.auto_save == 1)) {
    MD.saveCurrentKit(MD.currentKit);
  }
  MD.getBlockingKit(MD.currentKit);

  if (MD.connected) {
    ((MCLEncoder *)encoders[1])->min = 0;
    grid_page.cur_col = last_md_track;
  }
  grid_page.cur_row = param2.getValue();
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
