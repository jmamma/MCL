#include "MCL_impl.h"

#define FADER_LEN 16
#define FADE_RATE 16

void MixerPage::set_display_mode(uint8_t param) {
  display_mode = param;
  switch (param) {
  case MODEL_FLTF:
    strcpy(info_line2, "FLTF");
    break;
  case MODEL_FLTW:
    strcpy(info_line2, "FLTW");
    break;
  case MODEL_FLTQ:
    strcpy(info_line2, "FLTQ");
    break;
  case MODEL_LEVEL:
    display_mode = MODEL_LEVEL;
    strcpy(info_line2, "VOLUME");
    break;
  default:
    strcpy(info_line2, "PAR");
    break;
  }
}

#ifdef OLED_DISPLAY
static void oled_draw_routing() {
  for (int i = 0; i < 16; ++i) {
    // draw routing
    if (note_interface.is_note(i)) {

      oled_display.fillRect(0 + i * 8, 2, 6, 6, WHITE);
    }

    else if (mcl_cfg.routing[i] == 6) {

      oled_display.fillRect(0 + i * 8, 2, 6, 6, BLACK);
      oled_display.drawRect(0 + i * 8, 2, 6, 6, WHITE);

    }

    else {

      oled_display.fillRect(0 + i * 8, 2, 6, 6, BLACK);
      oled_display.drawLine(+i * 8, 5, 5 + (i * 8), 5, WHITE);
    }
  }
}

#endif

void MixerPage::setup() {
  encoders[0]->handler = encoder_level_handle;
  encoders[1]->handler = encoder_filtf_handle;
  encoders[2]->handler = encoder_filtw_handle;
  encoders[3]->handler = encoder_filtq_handle;
  if (route_page.encoders[0]->cur == 0) {
    route_page.encoders[0]->cur = 2;
  }
  create_chars_mixer();
#ifdef OLED_DISPLAY
  classic_display = false;
  oled_display.clearDisplay();
#endif
}

void MixerPage::init() {
  memcpy(params, MD.kit.params, sizeof(params));
  level_pressmode = 0;
  for (uint8_t i = 0; i < 4; i++) {
    encoders[i]->cur = 64;
    encoders[i]->old = 64;
  }
  bool switch_tracks = false;
  note_interface.state = true;
  midi_events.setup_callbacks();

#ifdef OLED_DISPLAY
  oled_display.clearDisplay();
  oled_draw_routing();
  set_display_mode(MODEL_LEVEL);
  for (uint8_t i = 0; i < 16; i++) {
    uint8_t scaled_level =
        (uint8_t)(((float)MD.kit.levels[i] / (float)127) * (float)FADER_LEN);

    oled_display.drawRect(0 + i * 8, 12 + (FADER_LEN - scaled_level), 6,
                          scaled_level + 1, WHITE);
    disp_levels[i] = 0;
  }
#endif
}

void MixerPage::cleanup() {
//  md_exploit.off();
#ifdef OLED_DISPLAY
  oled_display.clearDisplay();
#endif
  note_interface.state = false;

  midi_events.remove_callbacks();
}

void MixerPage::set_level(int curtrack, int value) {
  // in_sysex = 1;
  MD.kit.levels[curtrack] = value;
  MD.setTrackParam(curtrack, 33, value);
  // in_sysex = 0;
}

void MixerPage::loop() {}

void MixerPage::draw_levels() {
  GUI.setLine(GUI.LINE2);
  uint8_t scaled_level;
  uint8_t scaled_level2;
  char str[17] = "                ";
  uint8_t fader_level;
  for (int i = 0; i < 16; i++) {

    if (display_mode == MODEL_LEVEL) {
      fader_level = MD.kit.levels[i];
    } else {
      fader_level = MD.kit.params[i][display_mode];
    }
    scaled_level = (int)(((float)fader_level / (float)127) * 7);
    if (scaled_level == 7) {
      str[i] = (char)(255);
    } else if (scaled_level > 0) {
      str[i] = (char)(scaled_level + 2);
    }
  }
  GUI.put_string_at(0, str);
  uint8_t dec = MidiClock.get_tempo() / FADE_RATE;
  for (uint8_t n = 0; n < 16; n++) {
    if (disp_levels[n] < dec) {
      disp_levels[n] = 0;
    } else {
      disp_levels[n] -= dec;
    }
  }
}

