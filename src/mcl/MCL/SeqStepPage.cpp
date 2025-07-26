#include "MCL_impl.h"

#define MIDI_OMNI_MODE 17
#define NUM_KEYS 24
#define NOTE_C2 48

void SeqStepPage::setup() { SeqPage::setup(); }
void SeqStepPage::config() {
  bool is_midi_model = ((MD.kit.models[last_md_track] & 0xF0) == MID_01_MODEL);

  tuning_t const *tuning = MD.getKitModelTuning(last_md_track);
  if (tuning) {
    seq_param4.max = tuning->len - 1 + tuning->base;
  } else if (is_midi_model) {
    seq_param4.max = 127;
  }
  else {
    seq_param4.max = 1;
  }
  seq_param4.cur = 0;
  seq_param4.old = 0;
  // config info labels
  const char *str1 = getMDMachineNameShort(MD.kit.get_model(last_md_track), 1);
  const char *str2 = getMDMachineNameShort(MD.kit.get_model(last_md_track), 2);

  // 0-1
  copyMachineNameShort(str1, info1);
  info1[2] = '>';
  // 3-4
  copyMachineNameShort(str2, info1 + 3);
  // 5
  info1[5] = 0;

  config_mask_info(
      (mask_type ==
       MASK_PATTERN)); // false = transmit popup text for trig mode if required.
  config_encoders();
  // config menu
  config_as_trackedit();
}

void SeqStepPage::config_encoders() {
  if (show_seq_menu) {
    return;
  }
  uint8_t timing_mid = mcl_seq.md_tracks[last_md_track].get_timing_mid();
  seq_param3.cur = mcl_seq.md_tracks[last_md_track].length;
  seq_param3.old = seq_param3.cur;
  seq_param2.cur = timing_mid;
  seq_param2.old = timing_mid;
  seq_param2.max = timing_mid * 2 - 1;
}

void SeqStepPage::init() {
  DEBUG_PRINT_FN();
  DEBUG_PRINTLN(F("init seqstep"));
  SeqPage::init();

  pitch_param = 255;
  seq_menu_page.menu.enable_entry(SEQ_MENU_MASK, true);
  seq_menu_page.menu.enable_entry(SEQ_MENU_SOUND, true);
  seq_menu_page.menu.enable_entry(SEQ_MENU_LENGTH_MD, true);

  SeqPage::midi_device = midi_active_peering.get_device(UART1_PORT);

  midi_events.setup_callbacks();
  trig_interface.on();
  MD.set_rec_mode(1);
  trig_interface.send_md_leds(TRIGLED_STEPEDIT);
  check_and_set_page_select();

  auto &active_track = mcl_seq.md_tracks[last_md_track];
  MD.sync_seqtrack(active_track.length, active_track.speed,
                   active_track.step_count);

  trigled_mask = 0;
  locks_on_step_mask = 0;
  config();

  reset_on_release = false;
  ignore_release = 0;
  update_params_queue = false;
  seq_param1.max = NUM_TRIG_CONDITIONS * 2;
  seq_param2.min = 1;
  seq_param2.old = 12;
  seq_param1.cur = 0;
  seq_param3.max = 64;
  seq_param3.min = 1;
}

void SeqStepPage::disable_paramupdate_events() {
  mcl_seq.midi_events.update_params = false;
  MD.midi_events.disable_live_kit_update();
  seq_ptc_page.cc_link_enable = false;
  RAMPage::cc_link_enable = false;
}

void SeqStepPage::enable_paramupdate_events() {
  mcl_seq.midi_events.update_params = true;
  MD.midi_events.enable_live_kit_update();
  seq_ptc_page.cc_link_enable = true;
  RAMPage::cc_link_enable = true;
}

void SeqStepPage::cleanup() {
  midi_events.remove_callbacks();
  enable_paramupdate_events();
  SeqPage::cleanup();
  params_reset();
  MD.set_rec_mode(0);
  MD.popup_text(127, 2); // clear persistent trig mode popup
  if (MD.encoder_interface) {
    MD.deactivate_encoder_interface();
  }
}

