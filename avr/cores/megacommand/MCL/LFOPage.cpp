#include "LFOPage.h"
#include "LFO.h"
#include "MCL.h"
#include "MCLSeq.h"

#define LFO_TYPE 0
#define LFO_PARAM 1
#define INTERPOLATE
#define DIV_1_127 .0079

#define LFO_DESTINATION 0
#define LFO_SETTINGS 1

void LFOPage::setup() { DEBUG_PRINT_FN(); }

void LFOPage::init() {
  DEBUG_PRINT_FN();
#ifdef OLED_DISPLAY
  classic_display = false;
  oled_display.clearDisplay();
  oled_display.setFont();
#endif
  md_exploit.on();
  update_encoders();
}
void LFOPage::cleanup() {
    md_exploit.off();
#ifdef OLED_DISPLAY
  oled_display.clearDisplay();
#endif
}

void LFOPage::update_encoders() {
  if (page_mode == LFO_DESTINATION) {
    encoders[0]->cur = mcl_seq.lfo_tracks[0].params[0].dest;
    ((MCLEncoder*)encoders[0])->max = NUM_MD_TRACKS + 4;
    encoders[1]->cur = mcl_seq.lfo_tracks[0].params[0].param;
    ((MCLEncoder*)encoders[1])->max = 24;
    encoders[2]->cur = mcl_seq.lfo_tracks[0].params[1].dest;
    ((MCLEncoder*)encoders[2])->max = 16 + 4;
    encoders[3]->cur = mcl_seq.lfo_tracks[0].params[1].param;
    ((MCLEncoder*)encoders[3])->max = 24;
  }
 if (page_mode == LFO_SETTINGS) {
    encoders[0]->cur = waveform;
    ((MCLEncoder*)encoders[0])->max = 2;
    encoders[1]->cur = mcl_seq.lfo_tracks[0].mode;
    ((MCLEncoder*)encoders[1])->max = 2;
    encoders[2]->cur = mcl_seq.lfo_tracks[0].speed;
   ((MCLEncoder*)encoders[2])->max = 127;
    encoders[3]->cur = depth;
    ((MCLEncoder*)encoders[3])->max = 127;
  }
}

void LFOPage::loop() {
 if (page_mode == LFO_DESTINATION) {

  if (encoders[0]->hasChanged()) {
    USE_LOCK();
    SET_LOCK();
    mcl_seq.lfo_tracks[0].params[0].reset_param_offset();
    mcl_seq.lfo_tracks[0].params[0].dest = encoders[0]->cur;
    mcl_seq.lfo_tracks[0].params[0].update_offset();
    CLEAR_LOCK();
    if (encoders[0]->cur >= NUM_MD_TRACKS) {
      ((MCLEncoder*)encoders[1])->max = 8;
    }

  }
  if (encoders[1]->hasChanged()) {
    USE_LOCK();
    SET_LOCK();
    mcl_seq.lfo_tracks[0].params[0].reset_param_offset();
    mcl_seq.lfo_tracks[0].params[0].param = encoders[1]->cur;
    mcl_seq.lfo_tracks[0].params[0].offset = mcl_seq.lfo_tracks[0].params[0].get_param_offset(encoders[0]->cur, encoders[1]->cur);
    mcl_seq.lfo_tracks[0].params[0].update_offset();
    CLEAR_LOCK();
  }

  if (encoders[2]->hasChanged()) {
     USE_LOCK();
    SET_LOCK();
    mcl_seq.lfo_tracks[0].params[1].reset_param_offset();
    mcl_seq.lfo_tracks[0].params[1].dest = encoders[2]->cur;
    mcl_seq.lfo_tracks[0].params[1].update_offset();
    CLEAR_LOCK();
    if (encoders[0]->cur >= NUM_MD_TRACKS) {
      ((MCLEncoder*)encoders[3])->max = 8;
    }

  }
  if (encoders[3]->hasChanged()) {
    USE_LOCK();
    SET_LOCK();
    mcl_seq.lfo_tracks[0].params[1].reset_param_offset();
    mcl_seq.lfo_tracks[0].params[1].param = encoders[2]->cur;
    mcl_seq.lfo_tracks[0].params[1].offset = mcl_seq.lfo_tracks[0].params[1].get_param_offset(encoders[2]->cur, encoders[3]->cur);
    mcl_seq.lfo_tracks[0].params[1].update_offset();
    CLEAR_LOCK();
  }


  }
 if (page_mode == LFO_SETTINGS) {
  if (encoders[0]->hasChanged()) {
    waveform = encoders[0]->cur;
    load_wavetable(waveform, &mcl_seq.lfo_tracks[0], depth);
  }
  if (encoders[1]->hasChanged()) {
    mcl_seq.lfo_tracks[0].mode = encoders[1]->cur;
  }

  if (encoders[2]->hasChanged()) {
    mcl_seq.lfo_tracks[0].speed = encoders[2]->cur;
  }

  if (encoders[3]->hasChanged()) {
    depth = encoders[3]->cur;
    mcl_seq.lfo_tracks[0].params[0].depth = depth;
    mcl_seq.lfo_tracks[0].params[1].depth = depth;
    load_wavetable(waveform, &mcl_seq.lfo_tracks[0], depth);
  }


  }
}

