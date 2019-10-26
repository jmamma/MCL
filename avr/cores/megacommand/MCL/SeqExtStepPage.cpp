#include "SeqExtStepPage.h"
#include "MCL.h"

void SeqExtStepPage::setup() { SeqPage::setup(); }
void SeqExtStepPage::config() {
#ifdef EXT_TRACKS
  encoders[2]->cur = mcl_seq.ext_tracks[last_ext_track].length;
#endif
  // config info labels
  constexpr uint8_t len1 = sizeof(info1);

  char buf[len1] = {'\0'};

  strcpy(info2, "NOTE");
}
void SeqExtStepPage::config_encoders() {
#ifdef EXT_TRACKS
  if (mcl_seq.ext_tracks[last_ext_track].resolution == 1) {
    ((MCLEncoder *)encoders[1])->cur = 6;
    ((MCLEncoder *)encoders[1])->max = 11;
  } else {
    ((MCLEncoder *)encoders[1])->cur = 12;
    ((MCLEncoder *)encoders[1])->max = 23;
  }
  ((MCLEncoder *)encoders[2])->max = 128;
  config();
  SeqPage::midi_device = midi_active_peering.get_device(UART2_PORT);
#endif
}

void SeqExtStepPage::init() {
  DEBUG_PRINTLN("seq extstep init");
  curpage = SEQ_EXTSTEP_PAGE;
  md_exploit.on();
  note_interface.state = true;
  config_encoders();
  midi_events.setup_callbacks();
}

void SeqExtStepPage::cleanup() {
  SeqPage::cleanup();
  midi_events.remove_callbacks();
}

