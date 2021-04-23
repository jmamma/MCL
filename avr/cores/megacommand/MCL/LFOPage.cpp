#include "MCL_impl.h"
#include "ResourceManager.h"

#define LFO_TYPE 0
#define LFO_PARAM 1
#define INTERPOLATE
#define DIV_1_127 .0079

#define LFO_DESTINATION 1
#define LFO_SETTINGS 0

void LFOPage::setup() {
  //  lfo_track = &mcl_seq.lfo_tracks[0];

  lfo_track->params[0].update_offset();
  lfo_track->params[1].update_offset();
  lfo_track->wav_table_state[0] = false;
  lfo_track->wav_table_state[1] = false;
  DEBUG_PRINT_FN();
}

void LFOPage::init() {
  DEBUG_PRINT_FN();
#ifdef OLED_DISPLAY
  classic_display = false;
  oled_display.clearDisplay();
  oled_display.setFont();
#endif
  update_encoders();

  if (lfo_track->mode != LFO_MODE_FREE) {
    trig_interface.on();
  }
  // LFOPage not using base SeqPage init?
  R.Clear();
  R.use_machine_param_names();
}

void LFOPage::cleanup() {
  trig_interface.off();
#ifdef OLED_DISPLAY
  oled_display.clearDisplay();
#endif
}

void LFOPage::update_encoders() {
  if (page_mode == LFO_DESTINATION) {
    encoders[0]->cur = lfo_track->params[0].dest;
    ((MCLEncoder *)encoders[0])->max = NUM_MD_TRACKS + 4;
    encoders[1]->cur = lfo_track->params[0].param;
    ((MCLEncoder *)encoders[1])->max = 23;
    encoders[2]->cur = lfo_track->params[1].dest;
    ((MCLEncoder *)encoders[2])->max = NUM_MD_TRACKS + 4;
    encoders[3]->cur = lfo_track->params[1].param;
    ((MCLEncoder *)encoders[3])->max = 23;

    if (encoders[0]->cur > NUM_MD_TRACKS) {
      ((MCLEncoder *)encoders[1])->max = 7;
    } else {
      ((MCLEncoder *)encoders[1])->max = 23;
    }

    if (encoders[2]->cur > NUM_MD_TRACKS) {
      ((MCLEncoder *)encoders[3])->max = 7;
    } else {
      ((MCLEncoder *)encoders[3])->max = 23;
    }
  }
  if (page_mode == LFO_SETTINGS) {
    encoders[0]->cur = waveform;
    ((MCLEncoder *)encoders[0])->max = 5;

    encoders[1]->cur = lfo_track->speed;
    ((MCLEncoder *)encoders[1])->max = 127;

    encoders[2]->cur = lfo_track->params[0].depth;
    ((MCLEncoder *)encoders[2])->max = 127;

    encoders[3]->cur = lfo_track->params[1].depth;
    ((MCLEncoder *)encoders[3])->max = 127;
  }
  //  loop();

  for (uint8_t i = 0; i < GUI_NUM_ENCODERS; i++) {
    encoders[i]->old = encoders[i]->cur;
    ((LightPage *)this)->encoders_used_clock[i] =
        slowclock - SHOW_VALUE_TIMEOUT - 1;
  }
}

