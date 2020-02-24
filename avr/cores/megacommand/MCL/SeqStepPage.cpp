#include "MCL.h"
#include "SeqStepPage.h"

#define MIDI_OMNI_MODE 17
#define NUM_KEYS 24

void SeqStepPage::setup() { SeqPage::setup(); }
void SeqStepPage::config() {
  seq_param3.cur = mcl_seq.md_tracks[last_md_track].length;
  tuning_t const *tuning = MD.getModelTuning(MD.kit.models[last_md_track]);
  seq_param4.cur = 0;
  seq_param4.old = 0;
  if (tuning) {
    seq_param4.max = tuning->len - 1;
  } else {
    seq_param4.max = 1;
  }
  // config info labels
  const char *str1 = getMachineNameShort(MD.kit.models[last_md_track], 1);
  const char *str2 = getMachineNameShort(MD.kit.models[last_md_track], 2);

  constexpr uint8_t len1 = sizeof(info1);

  char buf[len1] = {'\0'};
  m_strncpy_p(buf, str1, len1);
  strncpy(info1, buf, len1);
  strncat(info1, ">", len1);
  m_strncpy_p(buf, str2, len1);
  strncat(info1, buf, len1);
  strcpy(info2, "STEP");

  // config menu
  config_as_trackedit();
}

void SeqStepPage::init() {
  DEBUG_PRINT_FN();
  DEBUG_PRINTLN("init seqstep");
  SeqPage::init();

  SeqPage::midi_device = midi_active_peering.get_device(UART1_PORT);

  seq_param1.max = 14;
  seq_param2.max = 23;
  seq_param2.min = 1;
  seq_param2.cur = 12;
  seq_param2.old = 12;
  seq_param1.cur = 0;
  seq_param3.max = 64;
  midi_events.setup_callbacks();
  curpage = SEQ_STEP_PAGE;
  trig_interface.on();
  config();
  note_interface.state = true;
}

void SeqStepPage::cleanup() {
  midi_events.remove_callbacks();
  SeqPage::cleanup();
  if (MidiClock.state != 2) {
    MD.setTrackParam(last_md_track, 0, MD.kit.params[last_md_track][0]);
  }
}

