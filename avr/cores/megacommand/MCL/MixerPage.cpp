#include "MCL.h"
#include "MixerPage.h"

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
  MD.currentKit = MD.getCurrentKit(CALLBACK_TIMEOUT);
  MD.saveCurrentKit(MD_KITBUF_POS);
  MD.getBlockingKit(MD_KITBUF_POS);
  DEBUG_PRINTLN("got blocking kit");
  level_pressmode = 0;
  mixer_param1.cur = 60;
  mixer_param2.cur = 60;
  md_exploit.on();
  note_interface.state = true;

  midi_events.setup_callbacks();
}
void MixerPage::cleanup() {
  md_exploit.off();
#ifdef OLED_DISPLAY
  oled_display.clearDisplay();
#endif
  note_interface.state = false;

  midi_events.remove_callbacks();
}

void MixerPage::set_level(int curtrack, int value) {
  in_sysex = 1;
  MD.setTrackParam(curtrack, 33, value);
  in_sysex = 0;
}

void MixerPage::loop() {
  uint8_t dec = MidiClock.tempo / 10;
  for (uint8_t n = 0; n < 16; n++) {
    if (disp_levels[n] > 0) {
      if (disp_levels[n] < dec) {
        disp_levels[n] = 0;
    }
    else {
      disp_levels[n] -= dec;
    }
    }
  }
}

void MixerPage::draw_levels() {
  GUI.setLine(GUI.LINE2);
  uint8_t scaled_level;
  uint8_t scaled_level2;
  char str[17] = "                ";
  for (int i = 0; i < 16; i++) {
//  if (MD.kit.levels[i] > 120) { scaled_level = 8; }
// else if (MD.kit.levels[i] < 4) { scaled_level = 0; }
#ifdef OLED_DISPLAY

    scaled_level = (uint8_t)(((float)MD.kit.levels[i] / (float)127) * 15);

    scaled_level2 = (uint8_t)(((float)disp_levels[i] / (float)127) * 15);

    if (note_interface.notes[i] == 1) {
      oled_display.fillRect(0 + i * 8, 6 + (15 - scaled_level), 6,
                            scaled_level + 1, WHITE);
      oled_display.drawLine( + i * 8, 25, 5 + (i*8), 25, WHITE);
    } else {
      oled_display.drawRect(0 + i * 8, 6 + (15 - scaled_level), 6,
                            scaled_level + 1, WHITE);
      oled_display.fillRect(0 + i * 8, 6 + (15 - scaled_level2), 6,
                            scaled_level2 + 1, WHITE);
    }
    /*
    if (scaled_level >= 7) {
      str[i] = (char)0xF8;
    }
    if (scaled_level == 0) {
      str[i] = (char)5;
    } else {
      str[i] = (char)(scaled_level + 6);
    }*/
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

      MD.kit.levels[i] = track_newval;
    }
  }
  mdEnc->cur = 64 + dir;
  mdEnc->old = 64;

  // draw_levels();
}

void MixerPage::display() {
  if (!classic_display) {
    oled_display.clearDisplay();
  }
#ifndef OLED_DISPLAY
  note_interface.draw_notes(0);
  if (!classic_display) {
    LCD.goLine(0);
    LCD.puts(GUI.lines[0].data);
  }
#endif
  draw_levels();
  if (!classic_display) {
    oled_display.display();
  }
}
bool MixerPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {
    uint8_t mask = event->mask;
    uint8_t port = event->port;
    uint8_t device = midi_active_peering.get_device(port);

    uint8_t track = event->source - 128;

    if (event->mask == EVENT_BUTTON_PRESSED) {
      return true;
    }

    if (event->mask == EVENT_BUTTON_RELEASED) {
      note_interface.draw_notes(0);

      return true;
    }
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
    GUI.setPage(&mute_page);
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON4)) {
    GUI.setPage(&cue_page);
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
void MixerMidiEvents::onNoteOffCallback_Midi(uint8_t *msg) {
}