void LFOPage::loop() {
  if (page_mode == LFO_DESTINATION) {

    if (encoders[0]->hasChanged()) {
      USE_LOCK();
      SET_LOCK();
      lfo_track->params[0].reset_param_offset();
      lfo_track->params[0].dest = encoders[0]->cur;
      lfo_track->params[0].update_offset();
      CLEAR_LOCK();
      if (encoders[0]->cur > NUM_MD_TRACKS) {
        ((MCLEncoder *)encoders[1])->max = 7;
      } else {
        ((MCLEncoder *)encoders[1])->max = 23;
      }
    }
    if (encoders[1]->hasChanged()) {
      USE_LOCK();
      SET_LOCK();
      lfo_track->params[0].reset_param_offset();
      lfo_track->params[0].param = encoders[1]->cur;
      //  lfo_track->params[0].offset = lfo_track->params[0].get_param_offset(
      //     encoders[0]->cur, encoders[1]->cur);
      lfo_track->params[0].update_offset();
      CLEAR_LOCK();
    }

    if (encoders[2]->hasChanged()) {
      USE_LOCK();
      SET_LOCK();
      lfo_track->params[1].reset_param_offset();
      lfo_track->params[1].dest = encoders[2]->cur;
      lfo_track->params[1].update_offset();
      CLEAR_LOCK();
      if (encoders[2]->cur > NUM_MD_TRACKS) {
        ((MCLEncoder *)encoders[3])->max = 7;
      } else {
        ((MCLEncoder *)encoders[3])->max = 23;
      }
    }
    if (encoders[3]->hasChanged()) {
      USE_LOCK();
      SET_LOCK();
      lfo_track->params[1].reset_param_offset();
      lfo_track->params[1].param = encoders[3]->cur;
      //  lfo_track->params[1].offset = lfo_track->params[1].get_param_offset(
      //     encoders[2]->cur, encoders[3]->cur);
      lfo_track->params[1].update_offset();
      CLEAR_LOCK();
    }
  }
  // wav_tables need to be recalculated when depth or waveform changes.

  if (page_mode == LFO_SETTINGS) {
    if (encoders[0]->hasChanged()) {
      lfo_track->set_wav_type(encoders[0]->cur);
    }

    if (encoders[1]->hasChanged()) {
      lfo_track->set_speed(encoders[1]->cur);
    }

    if (encoders[2]->hasChanged()) {
      lfo_track->set_depth(0, encoders[2]->cur);
    }

    if (encoders[3]->hasChanged()) {
      lfo_track->set_depth(1, encoders[3]->cur);
    }
  }

  if (!lfo_track->wav_table_up_to_date(0)) {
    lfo_track->load_wav_table(0);
  }

  if (!lfo_track->wav_table_up_to_date(1)) {
    lfo_track->load_wav_table(1);
  }
}

void LFOPage::draw_param(uint8_t knob, uint8_t dest, uint8_t param) {

  char myName[4] = "-- ";

  const char* modelname = NULL;
  if (dest != 0) {
    if (dest < 17) {
      modelname = model_param_name(MD.kit.get_model(dest - 1), param);
    } else {
      modelname = fx_param_name(MD_FX_ECHO + dest - 17, param);
    }
    if (modelname != NULL) {
      strncpy(myName, modelname, 4);
    }
  }
#ifdef OLED_DISPLAY
  draw_knob(knob, "PAR", myName);
#else
  GUI.setLine(GUI.LINE1);

  GUI.put_string_at(knob * 4, myName);
  ;

#endif
}

void LFOPage::draw_dest(uint8_t knob, uint8_t value) {
  char K[4];
  switch (value) {
  case 0:
    strcpy(K, "--");
    break;
  case 17:
    strcpy(K, "ECH");
    break;
  case 18:
    strcpy(K, "REV");
    break;
  case 19:
    strcpy(K, "EQ");
    break;
  case 20:
    strcpy(K, "DYN");
    break;
  default:
    //  K[0] = 'T';
    itoa(value, K, 10);
    break;
  }
#ifdef OLED_DISPLAY
  draw_knob(knob, "DEST", K);
#else
  GUI.setLine(GUI.LINE1);
  GUI.put_string_at(knob * 4, K);
#endif
}