void SeqStepPage::display() {

  oled_display.clearDisplay();
  draw_knob_frame();

  uint8_t timing_mid = mcl_seq.md_tracks[last_md_track].get_timing_mid();

  draw_knob_conditional(seq_param1.getValue());
  draw_knob_timing(seq_param2.getValue(), timing_mid);

  char K[4];
  mcl_gui.put_value_at(seq_param3.getValue(), K);
  bool is_poly = IS_BIT_SET16(mcl_cfg.poly_mask, last_md_track);
  if ((mcl_cfg.poly_mask > 0) && (is_poly)) {
    draw_knob(2, "PLEN", K);
  } else {
    draw_knob(2, "LEN", K);
  }
  tuning_t const *tuning = MD.getKitModelTuning(last_md_track);
  bool is_ptc = ((MD.kit.models[last_md_track] & 0xF0) == MID_01_MODEL) || tuning != NULL;
  if (show_pitch) {
    if (is_ptc) {
      strcpy(K, "--");
      if (seq_param4.cur != 0) {
        // uint8_t base = tuning->base;
        uint8_t note_num = seq_param4.cur;
        // + base;

        uint8_t note = note_num - (note_num / 12) * 12;
        uint8_t oct = note_num / 12;

        strcpy(K, number_to_note.notes_upper[note]);
        mcl_gui.put_value_at(oct, K + 2);
        K[3] = 0;
      }
      draw_knob(3, "PTC", K);
    }
  }

  if (mcl_gui.show_encoder_value(&seq_param4) && (seq_param4.cur > 0) &&
      (note_interface.notes_count_on() > 0) && (!show_seq_menu) &&
      (is_ptc) && !(recording)) {
    uint64_t note_mask[2] = {};
    uint8_t note = seq_param4.cur; // + tuning->base;
    SET_BIT64(note_mask, note);
    mcl_gui.draw_keyboard(32, 23, 6, 9, NUM_KEYS, note_mask);
    SeqPage::display();
  }

  else {
    uint8_t _midi_lock_tmp = MidiUartParent::handle_midi_lock;
    MidiUartParent::handle_midi_lock = 1;

    draw_lock_mask((page_select * 16), DEVICE_MD);
    draw_mask((page_select * 16), DEVICE_MD);

    MidiUartParent::handle_midi_lock = _midi_lock_tmp;

    SeqPage::display();
    if (md_micro && mcl_gui.show_encoder_value(&seq_param2) &&
        (note_interface.notes_count_on() > 0) && (!show_seq_menu) &&
        (!recording)) {

      mcl_gui.draw_microtiming(mcl_seq.md_tracks[last_md_track].speed,
                               seq_param2.cur);
    }
  }
  if (prepare) {
    page_select_page.md_prepare();
    prepare = false;
  }
}