#ifndef OLED_DISPLAY
void SeqStepPage::display() {
  GUI.setLine(GUI.LINE1);
  GUI.put_string_at(0, "                ");
  GUI.put_value_at1(15, page_select + 1);
  const char *str1 = getMachineNameShort(MD.kit.models[last_md_track], 1);
  const char *str2 = getMachineNameShort(MD.kit.models[last_md_track], 2);

  char c[3] = "--";

  if (seq_param1.getValue() == 0) {
    GUI.put_string_at(0, "L1");

  } else if (seq_param1.getValue() <= 8) {
    GUI.put_string_at(0, "L");

    GUI.put_value_at1(1, seq_param1.getValue());

  } else if (seq_param1.getValue() <= 13) {
    GUI.put_string_at(0, "P");
    uint8_t prob[5] = {1, 2, 5, 7, 9};
    GUI.put_value_at1(1, prob[seq_param1.getValue() - 9]);
  }

  else if (seq_param1.getValue() == 14) {
    GUI.put_string_at(0, "1S");
  }

  if (seq_param2.getValue() == 0) {
    GUI.put_string_at(2, "--");
  } else if ((seq_param2.getValue() < 12) && (seq_param2.getValue() != 0)) {
    GUI.put_string_at(2, "-");
    GUI.put_value_at2(3, 12 - seq_param2.getValue());

  } else {
    GUI.put_string_at(2, "+");
    GUI.put_value_at2(3, seq_param2.getValue() - 12);
  }

  if (show_pitch) {
    tuning_t const *tuning = MD.getModelTuning(MD.kit.models[last_md_track]);
    if (tuning != NULL) {
      if (seq_param4.cur == 0) {
        GUI.put_string_at(10, "--");
      } else {
        uint8_t base = tuning->base;
        uint8_t notenum = seq_param4.cur + base;
        MusicalNotes number_to_note;
        uint8_t oct = (notenum / 12) - 1;
        uint8_t note = notenum - 12 * (notenum / 12);
        GUI.put_string_at(10, number_to_note.notes_upper[note]);
        GUI.put_value_at1(12, oct);
      }
    }
  } else {
    GUI.put_p_string_at(10, str1);
    GUI.put_p_string_at(12, str2);
  }
  GUI.put_value_at(6, seq_param3.getValue());
  GUI.put_value_at1(15, page_select + 1);
  draw_pattern_mask((page_select * 16), DEVICE_MD);

  SeqPage::display();
}
#else
void SeqStepPage::display() {
  oled_display.clearDisplay();
  auto *oldfont = oled_display.getFont();
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

  strcpy(K, "--");
  K[3] = '\0';
  if (seq_param2.getValue() == 0) {
  } else if ((seq_param2.getValue() < 12) && (seq_param2.getValue() != 0)) {
    itoa(12 - seq_param2.getValue(), K + 1, 10);
  } else {
    K[0] = '+';
    itoa(seq_param2.getValue() - 12, K + 1, 10);
  }
  draw_knob(1, "UTIM", K);

  itoa(seq_param3.getValue(), K, 10);
  draw_knob(2, "LEN", K);

  tuning_t const *tuning = MD.getModelTuning(MD.kit.models[last_md_track]);
  if (show_pitch) {
    if (tuning != NULL) {
      strcpy(K, "--");
      if (seq_param4.cur != 0) {
        uint8_t base = tuning->base;
        uint8_t notenum = seq_param4.cur + base;
        MusicalNotes number_to_note;
        uint8_t oct = notenum / 12 - 1;
        uint8_t note = notenum - 12 * (notenum / 12);
        strcpy(K, number_to_note.notes_upper[note]);
        K[2] = oct + '0';
        K[3] = 0;
      }
      draw_knob(3, "PTC", K);
    }
  }
  if (mcl_gui.show_encoder_value(&seq_param4) && (seq_param4.cur > 0) &&
      (note_interface.notes_count_on() > 0) && (!show_seq_menu) &&
      (!show_step_menu) && (tuning != NULL)) {
    uint64_t note_mask = 0;
    uint8_t note = seq_param4.cur + tuning->base;
    SET_BIT64(note_mask, note - 24 * (note / 24));
    mcl_gui.draw_keyboard(32, 23, 6, 9, NUM_KEYS, note_mask);
    SeqPage::display();
  }

  else {
    draw_lock_mask((page_select * 16), DEVICE_MD);
    draw_pattern_mask((page_select * 16), DEVICE_MD);
    SeqPage::display();
    if (mcl_gui.show_encoder_value(&seq_param2) &&
        (note_interface.notes_count_on() > 0) && (!show_seq_menu) &&
        (!show_step_menu)) {

      mcl_gui.draw_microtiming(mcl_seq.md_tracks[last_md_track].resolution,
                               seq_param2.cur);
    }
  }
  oled_display.display();
  oled_display.setFont(oldfont);
}
#endif

void SeqStepPage::loop() {
  SeqPage::loop();

  if (seq_param1.hasChanged() || seq_param2.hasChanged() ||
      seq_param4.hasChanged()) {
    tuning_t const *tuning = MD.getModelTuning(MD.kit.models[last_md_track]);

    MDSeqTrack &active_track = mcl_seq.md_tracks[last_md_track];

    for (uint8_t n = 0; n < 16; n++) {

      if (note_interface.notes[n] == 1) {
        uint8_t step = n + (page_select * 16);
        if (step < active_track.length) {

          uint8_t utiming = (seq_param2.cur + 0);
          uint8_t condition = seq_param1.cur;

          //  timing = 3;
          // condition = 3;
          active_track.conditional[step] = condition;
          active_track.timing[step] = utiming; // upper

          if (!IS_BIT_SET64(active_track.pattern_mask, step)) {
            SET_BIT64(active_track.pattern_mask, step);
          }
          if ((seq_param4.cur > 0) && (last_md_track < NUM_MD_TRACKS) &&
              (tuning != NULL)) {
            uint8_t base = tuning->base;
            uint8_t note_num = seq_param4.cur;
            uint8_t machine_pitch = pgm_read_byte(&tuning->tuning[note_num]);
            active_track.set_track_pitch(step, machine_pitch);
          }
        }
      }
    }
    seq_param1.old = seq_param1.cur;
    seq_param2.old = seq_param2.cur;
    seq_param4.old = seq_param4.cur;
  }
}

