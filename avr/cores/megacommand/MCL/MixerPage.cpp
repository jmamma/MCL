#include "MCL.h"
#include "MixerPage.h"
#define FADER_LEN 16

void MixerPage::setup() {
  ((MCLEncoder *)encoders[0])->handler = encoder_level_handle;
  ((MCLEncoder *)encoders[3])->handler = encoder_level_handle;
  create_chars_mixer();
#ifdef OLED_DISPLAY
  classic_display = false;
  oled_display.clearDisplay();
#endif
}
void MixerPage::init() {
  level_pressmode = 0;
  mixer_param1.cur = 60;
  bool switch_tracks = false;
  note_interface.state = true;
  midi_events.setup_callbacks();
#ifdef OLED_DISPLAY
  oled_display.clearDisplay();
  mute_page.draw_mutes(0);
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
  in_sysex = 1;
  MD.kit.levels[curtrack] = value;
  MD.setTrackParam(curtrack, 33, value);
  in_sysex = 0;
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
  MCLEncoder *mdEnc = (MCLEncoder *)enc;

  int dir = mdEnc->getValue() - mdEnc->old;
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
  mdEnc->cur = 64 + dir;
  mdEnc->old = 64;

  // draw_levels();
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
  if (!classic_display) {
    oled_display.display();
  }
  uint8_t dec = MidiClock.tempo / 10;
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
          mute_page.toggle_mutes_batch();
        }
        note_interface.init_notes();
#ifdef OLED_DISPLAY
        mute_page.draw_mutes(0);
#endif
      }
      return true;
    }
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON4)) {
    // mute_page.toggle_mutes_batch();
    // note_interface.init_notes();
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
    GUI.setPage(&mute_page);
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
    GUI.setPage(&page_select_page);
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.ENCODER1) ||
      EVENT_PRESSED(event, Buttons.ENCODER2) ||
      EVENT_PRESSED(event, Buttons.ENCODER3) ||
      EVENT_PRESSED(event, Buttons.ENCODER4)) {
    GUI.setPage(&grid_page);
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