void encoder_level_handle(EncoderParent *enc) {

  bool redraw_frame = (mixer_page.display_mode != MODEL_LEVEL);

  mixer_page.set_display_mode(MODEL_LEVEL);

  int dir = enc->getValue() - enc->old;
  int track_newval;

  for (int i = 0; i < 16; i++) {
    if (note_interface.is_note_on(i)) {
      track_newval = MD.kit.levels[i] + dir;
      if (track_newval < 0) {
        track_newval = 0;
      }
      if (track_newval > 127) {
        track_newval = 127;
      }
      for (uint8_t level = MD.kit.levels[i]; level < track_newval; level++) {
        mixer_page.set_level(i, level);
      }
      for (uint8_t level = MD.kit.levels[i]; level > track_newval; level--) {
        mixer_page.set_level(i, level);
      }
      // if ((MD.kit.levels[i] < 127) && (MD.kit.levels[i] > 0)) {
      mixer_page.set_level(i, track_newval);
    }

    if (note_interface.is_note_on(i) || redraw_frame) {
#ifdef OLED_DISPLAY
      uint8_t scaled_level = (MD.kit.levels[i] / 127.0f) * FADER_LEN;

      oled_display.fillRect(0 + i * 8, 12, 6, FADER_LEN, BLACK);
      oled_display.drawRect(0 + i * 8, 12 + (FADER_LEN - scaled_level), 6,
                            scaled_level + 1, WHITE);
#endif
    }
  }
  enc->cur = 64 + dir;
  enc->old = 64;
}

void encoder_filtf_handle(EncoderParent *enc) {
  mixer_page.adjust_param(enc, MODEL_FLTF);
}

void encoder_filtw_handle(EncoderParent *enc) {
  mixer_page.adjust_param(enc, MODEL_FLTW);
}

void encoder_filtq_handle(EncoderParent *enc) {
  mixer_page.adjust_param(enc, MODEL_FLTQ);
}

void encoder_lastparam_handle(EncoderParent *enc) {
  mixer_page.adjust_param(enc, MD.midi_events.last_md_param);
}

void MixerPage::adjust_param(EncoderParent *enc, uint8_t param) {

  set_display_mode(param);

  int dir = enc->getValue() - enc->old;
  int newval;

  for (int i = 0; i < 16; i++) {
    if (note_interface.is_note_on(i)) {
      newval = MD.kit.params[i][param] + dir;
      if (newval < 0) {
        newval = 0;
      }
      if (newval > 127) {
        newval = 127;
      }
      for (uint8_t value = MD.kit.params[i][param]; value < newval; value++) {
        MD.setTrackParam(i, param, value);
        MD.kit.params[i][param] = value;
      }
      for (uint8_t value = MD.kit.params[i][param]; value > newval; value--) {
        MD.setTrackParam(i, param, value);
        MD.kit.params[i][param] = value;
      }
      MD.setTrackParam(i, param, newval);
      MD.kit.params[i][param] = newval;

#ifdef OLED_DISPLAY
      uint8_t scaled_level = (newval / 127.0f) * FADER_LEN;

      oled_display.fillRect(0 + i * 8, 12, 6, FADER_LEN, BLACK);
      oled_display.fillRect(0 + i * 8, 12 + (FADER_LEN - scaled_level), 6,
                            scaled_level + 1, WHITE);
#endif
    }
  }
  enc->cur = 64 + dir;
  enc->old = 64;
}

#ifndef OLED_DISPLAY
void MixerPage::display() {
  note_interface.draw_notes(0);
  if (!classic_display) {
    LCD.goLine(0);
    LCD.puts(GUI.lines[0].data);
  }
  GUI.setLine(GUI.LINE2);
  uint8_t scaled_level;
  uint8_t scaled_level2;
  char str[17] = "                ";
  for (int i = 0; i < 16; i++) {
    str[i] = (char)219;

    if (mcl_cfg.routing[i] == 6) {

      str[i] = (char)'-';
    }
    if (note_interface.is_note(i)) {

      str[i] = (char)255;
    }
  }
  GUI.put_string_at(0, str);
  draw_levels();
}
#else