void LFOPage::display() {
  if (!classic_display) {
#ifdef OLED_DISPLAY
    oled_display.clearDisplay();
#endif
  }
#ifndef OLED_DISPLAY
  GUI.clearLines();
  GUI.setLine(GUI.LINE1);
  uint8_t x;

  GUI.put_string_at(0, "LFO");
  GUI.put_value_at1(4, page_mode ? 1 : 0);
  GUI.setLine(GUI.LINE2);
  /*
    if (mcl_cfg.ram_page_mode == 0) {
      GUI.put_string_at(0, "MON");
    } else {
      GUI.put_string_at(0, "LNK");
    }
  */
  if (page_mode == LFO_DESTINATION) {
    draw_dest(0, encoders[0]->cur);
    draw_param(1, encoders[0]->cur, encoders[1]->cur);
    draw_dest(2, encoders[2]->cur);
    draw_param(3, encoders[2]->cur, encoders[3]->cur);
  }
  if (page_mode == LFO_SETTINGS) {
    char K[4];
    switch (lfo_track->wav_type) {
    case SIN_WAV:
      strcpy(K, "SIN");
      break;
    case TRI_WAV:
      strcpy(K, "TRI");
      break;
    case IRAMP_WAV:
      strcpy(K, "IR");
      break;
    case RAMP_WAV:
      strcpy(K, "R");
      break;
    case EXP_WAV:
      strcpy(K, "EX");
      break;
    case IEXP_WAV:
      strcpy(K, "IEX");
      break;
    }
    GUI.setLine(GUI.LINE1);
    GUI.put_string_at(0, K);
    GUI.put_value_at(4, encoders[1]->cur);
    GUI.put_value_at(8, encoders[2]->cur);
    GUI.put_value_at(12, encoders[3]->cur);
    GUI.setLine(GUI.LINE2);
    if (lfo_track->enable) {
      switch (lfo_track->mode) {
      case LFO_MODE_FREE:
        GUI.put_string_at(0, "FREE");
        break;
      case LFO_MODE_TRIG:
      case LFO_MODE_ONE:
        char str[17] = "----------------";

        for (int i = 0; i < 16; i++) {
          if (IS_BIT_SET64(lfo_track->pattern_mask, i)) {

            str[i] = (char)219;
          }
        }
        GUI.put_string_at(0, str);
        break;
      }
    } else {
      GUI.put_string_at(0, "LFO: OFF");
    }
  }
#endif
#ifdef OLED_DISPLAY
  auto oldfont = oled_display.getFont();
  uint8_t lfo_track_num = lfo_track->track_number;
  mcl_gui.draw_panel_number(lfo_track_num);
  mcl_gui.draw_panel_toggle("ON", "OFF", lfo_track->enable);
  draw_page_index(false, lfo_track->step_count / 4);

  uint8_t x = mcl_gui.knob_x0 + 5;
  uint8_t y = 8;
  uint8_t lfo_height = 7;
  uint8_t width = 13;
  LFOSeqTrack temp_track;

  // mcl_gui.draw_vertical_dashline(x, 0, knob_y);
  SeqPage::draw_knob_frame();

  if (page_mode == LFO_DESTINATION) {
    draw_dest(0, encoders[0]->cur);
    draw_param(1, encoders[0]->cur, encoders[1]->cur);
    draw_dest(2, encoders[2]->cur);
    draw_param(3, encoders[2]->cur, encoders[3]->cur);
  }
  if (page_mode == LFO_SETTINGS) {
    temp_track.set_wav_type(lfo_track->wav_type);
    temp_track.set_depth(0, lfo_height);
    temp_track.load_wav_table(0);
    uint8_t inc = LFO_LENGTH / width;
    for (uint8_t n = 0; n < LFO_LENGTH; n += inc, x++) {
      if (n < LFO_LENGTH) {
        oled_display.drawPixel(x, y + lfo_height - temp_track.wav_table[0][n],
                               WHITE);
      }
    }

    x = mcl_gui.knob_x0 + 2;
    oled_display.setCursor(x + 5, 6);
    oled_display.print("WAV");

    draw_knob(1, encoders[1], "SPD");
    draw_knob(2, encoders[2], "DEP1");
    draw_knob(3, encoders[3], "DEP2");
  }
  oled_display.setFont(&TomThumb);
  const char *info1;
  const char *info2;

  if (page_mode) {
    info2 = "LFO>DST";
  } else {
    info2 = "LFO>MOD";
  }

  const uint64_t slide_mask = 0;
  const uint64_t mute_mask = 0;

  switch (lfo_track->mode) {
    case LFO_MODE_TRIG:
    info1 = "TRIG";
    break;
    case LFO_MODE_ONE:
    info1 = "ONE";
    break;
    info1 = "FREE";
    break;
  }

  if (lfo_track->mode == LFO_MODE_TRIG || lfo_track->mode == LFO_MODE_ONE) {
    draw_lock_mask(0, 0, lfo_track->step_count, lfo_track->length, true);
    draw_mask(0, lfo_track->pattern_mask, lfo_track->step_count,
              lfo_track->length, mute_mask, slide_mask, true);
    if ((uint16_t)lfo_track->pattern_mask != trigled_mask) {
      trigled_mask = (uint16_t)lfo_track->pattern_mask;
      MD.set_trigleds(lfo_track->pattern_mask, TRIGLED_STEPEDIT);
    }
  }

  mcl_gui.draw_panel_labels(info1, info2);

  oled_display.display();
  oled_display.setFont(oldfont);
#endif
}

