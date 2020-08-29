#include "MCL_impl.h"

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

  config_mask_info();
  config_encoders();
  // config menu
  config_as_trackedit();
}

void SeqStepPage::config_encoders() {
  uint8_t timing_mid = mcl_seq.md_tracks[last_md_track].get_timing_mid();
  seq_param2.cur = timing_mid;
  seq_param2.old = timing_mid;
  seq_param2.max = timing_mid * 2 - 1;
}

void SeqStepPage::init() {
  DEBUG_PRINT_FN();
  DEBUG_PRINTLN("init seqstep");
  SeqPage::init();
  seq_menu_page.menu.enable_entry(SEQ_MENU_MASK, true);
  SeqPage::midi_device = midi_active_peering.get_device(UART1_PORT);

  seq_param1.max = NUM_TRIG_CONDITIONS * 2;
  seq_param2.min = 1;
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
  uint8_t cond = seq_param1.getValue();
  if (cond > NUM_TRIG_CONDITITONS) { cond -= NUM_TRIG_CONDITIONS; }
  if (cond == 0) {
    GUI.put_string_at(0, "L1");

  } else if (cond <= 8) {
    GUI.put_string_at(0, "L");

    GUI.put_value_at1(1, cond);

  } else if (cond <= 13) {
    GUI.put_string_at(0, "P");
    uint8_t prob[5] = {1, 2, 5, 7, 9};
    GUI.put_value_at1(1, prob[cond - 9]);
  }

  else if (cond == 14) {
    GUI.put_string_at(0, "1S");
  }
  uint8_t timing_mid = mcl_seq.md_tracks[last_md_track].get_timing_mid();
  if (seq_param2.getValue() == 0) {
    GUI.put_string_at(2, "--");
  } else if ((seq_param2.getValue() < timing_mid) &&
             (seq_param2.getValue() != 0)) {
    GUI.put_string_at(2, "-");
    GUI.put_value_at2(3, timing_mid - seq_param2.getValue());

  } else {
    GUI.put_string_at(2, "+");
    GUI.put_value_at2(3, seq_param2.getValue() - timing_mid);
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
  draw_mask((page_select * 16), DEVICE_MD);

  SeqPage::display();
}
#else
void SeqStepPage::display() {
  oled_display.clearDisplay();
  auto *oldfont = oled_display.getFont();
  draw_knob_frame();

  uint8_t timing_mid = mcl_seq.md_tracks[last_md_track].get_timing_mid();

  draw_knob_conditional(seq_param1.getValue());
  draw_knob_timing(seq_param2.getValue(), timing_mid);

  char K[4];
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
    draw_mask((page_select * 16), DEVICE_MD);
    SeqPage::display();
    if (mcl_gui.show_encoder_value(&seq_param2) &&
        (note_interface.notes_count_on() > 0) && (!show_seq_menu) &&
        (!show_step_menu)) {

      mcl_gui.draw_microtiming(mcl_seq.md_tracks[last_md_track].speed, seq_param2.cur);
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
          uint8_t condition = translate_to_step_conditional(seq_param1.cur);

          active_track.conditional[step] = condition;
          active_track.timing[step] = utiming;
          uint64_t *mask = get_mask();

          if ((mask_type != MASK_SLIDE) && (mask_type != MASK_MUTE)) {
            if (!IS_BIT_SET64_P(mask, step)) {
              SET_BIT64_P(mask, step);
            }
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
    if (device == DEVICE_A4) {
      return true;
    }

    uint64_t *seq_mask = get_mask();

    if (event->mask == EVENT_BUTTON_PRESSED) {
      mcl_seq.midi_events.update_params = false;
      MD.midi_events.disable_live_kit_update();

      if (MidiClock.state != 2) {
        active_track.send_parameter_locks(step, true);
      }
      show_pitch = true;

      if (step >= active_track.length) {
        return true;
      }

      seq_param2.max =
          mcl_seq.md_tracks[last_md_track].get_timing_mid() * 2 - 1;
      int8_t utiming = active_track.timing[step];
      uint8_t pitch = active_track.get_track_lock(step, 0) - 1;
      // Cond
      uint8_t condition = translate_to_knob_conditional(active_track.conditional[step]);
      seq_param1.cur = condition;
      uint8_t note_num = 255;

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
      //      if (note_interface.notes_count_on() <= 1) {
      if (utiming == 0) {
        utiming = mcl_seq.md_tracks[last_md_track].get_timing_mid();
      }
      seq_param2.cur = utiming;
      seq_param2.old = utiming;
      if (!IS_BIT_SET64_P(seq_mask, step)) {
        active_track.conditional[step] = condition;
        active_track.timing[step] = utiming;
        CLEAR_BIT64(active_track.oneshot_mask, step);
        SET_BIT64_P(seq_mask, step);
        note_interface.ignoreNextEvent(trackid);
      }
      //      }
    }

    if (event->mask == EVENT_BUTTON_RELEASED) {

      if (last_md_track < 15) {
        show_pitch = false;
      }
      if (step >= active_track.length) {
        return true;
      }

      if (note_interface.notes_all_off_md()) {
        mcl_gui.init_encoders_used_clock();
        active_track.reset_params();
        mcl_seq.midi_events.update_params = true;
        MD.midi_events.enable_live_kit_update();
      }
      if (IS_BIT_SET64_P(seq_mask, step)) {
        DEBUG_PRINTLN("clear step");

        if (clock_diff(note_interface.note_hold, slowclock) < TRIG_HOLD_TIME) {
          CLEAR_BIT64_P(seq_mask, step);
          if (mask_type == MASK_PATTERN) {
            active_track.conditional[step] = 0;
            active_track.timing[step] = active_track.get_timing_mid();
          }
        }
      }
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
  uint8_t store_lock = 255;
  for (int i = 0; i < 16; i++) {
    if ((note_interface.notes[i] == 1)) {
      step = i + (SeqPage::page_select * 16);
      if (active_track.set_track_locks(step, track_param, value)) {
        store_lock = 0;
      } else {
        store_lock = 1;
      }
      uint64_t *mask = seq_step_page.get_mask();

      SET_BIT64(active_track.lock_mask, step);
      if (seq_step_page.mask_type == MASK_PATTERN) {
        uint8_t utiming = (seq_param2.cur + 0);
        uint8_t condition = seq_step_page.translate_to_step_conditional(seq_param1.cur);

        active_track.conditional[step] = condition;
        active_track.timing[step] = utiming;
        SET_BIT64(active_track.pattern_mask, step);

      } else {
       // SET_BIT64_P(mask, step);
      }
    }
  }
  if (store_lock == 0) {
    char str[5] = "--  ";
    char str2[4] = "-- ";
    PGM_P modelname = NULL;
    modelname = model_param_name(MD.kit.models[last_md_track], track_param);
    if (modelname != NULL) {
      m_strncpy_p(str, modelname, 3);
      if (strlen(str) == 2) {
        str[2] = ' ';
        str[3] = '\0';
      }
    }
    itoa(value, str2, 10);
#ifdef OLED_DISPLAY
    oled_display.textbox(str, str2);
#endif
  }
  if (store_lock == 1) {
#ifdef OLED_DISPLAY
    oled_display.textbox("LOCK PARAMS ", "FULL");
#endif
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