void SeqStepPage::loop() {
  if (MD.global.extendedMode != 2) {
    mcl.setPage(GRID_PAGE);
    return;
  }
  SeqPage::loop();

  if (recording)
    return;

  MDSeqTrack &active_track = mcl_seq.md_tracks[last_md_track];

  uint8_t _midi_lock_tmp = MidiUartParent::handle_midi_lock;
  MidiUartParent::handle_midi_lock = 1;
  if (pitch_param != 255) {
    seq_param4.cur = pitch_param;
    pitch_param = 255;
  }

  if (seq_param1.hasChanged() || seq_param2.hasChanged() ||
      seq_param4.hasChanged()) {
    tuning_t const *tuning = MD.getKitModelTuning(last_md_track);

    for (uint8_t n = 0; n < 16; n++) {

      if (note_interface.is_note_on(n)) {
        uint8_t step = n + (page_select * 16);
        if (step < active_track.length) {

          uint8_t utiming = (seq_param2.cur + 0);
          bool cond_plock;
          uint8_t condition =
              translate_to_step_conditional(seq_param1.cur, &cond_plock);

          active_track.steps[step].cond_id = condition;
          active_track.steps[step].cond_plock = cond_plock;
          active_track.timing[step] = utiming;

          if (seq_param2.hasChanged()) {
            md_micro = true;
            MD.draw_microtiming(active_track.speed, utiming);
          }
          if (seq_param1.hasChanged()) {
            char str[4];
            if (seq_param1.getValue() > 0) {
              conditional_str(str, seq_param1.getValue(), true);
              MD.popup_text(str);
            }
          }
          switch (mask_type) {
          case MASK_LOCK:
            active_track.enable_step_locks(step);
            break;
          case MASK_PATTERN:
            active_track.steps[step].trig = true;
            break;
          }
          if (seq_param4.hasChanged() && (seq_param4.cur > 0) &&
              (last_md_track < NUM_MD_TRACKS) && (tuning != NULL || is_midi_model)) {
            uint8_t base = tuning->base;
            uint8_t note_num = seq_param4.cur;
            uint8_t machine_pitch =
                seq_ptc_page.get_machine_pitch(last_md_track, note_num);
            if (is_midi_model) { machine_pitch = note_num; }
            if (machine_pitch != MD.kit.params[last_md_track][0]) {
              active_track.set_track_pitch(step, machine_pitch);
              seq_step_page.encoders_used_clock[3] = slowclock; // indicate that encoder has changed.
            }
          }
        }
      }
    }
    seq_param1.old = seq_param1.cur;
    seq_param2.old = seq_param2.cur;
    seq_param4.old = seq_param4.cur;
  }

  if (update_params_queue && clock_diff(update_params_clock, slowclock) > 400) {
    enable_paramupdate_events();
    update_params_queue = false;
  }

  if (note_interface.notes_all_off_md() && !grid_page.bank_popup) {
    init_encoders_used_clock();
    // active_track.reset_params();
    MD.deactivate_encoder_interface();

    update_params_queue = true;
    update_params_clock = slowclock;

    note_interface.init_notes();
  }
  MidiUartParent::handle_midi_lock = _midi_lock_tmp;
}

void SeqStepPage::send_locks(uint8_t step) {
  MDSeqTrack &active_track = mcl_seq.md_tracks[last_md_track];
  uint8_t params[24];
  memset(params, 255, sizeof(params));
  bool ignore_locks_disabled = true;
  active_track.get_step_locks(step, params, ignore_locks_disabled);
  MD.activate_encoder_interface(params);
}

void SeqStepPage::disable_md_micro() {
  if (md_micro) {
    MD.draw_close_microtiming();
    md_micro = false;
  }
}