void LFOPage::onControlChangeCallback_Midi(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t param = msg[1];
  uint8_t value = msg[2];
  uint8_t track;
  uint8_t track_param;
  // If external keyboard controlling MD pitch, send parameter updates
  // to all polyphonic tracks
  uint8_t param_true = 0;

  MD.parseCC(channel, param, &track, &track_param);
}

void LFOPage::setup_callbacks() {
  if (midi_state) {
    return;
  }
  Midi.addOnControlChangeCallback(
      this, (midi_callback_ptr_t)&LFOPage::onControlChangeCallback_Midi);

  midi_state = true;
}

void LFOPage::remove_callbacks() {
  if (!midi_state) {
    return;
  }

  Midi.removeOnControlChangeCallback(
      this, (midi_callback_ptr_t)&LFOPage::onControlChangeCallback_Midi);

  midi_state = false;
}

bool LFOPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {
    uint8_t mask = event->mask;
    uint8_t port = event->port;
    auto device = midi_active_peering.get_device(port);

    uint8_t track = event->source - 128;
    uint8_t page_select = 0;
    uint8_t step = track + (page_select * 16);
    if (event->mask == EVENT_BUTTON_PRESSED) {
      if (device == &Analog4) {
        // GUI.setPage(&seq_extstep_page)
        return true;
      }
      if (!IS_BIT_SET64(lfo_track->pattern_mask, step)) {

        SET_BIT64(lfo_track->pattern_mask, step);
      } else {
        DEBUG_PRINTLN(F("Trying to clear"));
        if (clock_diff(note_interface.note_hold[port], slowclock) <
            TRIG_HOLD_TIME) {
          CLEAR_BIT64(lfo_track->pattern_mask, step);
        }
      }
    }
  }
  if (event->mask == EVENT_BUTTON_RELEASED) {
    return true;
  }
  /*  if (EVENT_PRESSED(event, Buttons.ENCODER1) ||
        EVENT_PRESSED(event, Buttons.ENCODER2) ||
        EVENT_PRESSED(event, Buttons.ENCODER3) ||
        EVENT_PRESSED(event, Buttons.ENCODER4)) {
      GUI.setPage(&grid_page);
    }
  */
  if (EVENT_PRESSED(event, Buttons.BUTTON4)) {
    page_mode = !(page_mode);
    update_encoders();
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
    if (lfo_track->mode >= LFO_MODE_ONE) {
      lfo_track->mode = 0;
    } else {
      lfo_track->mode += 1;
    }
    if (lfo_track->mode == LFO_MODE_FREE) {
      trig_interface.off();
    } else {
      note_interface.state = true;
      trig_interface.on();
    }
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
    if (lfo_track->enable) {
      lfo_track->reset_params_offset();
    }
    lfo_track->enable = !(lfo_track->enable);
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
    GUI.setPage(&page_select_page);
    return true;
  }

  return false;
}