bool SeqStepPage::handleEvent(gui_event_t *event) {

  if (SeqPage::handleEvent(event)) {
    return true;
  }

  MDSeqTrack &active_track = mcl_seq.md_tracks[last_md_track];

  if (note_interface.is_event(event)) {
    uint8_t mask = event->mask;
    uint8_t port = event->port;
    uint8_t device = midi_active_peering.get_device(port);

    uint8_t trackid = event->source - 128;
    uint8_t step = trackid + (page_select * 16);

    if (event->mask == EVENT_BUTTON_PRESSED) {
      if (device == DEVICE_A4) {
        //        GUI.setPage(&seq_extstep_page);

        return true;
      }
      mcl_seq.midi_events.update_params = false;
      MD.midi_events.disable_live_kit_update();

      if (MidiClock.state != 2) {
        active_track.send_parameter_locks(step);
      }
      show_pitch = true;

      if (step >= active_track.length) {
        return true;
      }

      seq_param2.max = 23;
      int8_t utiming = active_track.timing[step];         // upper
      uint8_t condition = active_track.conditional[step]; // lower
      uint8_t pitch = active_track.get_track_lock(step, 0) - 1;
      // Cond
      seq_param1.cur = condition;
      uint8_t note_num = 255;

      // SET_BIT64(active_track.pattern_mask, step);
      tuning_t const *tuning = MD.getModelTuning(MD.kit.models[last_md_track]);
      if (tuning) {
        for (uint8_t i = 0; i < tuning->len && note_num == 255; i++) {
          uint8_t ccStored = pgm_read_byte(&tuning->tuning[i]);
          if (ccStored >= pitch) {
            note_num = i;
          }
        }
        if (note_num == 255) {
          seq_param4.cur = 0;
        } else {
          seq_param4.cur = note_num;
        }
        seq_param4.old = seq_param4.cur;
      }
      // Micro
      if (utiming == 0) {
        utiming = 12;
      }
      seq_param2.cur = utiming;
      seq_param2.old = utiming;
    }

    if (event->mask == EVENT_BUTTON_RELEASED) {

      if (device == DEVICE_A4) {
        // GUI.setPage(&seq_extstep_page);
        return true;
      }

      if (last_md_track < 15) {
        show_pitch = false;
      }
      if (step >= active_track.length) {
        return true;
      }

      /*      uint8_t utiming = (seq_param2.cur + 0);
            uint8_t condition = seq_param1.cur;

            uint8_t step = track + (page_select * 16);
            //  timing = 3;
            // condition = 3;
            active_track.conditional[step] = condition;
            active_track.timing[step] = utiming; // upper
            if ((seq_param4.cur > 0) && (last_md_track < 15)) {
              tuning_t const *tuning =
                  MD.getModelTuning(MD.kit.models[last_md_track]);
              if (tuning != NULL) {
                uint8_t base = tuning->base;
                uint8_t note_num = seq_param4.cur;
                uint8_t machine_pitch =
         pgm_read_byte(&tuning->tuning[note_num]);
                active_track.set_track_pitch(step,
         machine_pitch);
              }
            }*/
      //   conditional_timing[cur_col][(track + (seq_param2.cur * 16))] =
      //   condition; //lower
      if (note_interface.notes_all_off_md()) {
        mcl_gui.init_encoders_used_clock();
        active_track.reset_params();
        mcl_seq.midi_events.update_params = true;
        MD.midi_events.enable_live_kit_update();
      }
      if (!IS_BIT_SET64(active_track.pattern_mask, step)) {
        uint8_t utiming = (seq_param2.cur + 0);
        uint8_t condition = seq_param1.cur;

        DEBUG_PRINTLN("settting");

        active_track.conditional[step] = condition;
        active_track.timing[step] = utiming; // upper
        // active_track.clear_step_locks(step);
        CLEAR_BIT64(active_track.oneshot_mask, step);
        SET_BIT64(active_track.pattern_mask, step);
      } else {
        DEBUG_PRINTLN("clear step");
        /*
        DEBUG_DUMP(step);
        Serial.println();
#       for (uint8_t n = 0; n < 64; n++) {
        if (IS_BIT_SET64(active_track.pattern_mask, n)) { Serial.print(1); }
        else { Serial.print(0); }
        }
        Serial.println();
        */
        if (clock_diff(note_interface.note_hold, slowclock) < TRIG_HOLD_TIME) {
          CLEAR_BIT64(active_track.pattern_mask, step);
          active_track.conditional[step] = 0;
          active_track.timing[step] = 12; // upper
        }
      }
      // Cond
      // seq_param4.cur = condition;
      // Microƒ
      // encoders[4]->cur = timing;
      // draw_notes(1);
      return true;
    }
    return true;
  } // end TI events

  if (EVENT_PRESSED(event, Buttons.ENCODER1)) {
    //    if (note_interface.notes_all_off() || (note_interface.notes_count() ==
    //    0)) {
    //      GUI.setPage(&grid_page);
    //    }
    return true;
  }

#ifdef EXT_TRACKS
  if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
    GUI.setPage(&seq_extstep_page);
    return true;
  }