void MixerPage::display() {

  auto oldfont = oled_display.getFont();

  uint8_t fader_level;
  uint8_t meter_level;
  uint8_t fader_x = 0;
  for (int i = 0; i < 16; i++) {

    if (display_mode == MODEL_LEVEL) {
      fader_level = MD.kit.levels[i];
    } else {
      fader_level = MD.kit.params[i][display_mode];
    }

    fader_level = (fader_level / 127.0f) * FADER_LEN + 1;
    meter_level = (disp_levels[i] / 127.0f) * FADER_LEN + 1;

    if (display_mode == MODEL_LEVEL) {

      if (note_interface.is_note_on(i)) {
        oled_display.fillRect(fader_x, 13 + (FADER_LEN - fader_level), 6,
                              fader_level, WHITE);
      } else {

        // draw meter only if not pressed
        oled_display.fillRect(fader_x + 1, 14 + (FADER_LEN - fader_level), 4,
                              FADER_LEN - meter_level, BLACK);
        oled_display.fillRect(fader_x + 1, 13 + (FADER_LEN - meter_level), 4,
                              meter_level, WHITE);
      }
    } else {
      meter_level = min(fader_level, meter_level);
      oled_display.fillRect(0 + i * 8, 12, 6, FADER_LEN, BLACK);
      oled_display.fillRect(fader_x, 13 + (FADER_LEN - fader_level), 6,
                            fader_level, WHITE);
      oled_display.fillRect(fader_x + 1, 14 + (FADER_LEN - meter_level), 4,
                            meter_level - 2, BLACK);
    }

    fader_x += 8;
  }

  uint8_t dec = MidiClock.get_tempo() / FADE_RATE;
  for (uint8_t n = 0; n < 16; n++) {
    if (disp_levels[n] < dec) {
      disp_levels[n] = 0;
    } else {
      disp_levels[n] -= dec;
    }
  }

  oled_display.display();
  oled_display.setFont(oldfont);
}
#endif

bool MixerPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {
    uint8_t mask = event->mask;
    uint8_t port = event->port;
    uint8_t device = midi_active_peering.get_device(port)->id;

    uint8_t track = event->source - 128;

    if (track > 16) {
      return false;
    }

    trig_interface.send_md_leds(TRIGLED_OVERLAY);

    if (event->mask == EVENT_BUTTON_PRESSED) {
#ifdef OLED_DISPLAY
      if (note_interface.is_note(track)) {
        oled_display.fillRect(0 + track * 8, 2, 6, 6, WHITE);
        if (note_interface.notes_count_on() == 1) {
          MD.setStatus(0x22, track);
        }
      }

#endif
      return true;
    }

    if (event->mask == EVENT_BUTTON_RELEASED) {
#ifndef OLED_DISPLAY
      note_interface.draw_notes(0);
#else
      uint8_t i = track;
      uint8_t scaled_level = (MD.kit.levels[i] / 127.0f) * FADER_LEN;
      oled_display.fillRect(0 + i * 8, 12, 6, FADER_LEN, BLACK);
      oled_display.drawRect(0 + i * 8, 12 + (FADER_LEN - scaled_level), 6,
                            scaled_level + 1, WHITE);

#endif

      if (note_interface.notes_count_on() == 0) {
        //  encoder_level_handle(mixer_page.encoders[0]);
        if (BUTTON_DOWN(Buttons.BUTTON4)) {
          route_page.toggle_routes_batch();
        }
        if (BUTTON_DOWN(Buttons.BUTTON1)) {
          route_page.toggle_routes_batch(true);
        }
        note_interface.init_notes();
#ifdef OLED_DISPLAY
        oled_draw_routing();
#endif
      }
      return true;
    }
  }
  /*
    if (EVENT_PRESSED(event, Buttons.BUTTON4)) {
            route_page.toggle_routes_batch();
          note_interface.init_notes();
  #ifdef OLED_DISPLAY
         route_page.draw_routes(0);
  #endif
    }
  */

  if (EVENT_CMD(event)) {
    uint8_t key = event->source - 64;
    if (event->mask == EVENT_BUTTON_PRESSED) {
      switch (key) {
      case MDX_KEY_NO: {
        goto reset_params;
      }
      }
    }
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
    trig_interface.on();
    GUI.setPage(&page_select_page);
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
  reset_params:
    for (uint8_t i = 0; i < 16; i++) {
      if (note_interface.is_note_on(i)) {
        for (uint8_t c = 0; c < 24; c++) {
          if (MD.kit.params[i][c] != params[i][c]) {
            MD.setTrackParam(i, c, params[i][c]);
            MD.kit.params[i][c] = params[i][c];
          }
        }
      }
    }
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.ENCODER1) ||
      EVENT_PRESSED(event, Buttons.ENCODER2) ||
      EVENT_PRESSED(event, Buttons.ENCODER3) ||
      EVENT_PRESSED(event, Buttons.ENCODER4)) {
    //    if (note_interface.notes_count() == 0) {
    //      route_page.update_globals();
    //      GUI.setPage(&grid_page);
    //    }
    return true;
  }

  return false;
}

