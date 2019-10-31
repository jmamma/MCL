#include "LFO.h"
#include "LFOPage.h"
#include "MCL.h"
#include "MCLGUI.h"
#include "MCLSeq.h"
#include "MD.h"

#define LFO_TYPE 0
#define LFO_PARAM 1
#define INTERPOLATE
#define DIV_1_127 .0079

#define LFO_DESTINATION 0
#define LFO_SETTINGS 1

void LFOPage::setup() {
  //  lfo_track = &mcl_seq.lfo_tracks[0];
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
    md_exploit.on();
  }
}
void LFOPage::cleanup() {
  md_exploit.off();
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
  for (uint8_t i = 0; i < GUI_NUM_ENCODERS; i++) {
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
      lfo_track->params[0].offset = lfo_track->params[0].get_param_offset(
          encoders[0]->cur, encoders[1]->cur);
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
      lfo_track->params[1].offset = lfo_track->params[1].get_param_offset(
          encoders[2]->cur, encoders[3]->cur);
      lfo_track->params[1].update_offset();
      CLEAR_LOCK();
    }
  }
  if (page_mode == LFO_SETTINGS) {
    if (encoders[0]->hasChanged()) {
      waveform = encoders[0]->cur;
      load_wavetable(waveform, lfo_track, 0, lfo_track->params[0].depth);
      load_wavetable(waveform, lfo_track, 1, lfo_track->params[1].depth);
    }

    if (encoders[1]->hasChanged()) {
      lfo_track->speed = encoders[1]->cur;
    }

    if (encoders[2]->hasChanged()) {
      lfo_track->params[0].depth = encoders[2]->cur;
      load_wavetable(waveform, lfo_track, 0, encoders[2]->cur);
    }

    if (encoders[3]->hasChanged()) {
      lfo_track->params[1].depth = encoders[3]->cur;
      load_wavetable(waveform, lfo_track, 1, encoders[3]->cur);
    }
  }
}

void LFOPage::load_wavetable(uint8_t waveform, LFOSeqTrack *lfo_track,
                             uint8_t param, uint8_t depth) {
  SinLFO sin_lfo;
  TriLFO tri_lfo;
  RampLFO ramp_lfo;
  IRampLFO iramp_lfo;
  ExpLFO exp_lfo;
  IExpLFO iexp_lfo;
  LFO *lfo;
  switch (waveform) {
  case SIN_WAV:
    lfo = (LFO *)&sin_lfo;
    lfo_track->offset_behaviour = LFO_OFFSET_CENTRE;
    break;
  case TRI_WAV:
    lfo = (LFO *)&tri_lfo;
    lfo_track->offset_behaviour = LFO_OFFSET_CENTRE;
    break;
  case IRAMP_WAV:
    lfo = (LFO *)&iramp_lfo;
    lfo_track->offset_behaviour = LFO_OFFSET_MAX;
    break;
  case RAMP_WAV:
    lfo = (LFO *)&ramp_lfo;
    lfo_track->offset_behaviour = LFO_OFFSET_MAX;
    break;
  case EXP_WAV:
    lfo = (LFO *)&exp_lfo;
    lfo_track->offset_behaviour = LFO_OFFSET_MAX;
    break;
  case IEXP_WAV:
    lfo = (LFO *)&iexp_lfo;
    lfo_track->offset_behaviour = LFO_OFFSET_MAX;
    break;
  }
  lfo->amplitude = depth;
  // ExpLFO exp_lfo(20);
  for (uint8_t n = 0; n < LFO_LENGTH; n++) {
    lfo_track->wav_table[param][n] = (float)lfo->get_sample(n);
  }
}

void LFOPage::draw_param(uint8_t knob, uint8_t dest, uint8_t param) {

  char myName[4] = "-- ";

  PGM_P modelname = NULL;
  if (dest != 0) {
    if (dest < 17) {
      modelname = model_param_name(MD.kit.models[dest - 1], param);
    } else {
      modelname = fx_param_name(MD_FX_ECHO + dest - 17, param);
    }
    if (modelname != NULL) {
      m_strncpy_p(myName, modelname, 4);
    }
  }
  draw_knob(knob, "PAR", myName);
}