bool SeqStepPage::handleEvent(gui_event_t *event) {

  if (SeqPage::handleEvent(event)) {
    return true;
  }

  MDSeqTrack &active_track = mcl_seq.md_tracks[last_md_track];

  if (note_interface.is_event(event) && !grid_page.bank_popup) {
    uint8_t mask = event->mask;
    uint8_t port = event->port;
    MidiDevice *device = midi_active_peering.get_device(port);

    uint8_t track = event->source - 128;
    if (device != &MD) {
      return true;
    }
    if (show_seq_menu) {
      opt_trackid = track + 1;
      note_interface.ignoreNextEvent(track);
      select_track(device, track);
      seq_menu_page.select_item(0);
      return true;
    }
    uint8_t step = track + (page_select * 16);

    step_select = track;

    if (recording) {
      if (event->mask == EVENT_BUTTON_PRESSED) {
        reset_undo();
        config_encoders();
        MD.triggerTrack(track, 127);
        last_rec_event = REC_EVENT_TRIG;
        last_step = track;

        if (MidiClock.state == 2) {
          mcl_seq.md_tracks[track].record_track(127);
        }
        trig_interface.send_md_leds(TRIGLED_OVERLAY);
        return true;
      }
      if (event->mask == EVENT_BUTTON_RELEASED) {
        trig_interface.send_md_leds(TRIGLED_OVERLAY);
        return true;
      }
    }

    if (event->mask == EVENT_BUTTON_PRESSED) {
      update_params_queue = false;
      disable_paramupdate_events();

      if (step >= active_track.length) {
        return true;
      }

      send_locks(step);
      show_pitch = true;

      seq_param2.max =
          mcl_seq.md_tracks[last_md_track].get_timing_mid() * 2 - 1;
      int8_t utiming = active_track.timing[step];
      // Cond
      uint8_t condition =
          translate_to_knob_conditional(active_track.steps[step].cond_id,
                                        active_track.steps[step].cond_plock);
      seq_param1.cur = condition;
      seq_param1.old = seq_param1.cur;

      tuning_t const *tuning = MD.getKitModelTuning(last_md_track);
      uint8_t pitch = active_track.get_track_lock_implicit(step, 0);
      if (pitch > 127) {
        pitch = MD.kit.params[last_md_track][0];
      }
      /*
      if (pitch == 255) {
        seq_param4.cur = 0;
        pitch_param = 255;
      } else if (tuning) {
      */
      if (tuning || is_midi_model) {
        uint8_t note_num = seq_ptc_page.get_note_from_machine_pitch(pitch);
        if (is_midi_model) {
          note_num = pitch;
        }
        if (note_num == 255) {
          seq_param4.cur = 0;
        } else {
          // uint8_t note_offset = tuning->base - ((tuning->base / 12) * 12);
          // note_num = note_num - note_offset;
          seq_param4.cur = note_num;
        }
      }
      seq_param4.old = seq_param4.cur;
      if (utiming == 0) {
        utiming = mcl_seq.md_tracks[last_md_track].get_timing_mid();
      }
      seq_param2.cur = utiming;
      seq_param2.old = utiming;
      if (!active_track.get_step(step, mask_type)) {
        reset_undo();
        bool cond_plock;
        active_track.steps[step].cond_id =
            translate_to_step_conditional(condition, &cond_plock);
        active_track.steps[step].cond_plock = cond_plock;
        active_track.timing[step] = utiming;
        CLEAR_BIT64(active_track.mute_mask, step);
        active_track.set_step(step, mask_type, true);
        SET_BIT16(ignore_release, track);
      }
    } else if (event->mask == EVENT_BUTTON_RELEASED) {
      disable_md_micro();
      show_pitch = false;
      if (IS_BIT_SET16(ignore_release, track)) {
        CLEAR_BIT16(ignore_release, track);
        return true;
      }
      if (step >= active_track.length) {
        return true;
      }

      if (active_track.get_step(step, mask_type)) {
        DEBUG_PRINTLN(F("clear step"));

        if (clock_diff(note_interface.note_hold[port], slowclock) <
            TRIG_HOLD_TIME) {
          reset_undo();
          active_track.set_step(step, mask_type, false);
          //if (mask_type == MASK_PATTERN && !active_track.steps[step].locks) {
            active_track.steps[step].cond_id = 0;
            active_track.steps[step].cond_plock = false;
            active_track.timing[step] = active_track.get_timing_mid();
          //}
        }
      }
      return true;
    }
    return true;
  } // end TI events

  if (EVENT_CMD(event)) {
    uint8_t key = event->source - 64;
    uint8_t step = note_interface.get_first_md_note() + (page_select * 16);
    if (note_interface.get_first_md_note() == 255) {
      step = 255;
    }

    switch (key) {
    // ENCODER BUTTONS
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x13:
    case 0x14:
    case 0x15:
    case 0x16:
    case 0x17: {
      if (step == 255) {
        return true;
      }
      uint8_t param = MD.currentSynthPage * 8 + key - 0x10;
      if (event->mask == EVENT_BUTTON_RELEASED) {
        for (uint8_t n = 0; n < NUM_MD_TRACKS; n++) {
          if (note_interface.is_note_on(n)) {
            uint8_t s = n + (page_select * 16);
            active_track.clear_step_lock(s, param);
          }
        }
      }
      if (event->mask == EVENT_BUTTON_PRESSED) {
        int8_t lock_idx = active_track.find_param(param);

        for (uint8_t n = 0; n < NUM_MD_TRACKS; n++) {
          if (note_interface.is_note_on(n)) {
            uint8_t s = n + (page_select * 16);
            if (lock_idx == -1 || !active_track.steps[s].is_lock(lock_idx)) {
              DEBUG_PRINTLN("setting lock");
              active_track.set_track_locks(s, param,
                                           MD.kit.params[last_md_track][param]);
              trig_interface.ignoreNextEvent(key);
            }
          }
        }
      }
      send_locks(step);
      return true;
    }
    }

    if (event->mask == EVENT_BUTTON_PRESSED) {
      switch (key) {
      case MDX_KEY_COPY: {
        // Note copy
        if (step != 255) {
          opt_copy_step_handler(255);
          disable_md_micro();
        } else if (trig_interface.is_key_down(MDX_KEY_SCALE)) {
          opt_copy_page_handler_cb();
          page_copy = 1;
          trig_interface.ignoreNextEvent(MDX_KEY_SCALE);
        } else {
          // Track copy
          opt_copy = recording ? 2 : 1;
          opt_copy_track_handler_cb();
        }
        return true;
      }
      case MDX_KEY_PASTE: {
        // Note paste
        reset_undo();
        if (step != 255) {
          opt_paste_step_handler();
          disable_md_micro();
          send_locks(step);
        } else if (trig_interface.is_key_down(MDX_KEY_SCALE)) {
          if (!page_copy || (opt_undo != 255)) { break; }
          opt_paste_page_handler();
          trig_interface.ignoreNextEvent(MDX_KEY_SCALE);
        } else {
          // Track paste
          opt_paste = recording ? 2 : 1;
          opt_paste_track_handler();
        }
        return true;
      }
      case MDX_KEY_CLEAR: {
        // Note clear
        if (step != 255) {
          if (last_step != step) {
            reset_undo();
          }
          opt_clear_step = 1;
          opt_clear_step_handler();
          disable_md_micro();
          last_step = step;
        } else if (trig_interface.is_key_down(MDX_KEY_SCALE)) {
          page_copy = 1;
          opt_clear_page_handler();
          trig_interface.ignoreNextEvent(MDX_KEY_SCALE);
        } else {
          // Track clear
          opt_clear = recording ? 2 : 1;
          opt_clear_track_handler();
        }
        return true;
      }
      case MDX_KEY_NO: {
        if (mask_type != MASK_PATTERN) {
          mask_type = MASK_PATTERN;
          config_mask_info(false);
        }
        else {
          for (uint8_t n = 0; n < NUM_MD_TRACKS; n++) {
            if (note_interface.is_note_on(n)) {
              uint8_t s = n + (page_select * 16);
              TOGGLE_BIT64(active_track.mute_mask,s);
            }
          }
        }
        return true;
      }
      case MDX_KEY_BANKB: {
        if (trig_interface.is_key_down(MDX_KEY_FUNC)) {
          if (mask_type == MASK_LOCK) {
            mask_type = MASK_PATTERN;
          } else {
            mask_type = MASK_LOCK;
          }
          config_mask_info(false);
        return true;
        }
      }
      case MDX_KEY_BANKC: {
        if (trig_interface.is_key_down(MDX_KEY_FUNC)) {
          if (mask_type == MASK_MUTE) {
            mask_type = MASK_PATTERN;
          } else {
            mask_type = MASK_MUTE;
          }
          config_mask_info(false);
        return true;
        }
      }

      case MDX_KEY_BANKD: {
        if (trig_interface.is_key_down(MDX_KEY_FUNC)) {
          if (mask_type == MASK_SLIDE) {
            mask_type = MASK_PATTERN;
          } else {
            mask_type = MASK_SLIDE;
          }
          config_mask_info(false);
        return true;
        }
      }
      }
      if (step != 255) {
        switch (key) {
        case MDX_KEY_YES: {
          active_track.send_parameter_locks(step, true);
          reset_on_release = true;
          MD.triggerTrack(last_md_track, 127);
          return true;
        }

        case MDX_KEY_UP: {
          seq_param1.cur += 1;
          return true;
        }
        case MDX_KEY_DOWN: {
          seq_param1.cur -= 1;
          return true;
        }
        case MDX_KEY_LEFT: {
          seq_param2.cur -= 1;
          return true;
        }
        case MDX_KEY_RIGHT: {
          seq_param2.cur += 1;
          return true;
        }
        }
      }
    }
  }
  /*
      if (recording) {
        if (EVENT_PRESSED(event, Buttons.BUTTON4)) {
          switch (last_rec_event) {
          case REC_EVENT_TRIG:
            if (BUTTON_DOWN(Buttons.BUTTON3)) {
              oled_display.textbox("CLEAR ", "TRACKS");
              for (uint8_t n = 0; n < 16; ++n) {
                mcl_seq.md_tracks[n].clear_track();
              }
            } else {
              oled_display.textbox("CLEAR ", "TRACK");
              mcl_seq.md_tracks[last_step].clear_track();
            }
            break;
          case REC_EVENT_CC:
            oled_display.textbox("CLEAR ", "LOCK");
            active_track.clear_param_locks(last_param_id);
            if (BUTTON_DOWN(Buttons.BUTTON3)) {
              oled_display.textbox("CLEAR ", "LOCKS");
              for (uint8_t c = 0; c < NUM_LOCKS; c++) {
                if (active_track.locks_params[c] > 0) {
                  active_track.clear_param_locks(active_track.locks_params[c] -
                                                 1);
                }
              }
            }
            break;
          }
          return true;
        }
      }
    */

  if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
    toggle_record();
    return true;
  }

  return false;
}