#ifndef OLED_DISPLAY
void SeqExtStepPage::display() {

  GUI.setLine(GUI.LINE1);
  GUI.put_string_at(0, "                ");

  char c[3] = "--";

  if (encoders[0]->getValue() == 0) {
    GUI.put_string_at(0, "L1");

  } else if (encoders[0]->getValue() <= 8) {
    GUI.put_string_at(0, "L");

    GUI.put_value_at1(1, encoders[0]->getValue());

  } else {
    GUI.put_string_at(0, "P");
    uint8_t prob[5] = {1, 2, 5, 7, 9};
    GUI.put_value_at1(1, prob[encoders[0]->getValue() - 9]);
  }

  // Cond
  //    GUI.put_value_at2(0, encoders[1]->getValue());
  // Pos
  // 0  1   2  3  4  5  6  7  8  9  10  11
  //  -5  -4 -3 -2 -1 0
#ifdef EXT_TRACKS
  if (mcl_seq.ext_tracks[last_ext_track].resolution == 1) {
    if (encoders[1]->getValue() == 0) {
      GUI.put_string_at(2, "--");
    } else if ((encoders[1]->getValue() < 6) &&
               (encoders[1]->getValue() != 0)) {
      GUI.put_string_at(2, "-");
      GUI.put_value_at1(3, encoders[1]->getValue() - 6);
    } else {
      GUI.put_string_at(2, "+");
      GUI.put_value_at1(3, encoders[1]->getValue() - 6);
    }
  } else {
    if (encoders[1]->getValue() == 0) {
      GUI.put_string_at(2, "--");
    } else if ((encoders[1]->getValue() < 12) &&
               (encoders[1]->getValue() != 0)) {
      GUI.put_string_at(2, "-");
      GUI.put_value_at1(3, 12 - encoders[1]->getValue());

    } else {
      GUI.put_string_at(2, "+");
      GUI.put_value_at1(3, encoders[1]->getValue() - 12);
    }
  }

  MusicalNotes number_to_note;
  uint8_t notenum;
  uint8_t notes_held = 0;
  uint8_t i;
  for (i = 0; i < 16; i++) {
    if (note_interface.notes[i] == 1) {
      notes_held += 1;
    }
  }

  if (notes_held > 0) {
    for (i = 0; i < 4; i++) {

      notenum = mcl_seq.ext_tracks[last_ext_track]
                    .notes[i][note_interface.last_note + page_select * 16];
      if (notenum != 0) {
        notenum = notenum - 1;
        uint8_t oct = notenum / 12;
        uint8_t note = notenum - 12 * (notenum / 12);
        if (mcl_seq.ext_tracks[last_ext_track]
                .notes[i][note_interface.last_note + page_select * 16] > 0) {

          GUI.put_string_at(4 + i * 3, number_to_note.notes_upper[note]);
          GUI.put_value_at1(4 + i * 3 + 2, oct);

        } else {
          GUI.put_string_at(4 + i * 3, number_to_note.notes_lower[note]);
          GUI.put_value_at1(4 + i * 3 + 2, oct);
        }
      }
    }
  } else {
    GUI.put_value_at1(15, page_select + 1);
    GUI.put_value_at(6, encoders[2]->getValue());

    GUI.put_value_at(6, (encoders[2]->getValue() /
                         (2 / mcl_seq.ext_tracks[last_ext_track].resolution)));
    if (Analog4.connected) {
      GUI.put_string_at(10, "A4T");
    } else {
      GUI.put_string_at(10, "MID");
    }
    GUI.put_value_at1(13, last_ext_track + 1);
  }
  draw_pattern_mask((page_select * 16), DEVICE_A4);
#endif
  SeqPage::display();
}
#else
void SeqExtStepPage::display() {
  SeqPage::display();

  draw_knob_frame();

  char K[4];
  if (seq_param1.getValue() == 0) {
    strcpy(K, "L1");
  } else if (seq_param1.getValue() <= 8) {
    strcpy(K, "L ");
    K[1] = seq_param1.getValue() + '0';
  } else if (seq_param1.getValue() <= 13) {
    strcpy(K, "P ");
    uint8_t prob[5] = {1, 2, 5, 7, 9};
    K[1] = prob[seq_param1.getValue() - 9] + '0';
  } else if (seq_param1.getValue() == 14) {
    strcpy(K, "1S");
  }
  draw_knob(0, "COND", K);

#ifdef EXT_TRACKS
  auto &active_track = mcl_seq.ext_tracks[last_ext_track];
  strcpy(K, "--");
  K[3] = '\0';
  if (active_track.resolution == 1) {
    if (encoders[1]->getValue() == 0) {
    } else if ((encoders[1]->getValue() < 6) &&
               (encoders[1]->getValue() != 0)) {
      itoa(6 - encoders[1]->getValue(), K + 1, 10);
    } else {
      K[0] = '+';
      itoa(encoders[1]->getValue() - 6, K + 1, 10);
    }
  } else {
    if (encoders[1]->getValue() == 0) {
    } else if ((encoders[1]->getValue() < 12) &&
               (encoders[1]->getValue() != 0)) {
      itoa(12 - encoders[1]->getValue(), K + 1, 10);

    } else {
      K[0] = '+';
      itoa(encoders[1]->getValue() - 12, K + 1, 10);
    }
  }
  draw_knob(1, "UTIM", K);

  MusicalNotes number_to_note;
  uint8_t note;
  uint8_t notes_held = 0;
  uint8_t i;
  for (i = 0; i < 16; i++) {
    if (note_interface.notes[i] == 1) {
      notes_held += 1;
    }
  }

  itoa(encoders[2]->getValue() / (2 / active_track.resolution), K, 10);
  draw_knob(2, "LEN", K);

  if (notes_held > 0) {
    uint8_t x = knob_x0 + knob_w * 3 + 4;
    auto *oldfont = oled_display.getFont();
    oled_display.setFont(&TomThumb);
    for (i = 0; i < 4; i++) {
      oled_display.setCursor(x, i * 5 + 5);
      note = active_track.notes[i][note_interface.last_note + page_select * 16];
      if (note != 0) {
        note = note - 1;
        uint8_t oct = note / 12;
        uint8_t note = note - 12 * (note / 12);
        if (active_track.notes[i][note_interface.last_note + page_select * 16] >
            0) {
          oled_display.print(number_to_note.notes_upper[note]);
        } else {
          oled_display.print(number_to_note.notes_lower[note]);
        }
        oled_display.print(oct);
      }
    }
    oled_display.setFont(oldfont);
  }

  draw_pattern_mask(page_select * 16, DEVICE_A4);
  oled_display.display();
#endif
}
#endif