void LFOPage::draw_dest(uint8_t knob, uint8_t value) {
  char K[4];
  switch (value) {
  case 0:
    strcpy(K, "--");
    break;
  case 17:
    strcpy(K, "DEL");
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
  draw_knob(knob, "DST", K);
}

void LFOPage::display() {
  auto oldfont = oled_display.getFont();
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

#endif
#ifdef OLED_DISPLAY
  oled_display.setTextColor(WHITE);
  oled_display.setFont(&Elektrothic);
  oled_display.setCursor(trackid_x, trackid_y);
  uint8_t lfo_track_num = lfo_track->track_number;
  if (lfo_track_num < 10) {
    oled_display.print('0');
  }
  oled_display.print(lfo_track_num);
  oled_display.setFont(&TomThumb);
  //  draw MD/EXT label

  oled_display.setCursor(label_x + 1, label_md_y + 6);
  oled_display.print("LFO");
  if ((lfo_track->enable)) {
    oled_display.fillRect(label_x, label_md_y + 7, label_w, label_h, WHITE);
    oled_display.setCursor(label_x + 1, label_md_y + 13);
    oled_display.setTextColor(BLACK);
    oled_display.print("ON");
    oled_display.setTextColor(WHITE);
  } else {
    oled_display.setCursor(label_x + 1, label_md_y + 13);
    oled_display.print("OFF");
  }
  draw_page_index();

  oled_display.setCursor(0, 0);

  uint8_t x = knob_x0 + 4;
  uint8_t y = 8;
  uint8_t lfo_height = 7;
  uint8_t width = 13;
  LFOSeqTrack temp_track;

  // mcl_gui.draw_vertical_dashline(x, 0, knob_y);
  SeqPage::draw_knob_frame();
  char K[4];

  if (page_mode == LFO_DESTINATION) {
    draw_dest(0, encoders[0]->cur);
    draw_param(1, encoders[0]->cur, encoders[1]->cur);
    draw_dest(2, encoders[2]->cur);
    draw_param(3, encoders[2]->cur, encoders[3]->cur);
  }
  if (page_mode == LFO_SETTINGS) {
    /*
        char K[4];
        switch (encoders[0]->cur) {
        case SIN_WAV:
          strcpy(K, "SIN");
          break;
        case TRI_WAV:
          strcpy(K, "TRI");
          break;
        case IRAMP_WAV:
          strcpy(K, "SAW");
          break;
        case RAMP_WAV:
          strcpy(K, "RMP");
          break;
        case EXP_WAV:
          strcpy(K, "DEC");
          break;
        case IEXP_WAV:
          strcpy(K, "EXP");
          break;
        }
        strcpy(K, "");
    */ 
    load_wavetable(waveform, &temp_track, 0, lfo_height);
    uint8_t inc = LFO_LENGTH / width;
      for (uint8_t n = 0; n < LFO_LENGTH; n += inc, x++) {
      if (n < LFO_LENGTH) {
      oled_display.drawPixel(x, y + lfo_height - temp_track.wav_table[0][n],
                             WHITE);
      }
    }
  
    x = knob_x0 + 2;
    oled_display.setCursor(x + 4, 7);
    oled_display.print("WAV");

    draw_knob(1, encoders[1], "SPD");
    draw_knob(2, encoders[2], "DEP1");
    draw_knob(3, encoders[3], "DEP2");
  }
  // draw_pattern_mask();
  oled_display.setFont(&TomThumb);
  // oled_display.setCursor(1, info1_y + 6);
  // oled_display.print("MODE: ");

  oled_display.setCursor(1, info2_y + 6);
  switch (lfo_track->mode) {
  case LFO_MODE_FREE:
    oled_display.print("FREE");
    break;
  case LFO_MODE_TRIG:
    draw_lock_mask(0, 0, lfo_track->step_count, lfo_track->length, true);
    draw_pattern_mask(0, lfo_track->pattern_mask, lfo_track->step_count,
                      lfo_track->length, true);
    oled_display.print("TRIG");
    break;
  case LFO_MODE_ONE:
    draw_lock_mask(0, 0, lfo_track->step_count, lfo_track->length, true);
    draw_pattern_mask(0, lfo_track->pattern_mask, lfo_track->step_count,
                      lfo_track->length, true);
    oled_display.print("TRIG ONE");
    break;
  }

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
    uint8_t device = midi_active_peering.get_device(port);

    uint8_t track = event->source - 128;
    uint8_t page_select = 0;
    uint8_t step = track + (page_select * 16);
    uint8_t midi_device = device;
    if (event->mask == EVENT_BUTTON_PRESSED) {
      if (device == DEVICE_A4) {
        // GUI.setPage(&seq_extstep_page)
        return true;
      }
      if (!IS_BIT_SET64(lfo_track->pattern_mask, step)) {

        SET_BIT64(lfo_track->pattern_mask, step);
      } else {
        DEBUG_PRINTLN("Trying to clear");
        if (clock_diff(note_interface.note_hold, slowclock) < TRIG_HOLD_TIME) {
          CLEAR_BIT64(lfo_track->pattern_mask, step);
        }
      }
    }
  }
  if (event->mask == EVENT_BUTTON_RELEASED) {
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.ENCODER1) ||
      EVENT_PRESSED(event, Buttons.ENCODER2) ||
      EVENT_PRESSED(event, Buttons.ENCODER3) ||
      EVENT_PRESSED(event, Buttons.ENCODER4)) {
    GUI.setPage(&grid_page);
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
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
      md_exploit.off();
    } else {
      md_exploit.on();
    }
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON4)) {
    lfo_track->enable = !(lfo_track->enable);
    if (!lfo_track->enable) {
      lfo_track->reset_params_offset();
    }
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
    GUI.setPage(&page_select_page);
    return true;
  }

  return false;
}