void SeqStepMidiEvents::onControlChangeCallback_Midi(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t param = msg[1];
  uint8_t value = msg[2];
  uint8_t track;
  uint8_t track_param;

  MD.parseCC(channel, param, &track, &track_param);
  if (track > 15) {
    return;
  }

  MDSeqTrack &active_track = mcl_seq.md_tracks[last_md_track];

  uint8_t step;
  if (track_param > 23) {
    return;
  }

  if (SeqPage::recording) {
    // Record CC
    seq_step_page.last_rec_event = REC_EVENT_CC;
    if (MidiClock.state != 2) {
      return;
    }

    seq_step_page.last_param_id = track_param;
    // ignore level
    if (track_param > 31) {
      return;
    }
    seq_step_page.config_encoders();

    mcl_seq.md_tracks[track].record_track_locks(track_param, value);
    return;
  }

  uint8_t store_lock = 255;
  for (uint8_t i = 0; i < 16; i++) {
    if ((note_interface.is_note_on(i))) {
      step = i + (SeqPage::page_select * 16);
      if (step < active_track.length) {
        if (active_track.set_track_locks(step, track_param, value)) {
          store_lock = 0;
          trig_interface.ignoreNextEvent(track_param - MD.currentSynthPage * 8 +
                                         16);
        } else {
          store_lock = 1;
        }

        active_track.enable_step_locks(step);
        if (seq_step_page.mask_type == MASK_PATTERN) {
          uint8_t utiming = (seq_param2.cur + 0);
          bool cond_plock;
          uint8_t condition = seq_step_page.translate_to_step_conditional(
              seq_param1.cur, &cond_plock);

          active_track.steps[step].trig = true;
          active_track.steps[step].cond_id = condition;
          active_track.steps[step].cond_plock = cond_plock;
          active_track.timing[step] = utiming;

        } else {
          // SET_BIT64_P(mask, step);
        }
      }
    }
  }
  if (store_lock == 0) {
    char str[5] = "--  ";
    char str2[4] = "-- ";
    const char *modelname =
        model_param_name(MD.kit.get_model(last_md_track), track_param);
    if (modelname != NULL) {
      strncpy(str, modelname, 3);
      if (strlen(str) == 2) {
        str[2] = ' ';
        str[3] = '\0';
      }
    }
    mcl_gui.put_value_at(value, str2);
    oled_display.textbox(str, str2);
  }
  if (store_lock == 1) {
    oled_display.textbox("LOCK PARAMS ", "FULL");
    // seq_step_page.send_locks(step);
  }
}

void SeqStepMidiEvents::setup_callbacks() {
  if (state) {
    return;
  }

  Midi.addOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&SeqStepMidiEvents::onControlChangeCallback_Midi);
  state = true;
}

void SeqStepMidiEvents::remove_callbacks() {

  if (!state) {
    return;
  }
  Midi.removeOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&SeqStepMidiEvents::onControlChangeCallback_Midi);
  state = false;
}
