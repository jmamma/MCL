#include "MCL.h"
#include "MixerPage.h"
#define FADER_LEN 16

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
  level_pressmode = 0;
  encoders[0]->cur = 60;
  encoders[1]->cur = 60;
  encoders[2]->cur = 60;
  encoders[3]->cur = 60;
  bool switch_tracks = false;
  note_interface.state = true;
  midi_events.setup_callbacks();
  for (uint8_t i = 0; i < 16; i++) {
    for (uint8_t c = 0; c < 3; c++) {
      params[i][c] = MD.kit.params[i][MODEL_FLTF + c];
    }
  }
#ifdef OLED_DISPLAY
  oled_display.clearDisplay();
  draw_routes(0);

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


void MixerPage::draw_routes(uint8_t line_number) {
  if (line_number == 0) {
    GUI.setLine(GUI.LINE1);
  } else {
    GUI.setLine(GUI.LINE2);
  }
  /*Initialise the string with blank steps*/
  char str[17] = "----------------";

  for (int i = 0; i < 16; i++) {

#ifdef OLED_DISPLAY
    if (note_interface.notes[i] > 0) {

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

#else

    str[i] = (char)219;

    if (mcl_cfg.routing[i] == 6)  {

      str[i] = (char)'-';
    }
    if (note_interface.notes[i] > 0) {

      str[i] = (char)255;
    }
#endif
  }
#ifndef OLED_DISPLAY
  GUI.put_string_at(0, str);
#endif
}

void MixerPage::set_level(int curtrack, int value) {
  // in_sysex = 1;
  MD.kit.levels[curtrack] = value;
  USE_LOCK();
  SET_LOCK();
  MD.setTrackParam(curtrack, 33, value);
  CLEAR_LOCK();
  // in_sysex = 0;
}

void MixerPage::loop() {}

void MixerPage::draw_levels() {
  GUI.setLine(GUI.LINE2);
  uint8_t scaled_level;
  uint8_t scaled_level2;
  char str[17] = "                ";
  for (int i = 0; i < 16; i++) {
//  if (MD.kit.levels[i] > 120) { scaled_level = 8; }
// else if (MD.kit.levels[i] < 4) { scaled_level = 0; }
#ifdef OLED_DISPLAY

    scaled_level =
        (uint8_t)(((float)MD.kit.levels[i] / (float)127) * (float)(FADER_LEN)) +
        1;

    scaled_level2 =
        (uint8_t)(((float)disp_levels[i] / (float)127) * (float)(FADER_LEN)) +
        1;

    if (note_interface.notes[i] == 1) {
      oled_display.fillRect(0 + i * 8, 13 + (FADER_LEN - scaled_level), 6,
                            scaled_level, WHITE);
    } else {

      oled_display.fillRect(1 + i * 8, 14 + (FADER_LEN - scaled_level), 4,
                            FADER_LEN - scaled_level2, BLACK);
      oled_display.fillRect(1 + i * 8, 13 + (FADER_LEN - scaled_level2), 4,
                            scaled_level2, WHITE);
    }
#else

    scaled_level = (int)(((float)MD.kit.levels[i] / (float)127) * 7);
    if (scaled_level == 7) {
      str[i] = (char)(255);
    } else if (scaled_level > 0) {
      str[i] = (char)(scaled_level + 2);
    }
#endif
  }
  GUI.put_string_at(0, str);
}

void encoder_level_handle(Encoder *enc) {

  int dir = enc->getValue() - enc->old;
  int track_newval;

  for (int i = 0; i < 16; i++) {
    if (note_interface.notes[i] == 1) {
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
#ifdef OLED_DISPLAY
      uint8_t scaled_level = ((uint8_t)(((float)MD.kit.levels[i] / (float)127) *
                                        (float)FADER_LEN));

      oled_display.fillRect(0 + i * 8, 12, 6, FADER_LEN, BLACK);
      oled_display.drawRect(0 + i * 8, 12 + (FADER_LEN - scaled_level), 6,
                            scaled_level + 1, WHITE);

#endif
    }
  }
  enc->cur = 64 + dir;
  enc->old = 64;
}
void encoder_filtf_handle(Encoder *enc) {
  mixer_page.adjust_param(enc, MODEL_FLTF);
}
void encoder_filtw_handle(Encoder *enc) {
  mixer_page.adjust_param(enc, MODEL_FLTW);
}
void encoder_filtq_handle(Encoder *enc) {
  mixer_page.adjust_param(enc, MODEL_FLTQ);
}
void encoder_lastparam_handle(Encoder *enc) {
  mixer_page.adjust_param(enc, MD.midi_events.last_md_param);
}

void MixerPage::adjust_param(Encoder *enc, uint8_t param) {

  int dir = enc->getValue() - enc->old;
  int newval;

  for (int i = 0; i < 16; i++) {
    if (note_interface.notes[i] == 1) {
      newval = MD.kit.params[i][param] + dir;
      if (newval < 0) {
        newval = 0;
      }
      if (newval > 127) {
        newval = 127;
      }
      for (uint8_t value = MD.kit.params[i][param]; value < newval; value++) {
        USE_LOCK();
        SET_LOCK();
        MD.setTrackParam(i, param, value);
        MD.kit.params[i][param] = value;
        CLEAR_LOCK();
      }
      for (uint8_t value = MD.kit.params[i][param]; value > newval; value--) {
        USE_LOCK();
        SET_LOCK();
        MD.setTrackParam(i, param, value);
        MD.kit.params[i][param] = value;
        CLEAR_LOCK();
      }
      USE_LOCK();
      SET_LOCK();
      MD.setTrackParam(i, param, newval);
      MD.kit.params[i][param] = newval;
      CLEAR_LOCK();
    }
  }
  enc->cur = 64 + dir;
  enc->old = 64;
}

void MixerPage::display() {
  if (!classic_display) {
    //  oled_display.clearDisplay();
  }
#ifndef OLED_DISPLAY
  note_interface.draw_notes(0);
  if (!classic_display) {
    LCD.goLine(0);
    LCD.puts(GUI.lines[0].data);
  }

#endif
#ifdef OLED_DISPLAY
  // mute_page.draw_mutes(0);
#endif
  draw_levels();
#ifdef OLED_DISPLAY
  if (!classic_display) {
    oled_display.display();
  }
#endif
  uint8_t dec = MidiClock.get_tempo() / 10;
  for (uint8_t n = 0; n < 16; n++) {
    if (disp_levels[n] < dec) {
      disp_levels[n] = 0;
    } else {
      disp_levels[n] -= dec;
    }
  }
}
bool MixerPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {
    uint8_t mask = event->mask;
    uint8_t port = event->port;
    uint8_t device = midi_active_peering.get_device(port);

    uint8_t track = event->source - 128;

    if (track > 16) {
      return;
    }
    if (event->mask == EVENT_BUTTON_PRESSED) {
#ifdef OLED_DISPLAY

      if (note_interface.notes[track] > 0) {

        oled_display.fillRect(0 + track * 8, 2, 6, 6, WHITE);
      }

#endif

      return true;
    }

    if (event->mask == EVENT_BUTTON_RELEASED) {
      note_interface.draw_notes(0);
#ifdef OLED_DISPLAY
      uint8_t i = track;
      uint8_t scaled_level =
          (uint8_t)(((float)MD.kit.levels[i] / (float)127) * FADER_LEN);

      oled_display.fillRect(0 + i * 8, 12, 6, FADER_LEN, BLACK);
      oled_display.drawRect(0 + i * 8, 12 + (FADER_LEN - scaled_level), 6,
                            scaled_level + 1, WHITE);

#endif

      if (note_interface.notes_all_off_md()) {
        if (BUTTON_DOWN(Buttons.BUTTON4)) {
          route_page.toggle_routes_batch();
        }
        if (BUTTON_DOWN(Buttons.BUTTON1)) {
          route_page.toggle_routes_batch(true);
        }
        note_interface.init_notes();
#ifdef OLED_DISPLAY
       draw_routes(0);
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
       draw_routes(0);
#endif
  }
*/
  if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
    route_page.update_globals();
    md_exploit.off();
    md_exploit.on();
    GUI.setPage(&page_select_page);
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
    for (uint8_t i = 0; i < 16; i++) {
      if (note_interface.notes[i] == 1) {
        for (uint8_t c = 0; c < 3; c++) {
          if (MD.kit.params[i][MODEL_FLTF + c] != params[i][c]) {
            USE_LOCK();
            SET_LOCK();
            MD.setTrackParam(i, MODEL_FLTF + c, params[i][c]);
            MD.kit.params[i][MODEL_FLTF + c] = params[i][c];
            CLEAR_LOCK();
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
    if (note_interface.notes_count() == 0) {
       route_page.update_globals();
       GUI.setPage(&grid_page);
    }
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

  state = true;
}

void MixerMidiEvents::remove_callbacks() {
  if (!state) {
    return;
  }

  DEBUG_PRINTLN("remove calblacks");
  Midi.removeOnNoteOnCallback(
      this, (midi_callback_ptr_t)&MixerMidiEvents::onNoteOnCallback_Midi);
  Midi.removeOnNoteOffCallback(
      this, (midi_callback_ptr_t)&MixerMidiEvents::onNoteOffCallback_Midi);

  state = false;
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