void LFOPage::load_wavetable(uint8_t waveform, LFOSeqTrack *lfo_track, uint8_t depth) {
  SinLFO sin_lfo;
  TriLFO tri_lfo;
  ExpLFO exp_lfo;
  LFO *lfo;

  switch (waveform) {
    case SIN_WAV:
      lfo = (LFO*) &sin_lfo;
      lfo_track->offset_behaviour = LFO_OFFSET_CENTRE;
    break;
    case TRI_WAV:
      lfo = (LFO*) &tri_lfo;
      lfo_track->offset_behaviour = LFO_OFFSET_CENTRE;
    break;
    case EXP_WAV:
      lfo = (LFO*) &exp_lfo;
      lfo_track->offset_behaviour = LFO_OFFSET_MAX;
    break;
  }
  //ExpLFO exp_lfo(20);
  for (uint8_t n = 0; n < LFO_LENGTH; n++) {
  lfo_track->wav_table[n] = (float)lfo->get_sample(n) * ((float)depth * (float) (DIV_1_127));
  }
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

#endif
#ifdef OLED_DISPLAY
  oled_display.setFont();
  oled_display.setCursor(0, 0);

  oled_display.print("LFO ");
  oled_display.print(page_mode ? 1 : 0);
  oled_display.print(" ");

/*  PGM_P param_name = NULL;
  char str[4];
  for (uint8_t i = 0; i < GUI_NUM_ENCODERS; i++) {
    uint8_t n = i + ((page_mode ? 1 : 0) * GUI_NUM_ENCODERS);

    uint8_t fx_param = params[n].param;
    uint8_t fx_type = params[n].type;
    param_name = fx_param_name(fx_type, fx_param);
    m_strncpy_p(str, param_name, 4);

    mcl_gui.draw_light_encoder(30 + 20 * i, 18, encoders[i], str);

 //   mcl_gui.draw_md_encoder(30 + 20 * i, 6, encoders[i], str);
  }
*/

  uint8_t x = 0;
  uint8_t h = 30;
  uint8_t y = 0;

  for (uint8_t n = 0; n < LFO_LENGTH; n++) {
  oled_display.drawPixel(x + n, (float) 32 - mcl_seq.lfo_tracks[0].wav_table[n] * ((float) h * (float)DIV_1_127), WHITE);
    if (n % 2 == 0) {
      oled_display.drawPixel(x + n, (h / 2) + y, WHITE);
    }

  }
  uint8_t i = 0;
  if (page_mode == LFO_DESTINATION) {

       mcl_gui.draw_light_encoder(30 + 20 * i, 5, encoders[i++], "DST");
       mcl_gui.draw_light_encoder(30 + 20 * i, 5, encoders[i++], "PAR");
       mcl_gui.draw_light_encoder(30 + 20 * i, 5, encoders[i++], "DST");
       mcl_gui.draw_light_encoder(30 + 20 * i, 5, encoders[i++], "PAR");

  }
 if (page_mode == LFO_SETTINGS) {
        mcl_gui.draw_light_encoder(30 + 20 * i, 5, encoders[i++], "SHP");
       mcl_gui.draw_light_encoder(30 + 20 * i, 5, encoders[i++], "TYP");
       mcl_gui.draw_light_encoder(30 + 20 * i, 5, encoders[i++], "SPD");
       mcl_gui.draw_light_encoder(30 + 20 * i, 5, encoders[i++], "DEP");
 }

  draw_pattern_mask();

  oled_display.display();

#endif
}

void LFOPage::draw_pattern_mask() {

  uint8_t trig_x = 10;
  uint8_t trig_y = 20;
  uint8_t seq_w = 5;
  uint8_t trig_h = 5;

    uint64_t pattern_mask = mcl_seq.lfo_tracks[0].pattern_mask;

    uint8_t offset = 0;
    for (int i = 0; i < 16; i++) {

      uint8_t idx = i + offset;
      bool in_range = idx <  mcl_seq.lfo_tracks[0].length;

      if (note_interface.notes[i] == 1) {
        // TI feedback
        oled_display.fillRect(trig_x - 1, trig_y, seq_w + 2, trig_h + 1, WHITE);
      } else if (!in_range) {
        // don't draw
      } else {
        if (IS_BIT_SET64(pattern_mask, i + offset)) {
          /*If the bit is set, there is a trigger at this position. */
          oled_display.fillRect(trig_x, trig_y, seq_w, trig_h, WHITE);
        } else {
          oled_display.drawRect(trig_x, trig_y, seq_w, trig_h, WHITE);
        }
      }

      trig_x += seq_w + 1;
    }
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
        // GUI.setPage(&seq_extstep_page);
        return true;
      }
      if (!IS_BIT_SET64(mcl_seq.lfo_tracks[0].pattern_mask, step)) {

        SET_BIT64(mcl_seq.lfo_tracks[0].pattern_mask, step);
      } else {
        DEBUG_PRINTLN("Trying to clear");
        if (clock_diff(note_interface.note_hold,slowclock) < TRIG_HOLD_TIME) {
          CLEAR_BIT64(mcl_seq.lfo_tracks[0].pattern_mask, step);
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
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON4)) {
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
    GUI.setPage(&page_select_page);
    return true;
  }

  return false;
}