void MixerMidiEvents::setup_callbacks() {
  if (state) {
    return;
  }
  Midi.addOnNoteOnCallback(
      this, (midi_callback_ptr_t)&MixerMidiEvents::onNoteOnCallback_Midi);
  Midi.addOnNoteOffCallback(
      this, (midi_callback_ptr_t)&MixerMidiEvents::onNoteOffCallback_Midi);
  Midi.addOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&MixerMidiEvents::onControlChangeCallback_Midi);

  state = true;
}

void MixerMidiEvents::remove_callbacks() {
  if (!state) {
    return;
  }

  DEBUG_PRINTLN(F("remove calblacks"));
  Midi.removeOnNoteOnCallback(
      this, (midi_callback_ptr_t)&MixerMidiEvents::onNoteOnCallback_Midi);
  Midi.removeOnNoteOffCallback(
      this, (midi_callback_ptr_t)&MixerMidiEvents::onNoteOffCallback_Midi);
  Midi.removeOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&MixerMidiEvents::onControlChangeCallback_Midi);

  state = false;
}

void MixerMidiEvents::onControlChangeCallback_Midi(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t param = msg[1];
  uint8_t value = msg[2];
  uint8_t track;
  uint8_t track_param;

  MD.parseCC(channel, param, &track, &track_param);
  if (track_param == 32) {
    return;
  } // don't process mute
  for (int i = 0; i < 16; i++) {
    if ((note_interface.is_note_on(i)) && (i != track)) {
      MD.setTrackParam(i, track_param, value);
      if (track_param < 24) {
        MD.kit.params[i][track_param] = value;
      } else {
        MD.kit.levels[i] = value;
      }
    }
    if (track_param == 33) {
      uint8_t scaled_level = (MD.kit.levels[i] / 127.0f) * FADER_LEN;
#ifdef OLED_DISPLAY
      oled_display.fillRect(0 + i * 8, 12, 6, FADER_LEN, BLACK);
      oled_display.drawRect(0 + i * 8, 12 + (FADER_LEN - scaled_level), 6,
                            scaled_level + 1, WHITE);
#endif
    }
  }
  mixer_page.set_display_mode(track_param);
}

uint8_t MixerMidiEvents::note_to_trig(uint8_t note_num) {
  uint8_t trig_num = 0;
  for (uint8_t i = 0; i < sizeof(MD.global.drumMapping); i++) {
    if (note_num == MD.global.drumMapping[i]) {
      trig_num = i;
    }
  }
  return trig_num;
}
void MixerMidiEvents::onNoteOnCallback_Midi(uint8_t *msg) {
  uint8_t note_num = msg[1];
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);

  uint8_t n = note_to_trig(msg[1]);
  if (msg[0] != 153) {
    mixer_page.disp_levels[n] = MD.kit.levels[n];
  }
}
void MixerMidiEvents::onNoteOffCallback_Midi(uint8_t *msg) {}