bool SeqExtStepPage::handleEvent(gui_event_t *event) {
  if (SeqPage::handleEvent(event)) {
    return true;
  }
  auto &active_track = mcl_seq.ext_tracks[last_ext_track];
#ifdef EXT_TRACKS
  if (note_interface.is_event(event)) {
    uint8_t mask = event->mask;
    uint8_t port = event->port;
    uint8_t device = midi_active_peering.get_device(port);
    uint8_t track = event->source - 128;

    if (device == DEVICE_A4) {
      track -= 16;
    }

    if (mask == EVENT_BUTTON_PRESSED) {
      DEBUG_PRINTLN(track);
      if (device == DEVICE_MD) {

        if ((track + (page_select * 16)) >= active_track.length) {
          DEBUG_PRINTLN("setting to 0");
          DEBUG_PRINTLN(last_ext_track);
          DEBUG_PRINTLN(page_select);
          note_interface.notes[track] = 0;
          return true;
        }

        int8_t utiming =
            active_track.timing[(track + (page_select * 16))]; // upper
        uint8_t condition =
            active_track.conditional[(track + (page_select * 16))]; // lower
        encoders[0]->cur = condition;
        // Micro
        if (utiming == 0) {
          if (active_track.resolution == 1) {
            utiming = 6;
            ((MCLEncoder *)encoders[1])->max = 11;
          } else {
            ((MCLEncoder *)encoders[1])->max = 23;
            utiming = 12;
          }
        }
        encoders[1]->cur = utiming;

        note_interface.last_note = track;
      }
    }
    if (mask == EVENT_BUTTON_RELEASED) {
      if (device == DEVICE_MD) {

        uint8_t utiming = (encoders[1]->cur + 0);
        uint8_t condition = encoders[0]->cur;
        if ((track + (page_select * 16)) >= active_track.length) {
          return true;
        }

        //  timing = 3;
        // condition = 3;
        if (clock_diff(note_interface.note_hold, slowclock) < TRIG_HOLD_TIME) {
          for (uint8_t c = 0; c < 4; c++) {
            if (active_track.notes[c][track + page_select * 16] > 0) {
              MidiUart2.sendNoteOff(
                  last_ext_track,
                  abs(active_track.notes[c][track + page_select * 16]) - 1, 0);
            }
            active_track.notes[c][track + page_select * 16] = 0;
          }
          active_track.timing[(track + (page_select * 16))] = 0;
          active_track.conditional[(track + (page_select * 16))] = 0;
        }

        else {
          active_track.timing[(track + (page_select * 16))] = utiming; // upper
          active_track.conditional[(track + (page_select * 16))] =
              condition; // upper
        }
      }
      return true;
    }
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.ENCODER1)) {
    if (note_interface.notes_all_off() || (note_interface.notes_count() == 0)) {
      md_exploit.off();
      GUI.setPage(&grid_page);
    }
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON3) && BUTTON_DOWN(Buttons.BUTTON2)) {
    if (active_track.resolution == 1) {
      active_track.resolution = 2;
      init();

    } else {
      active_track.resolution = 1;
      init();
    }

    return true;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
    GUI.setPage(&seq_step_page);
    return true;
  }
  if (EVENT_RELEASED(event, Buttons.BUTTON4)) {
    active_track.clear_track();
    return true;
  }
  if ((EVENT_PRESSED(event, Buttons.BUTTON3) && BUTTON_DOWN(Buttons.BUTTON4)) ||
      (EVENT_PRESSED(event, Buttons.BUTTON4) && BUTTON_DOWN(Buttons.BUTTON3))) {
    for (uint8_t n = 0; n < mcl_seq.num_ext_tracks; n++) {
      active_track.clear_track();
    }
    return true;
  }
#endif
  return false;
}

void SeqExtStepMidiEvents::onNoteOnCallback_Midi2(uint8_t *msg) {
  // Step edit for ExtSeq
  // For each incoming note, check to see if note interface has any steps
  // selected For selected steps record notes.
#ifdef EXT_TRACKS
  DEBUG_PRINTLN("note on midi2 ext");
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  if (last_ext_track < mcl_seq.num_ext_tracks) {
    last_ext_track = channel;
    seq_extstep_page.config_encoders();

    if (MidiClock.state != 2) {
      mcl_seq.ext_tracks[channel].note_on(msg[1]);
    }
    for (uint8_t i = 0; i < 16; i++) {
      if (note_interface.notes[i] == 1) {
        mcl_seq.ext_tracks[channel].set_ext_track_step(
            seq_extstep_page.page_select * 16 + i, msg[1], msg[2]);
      }
    }
  }
#endif
}

void SeqExtStepMidiEvents::onNoteOffCallback_Midi2(uint8_t *msg) {
#ifdef EXT_TRACKS
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  if (MidiClock.state != 2) {
    mcl_seq.ext_tracks[channel].note_off(msg[1]);
  }
#endif
}

void SeqExtStepMidiEvents::setup_callbacks() {
  if (state) {
    return;
  }
  Midi2.addOnNoteOnCallback(
      this, (midi_callback_ptr_t)&SeqExtStepMidiEvents::onNoteOnCallback_Midi2);
  Midi2.addOnNoteOffCallback(
      this,
      (midi_callback_ptr_t)&SeqExtStepMidiEvents::onNoteOffCallback_Midi2);

  state = true;
}

void SeqExtStepMidiEvents::remove_callbacks() {

  if (!state) {
    return;
  }
  Midi2.removeOnNoteOnCallback(
      this, (midi_callback_ptr_t)&SeqExtStepMidiEvents::onNoteOnCallback_Midi2);
  Midi2.removeOnNoteOffCallback(
      this,
      (midi_callback_ptr_t)&SeqExtStepMidiEvents::onNoteOffCallback_Midi2);
  state = false;
}