#endif

  return false;
}

void SeqStepMidiEvents::onNoteOnCallback_Midi2(uint8_t *msg) {

  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);

  tuning_t const *tuning = MD.getModelTuning(MD.kit.models[last_md_track]);
  uint8_t note_num = msg[1] - tuning->base;
  if (note_num < tuning->len) {
    uint8_t machine_pitch = pgm_read_byte(&tuning->tuning[note_num]);
    if (MidiClock.state != 2) {
      MD.setTrackParam(last_md_track, 0, machine_pitch);
      MD.triggerTrack(last_md_track, 127);
    }
    seq_param4.cur = note_num;
  }
}

void SeqStepMidiEvents::onControlChangeCallback_Midi(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t param = msg[1];
  uint8_t value = msg[2];
  uint8_t track;
  uint8_t track_param;

  MD.parseCC(channel, param, &track, &track_param);
  MDSeqTrack &active_track = mcl_seq.md_tracks[last_md_track];
  uint8_t step;
  if (track_param > 23) {
    return;
  }

  for (int i = 0; i < 16; i++) {
    if ((note_interface.notes[i] == 1)) {
      step = i + (SeqPage::page_select * 16);
      active_track.set_track_locks(step, track_param, value);
      if (!IS_BIT_SET64(active_track.pattern_mask, step)) {
        SET_BIT64(active_track.pattern_mask, step);
        SET_BIT64(active_track.lock_mask, step);
      }
    }
  }
}

void SeqStepMidiEvents::setup_callbacks() {
  if (state) {
    return;
  }

  Midi.addOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&SeqStepMidiEvents::onControlChangeCallback_Midi);
  Midi2.addOnNoteOnCallback(
      this, (midi_callback_ptr_t)&SeqStepMidiEvents::onNoteOnCallback_Midi2);

  state = true;
}

void SeqStepMidiEvents::remove_callbacks() {

  if (!state) {
    return;
  }
  Midi.removeOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&SeqStepMidiEvents::onControlChangeCallback_Midi);
  Midi2.removeOnNoteOnCallback(
      this, (midi_callback_ptr_t)&SeqStepMidiEvents::onNoteOnCallback_Midi2);
  state = false;
}
