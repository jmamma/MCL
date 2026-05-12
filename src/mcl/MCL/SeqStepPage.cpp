#include "SeqStepPage.h"
#include "../Drivers/MidiDevice.h"
#include "DeviceManager.h"
#include "DeviceParamResolver.h"
#include "GUI_hardware.h"
#include "GridPages.h"
#include "MCLGUI.h"
#include "MCLStrings.h"
#include "MCLSysConfig.h"
#include "PageSelectPage.h"
#include "SeqPages.h"
#include "SeqStepTrackRef.h"
#ifdef PLATFORM_TBD
#include "MidiSetup.h"
#endif
#include "MCLSeq.h"

namespace {

constexpr uint8_t kStepPageVisibleSteps = 16;
constexpr uint8_t kStepPageKeyboardKeys = 24;

uint8_t page_step_offset() {
  return SeqPage::page_select * kStepPageVisibleSteps;
}

SeqStepTrackRef step_track_for(uint8_t track) {
  return seq_step_track_for(track);
}

SeqStepTrackRef active_step_track() { return seq_step_active_track(); }

#ifdef PLATFORM_TBD
void retain_shift_step_selection(SeqStepPage &page, uint8_t track) {
  SET_BIT16(page.shift_select_latch, track);
  SET_BIT32(note_interface.notes_on, track);
  CLEAR_BIT32(note_interface.notes_off, track);
}

void clear_shift_step_selection(SeqStepPage &page) {
  uint16_t latch = page.shift_select_latch;
  if (latch == 0) {
    return;
  }
  for (uint8_t n = 0; n < kStepPageVisibleSteps; ++n) {
    if (IS_BIT_SET16(latch, n)) {
      note_interface.clear_note(n);
    }
  }
  page.shift_select_latch = 0;
}
#endif

void draw_active_step_masks(SeqStepPage &page, SeqStepTrackRef active_track,
                            uint8_t offset, bool show_current_step = true) {
  uint64_t mask = 0, lock_mask = 0, mute_mask = 0, slide_mask = 0;
  uint64_t led_mask = 0;
  uint8_t step_count = active_track.step_count();
  uint8_t length = active_track.length();

  active_track.get_mask(&mask, MASK_PATTERN);
  switch (SeqPage::mask_type) {
  case MASK_PATTERN:
    led_mask = mask;
    mute_mask = active_track.mute_mask();
    break;
  case MASK_LOCK:
    active_track.get_mask(&lock_mask, MASK_LOCK);
    led_mask = lock_mask;
    mask = lock_mask;
    break;
  case MASK_MUTE:
    mute_mask = active_track.mute_mask();
    led_mask = mute_mask;
    break;
  case MASK_SLIDE:
    active_track.get_mask(&slide_mask, MASK_SLIDE);
    led_mask = slide_mask;
    break;
  }

  page.shed_mask(led_mask, length, offset);
  page.draw_mask(offset, mask, step_count, length, mute_mask, slide_mask,
                 show_current_step);

  if (SeqPage::recording) {
    return;
  }

  uint64_t locks_on_step_mask_ = 0;
  active_track.get_mask(&locks_on_step_mask_, MASK_LOCKS_ON_STEP);
  page.shed_mask(locks_on_step_mask_, length, offset);

  if ((uint16_t)led_mask != trigled_mask) {
    trigled_mask = led_mask;
    active_track.set_step_edit_trig_leds(trigled_mask, TRIGLED_STEPEDIT);
    if (active_track.uses_kit_sound() && SeqPage::mask_type == MASK_MUTE) {
      active_track.set_step_edit_trig_leds(mask, TRIGLED_STEPEDIT, 1);
    }
    if (SeqPage::mask_type == MASK_MUTE) {
      GUI_hardware.led.set_trigleds(mask, TRIGLED_STEPEDIT, 1);
    }
  }
  if ((uint16_t)locks_on_step_mask_ != locks_on_step_mask) {
    locks_on_step_mask = locks_on_step_mask_;
    active_track.set_step_edit_trig_leds(locks_on_step_mask, TRIGLED_STEPEDIT,
                                         1);
  }

  GUI_hardware.led.set_trigleds(led_mask, TRIGLED_STEPEDIT);
  GUI_hardware.led.set_trigleds(locks_on_step_mask, TRIGLED_STEPEDIT, 1);
  if (MidiClock.state == 2 && step_count >= offset &&
      step_count < offset + kStepPageVisibleSteps) {
    GUI_hardware.led.toggle_trigled(step_count - offset);
  }
}

void draw_active_microtiming(SeqStepTrackRef active_track,
                             uint8_t encoder_value) {
#if !defined(__AVR__)
  if (active_track.uses_signed_microtiming()) {
    mcl_gui.draw_microtiming_spsx(
        active_track.speed(),
        active_track.microtiming_from_encoder(encoder_value));
    return;
  }
#endif
  mcl_gui.draw_microtiming(active_track.speed(), encoder_value);
}

} // namespace

bool SeqStepPage::toggle_mask(uint8_t mask) {
  if (key_interface.is_key_down(MDX_KEY_FUNC)) {
    mask_type = (mask_type == mask) ? MASK_PATTERN : mask;
    config_mask_info(false);
    return true;
  }
  return false;
}

void SeqStepPage::config() {
  SeqStepTrackRef active_track = active_step_track();

  uint8_t driver_pitch_max = 0;
  bool is_midi_model = false;
  if (active_track.configure_panel(info1, sizeof(info1), driver_pitch_max,
                                   is_midi_model)) {
    SeqPage::is_midi_model = is_midi_model;
    seq_param4.max = driver_pitch_max;
    seq_param4.cur = 0;
    seq_param4.old = 0;
    config_mask_info(mask_type == MASK_PATTERN);
    config_encoders();
    config_as_trackedit();
    return;
  }
}

void SeqStepPage::config_encoders() {
  if (show_seq_menu) {
    return;
  }

  SeqStepTrackRef active_track = active_step_track();
  seq_param3.cur = active_track.length();
  seq_param3.old = seq_param3.cur;
  seq_param2.cur = active_track.timing_encoder_center();
  seq_param2.old = seq_param2.cur;
  seq_param2.min = active_track.timing_encoder_min();
  seq_param2.max = active_track.timing_encoder_max();
}

void SeqStepPage::init() {
  DEBUG_PRINT_FN();
  DEBUG_PRINTLN(F("init seqstep"));
  SeqPage::init();

  pitch_param = 255;
  SeqPage::select_device_slot(1);
  SeqStepTrackRef active_track = active_step_track();

  seq_menu_page.menu.enable_entry(SEQ_MENU_MASK, true);
  seq_menu_page.menu.enable_entry(SEQ_MENU_SOUND,
                                  active_track.uses_kit_sound());
  seq_menu_page.menu.enable_entry(SEQ_MENU_LENGTH_PRIMARY, true);

  midi_events.setup_callbacks();
  key_interface.on();
  active_track.set_step_edit_rec_mode(1);
  key_interface.send_trig_leds(TRIGLED_STEPEDIT);
  check_and_set_page_select();

  active_track.sync_step_edit();

  trigled_mask = 0;
  locks_on_step_mask = 0;
  config();

  reset_on_release = false;
  ignore_release = 0;
#ifdef PLATFORM_TBD
  clear_shift_step_selection(*this);
#endif
  update_params_queue = false;
  ((MCLEncoder *)encoders[2])->handler = pattern_len_handler;
  seq_param1.max = active_track.condition_count() * 2;
  seq_param2.min = active_track.timing_encoder_min();
  seq_param2.old = active_track.timing_encoder_center();
  seq_param1.cur = 0;
  seq_param3.max = 64;
  seq_param3.min = 1;
}

void SeqStepPage::disable_paramupdate_events() {
  if (!active_step_track().uses_kit_sound()) {
    return;
  }
  active_step_track().set_live_param_update(false);
}

void SeqStepPage::enable_paramupdate_events() {
  if (!active_step_track().uses_kit_sound()) {
    return;
  }
  active_step_track().set_live_param_update(true);
}

void SeqStepPage::cleanup() {
  SeqStepTrackRef active_track = active_step_track();
  midi_events.remove_callbacks();
#ifdef PLATFORM_TBD
  clear_shift_step_selection(*this);
#endif
  enable_paramupdate_events();
  SeqPage::cleanup();
  params_reset();
  if (active_track.uses_kit_sound()) {
    active_track.set_step_edit_rec_mode(0);
    active_track.clear_step_edit_popup();
    active_track.end_param_editor();
  }
}

void SeqStepPage::display() {
  SeqStepTrackRef active_track = active_step_track();

  oled_display.clearDisplay();
  draw_knob_frame();

  uint8_t timing_mid = active_track.timing_display_mid();

  draw_knob_conditional(seq_param1.getValue());
  draw_knob_timing(seq_param2.getValue(), timing_mid);

  char K[4];
  mcl_gui.put_value_at(seq_param3.getValue(), K);
  if (IS_BIT_SET16(mcl_cfg.poly_mask, seq_primary_track_index())) {
    draw_knob(2, mclstr_plen, K);
  } else {
    draw_knob(2, mclstr_len, K);
  }
  bool is_ptc = active_track.uses_note_pitch();
  if (show_pitch && is_ptc) {
    strcpy_P(K, mclstr_dash);
    if (seq_param4.cur != 0) {
      // uint8_t base = tuning->base;
      uint8_t note_num = seq_param4.cur;
      // + base;

      uint8_t note = note_num % 12;
      uint8_t oct = note_num / 12;

      strcpy(K, number_to_note.notes_upper[note]);
      mcl_gui.put_value_at(oct, K + 2);
      K[3] = 0;
    }
    draw_knob(3, mclstr_ptc, K);
  }

  if (mcl_gui.show_encoder_value(&seq_param4) && (seq_param4.cur > 0) &&
      (note_interface.notes_count_on() > 0) && (!show_seq_menu) && (is_ptc) &&
      !(recording)) {
    uint64_t note_mask[2] = {};
    uint8_t note = seq_param4.cur; // + tuning->base;
    SET_BIT64(note_mask, note);
    mcl_gui.draw_keyboard(32, 23, 6, 9, kStepPageKeyboardKeys, note_mask);
    SeqPage::display();
  }

  else {
    uint8_t _midi_lock_tmp = MidiUartParent::handle_midi_lock;
    MidiUartParent::handle_midi_lock = 1;

    draw_active_step_masks(*this, active_track, page_step_offset());

    MidiUartParent::handle_midi_lock = _midi_lock_tmp;

    SeqPage::display();
    if (microtiming_overlay_active() &&
        mcl_gui.show_encoder_value(&seq_param2) &&
        (note_interface.notes_count_on() > 0) && (!show_seq_menu) &&
        (!recording)) {
      draw_active_microtiming(active_track, seq_param2.cur);
    }
  }
  if (prepare) {
    page_select_page.prepare_overlay();
    prepare = false;
  }
  // The SPS bottom-32 strip is now an overlay (SpsStripPage) installed
  // by SpsMode while the latch is active; rendering happens via the
  // GUI overlay slot, not from inside this page's display().
}

void SeqStepPage::loop() {
  SeqStepTrackRef active_track = active_step_track();
  if (!active_track.step_editor_available()) {
    mcl.setPage(GRID_PAGE);
    return;
  }
  SeqPage::loop();

  if (recording)
    return;

  uint8_t _midi_lock_tmp = MidiUartParent::handle_midi_lock;
  MidiUartParent::handle_midi_lock = 1;
  if (pitch_param != 255) {
    seq_param4.cur = pitch_param;
    pitch_param = 255;
  }

  if (seq_param1.hasChanged() || seq_param2.hasChanged() ||
      seq_param4.hasChanged()) {
    bool has_note_pitch = active_track.uses_note_pitch();

    for (uint8_t n = 0; n < kStepPageVisibleSteps; n++) {
      if (note_interface.is_note_on(n)) {
        uint8_t step = n + page_step_offset();
        if (step < active_track.length()) {
          bool cond_plock;
          uint8_t condition = active_track.step_conditional_from_knob(
              seq_param1.cur, &cond_plock);

          active_track.set_conditional(step, condition, cond_plock);
          active_track.set_timing_from_encoder(step, seq_param2.cur);

          if (seq_param2.hasChanged()) {
            set_microtiming_overlay_active(true);
            draw_active_microtiming(active_track, seq_param2.cur);
          }
          if (seq_param1.hasChanged() && active_track.uses_kit_sound()) {
            char str[4];
            if (seq_param1.getValue() > 0) {
              conditional_str(str, seq_param1.getValue(), true);
              active_track.popup_text(str);
            }
          }
          switch (mask_type) {
          case MASK_LOCK:
            active_track.enable_step_locks(step);
            break;
          case MASK_PATTERN:
            active_track.set_step(step, MASK_PATTERN, true);
            break;
          }
          if (has_note_pitch && seq_param4.hasChanged() && seq_param4.cur > 0) {
            uint8_t pitch = active_track.pitch_lock_from_note(seq_param4.cur);
            if (pitch != 255) {
              active_track.set_track_pitch(step, pitch);
            }
            seq_step_page.encoders_used_clock[3] = read_clock_ms();
          }
        }
      }
    }
    seq_param1.old = seq_param1.cur;
    seq_param2.old = seq_param2.cur;
    seq_param4.old = seq_param4.cur;
  }

  if (update_params_queue &&
      clock_diff(update_params_clock, read_clock_ms()) > 400) {
    enable_paramupdate_events();
    update_params_queue = false;
  }

  if (note_interface.trig_notes_all_released() && !grid_page.bank_popup) {
    init_encoders_used_clock();
    active_track.end_param_editor();

    update_params_queue = true;
    update_params_clock = read_clock_ms();

    note_interface.init_notes();
  }
  MidiUartParent::handle_midi_lock = _midi_lock_tmp;
}

void SeqStepPage::send_locks(uint8_t step) {
  SeqStepTrackRef active_track = active_step_track();
  if (!active_track.uses_kit_sound()) {
    return;
  }
  uint8_t params[SPS_PARAMS_PER_TRACK];
  memset(params, 255, sizeof(params));
  active_track.get_step_locks(step, params, true);
  active_track.begin_param_editor(params, sizeof(params));
}

void SeqStepPage::disable_microtiming_overlay() {
  if (microtiming_overlay_active()) {
    active_step_track().close_microtiming();
    set_microtiming_overlay_active(false);
  }
}

bool SeqStepPage::handleEvent(gui_event_t *event) {

#ifdef PLATFORM_TBD
  if (EVENT_CMD(event) && event->source == MDX_KEY_FUNC &&
      event->mask == EVENT_BUTTON_RELEASED && shift_select_latch != 0) {
    clear_shift_step_selection(*this);
    disable_microtiming_overlay();
    show_pitch = false;
    return true;
  }
#endif

  if (SeqPage::handleEvent(event)) {
    return true;
  }

  if (EVENT_NOTE(event) && !grid_page.bank_popup) {
    SeqStepTrackRef active_track = active_step_track();
    uint8_t port = event->port;
    uint8_t track = event->source;
    if (!device_manager.port_supports(port,
                                      MidiDeviceCapability::MdTrigInterface)) {
      return true;
    }
    MidiDevice *device = device_manager.device_for_port(port);
    if (show_seq_menu) {
      opt_trackid = track + 1;
      note_interface.ignoreNextEvent(track);
      if (active_track.selects_track_locally()) {
        seq_set_primary_track_index(track);
        config();
      } else {
        select_track(device, track);
      }
      seq_menu_page.select_item(0);
      return true;
    }
    uint8_t step = track + page_step_offset();
    uint8_t length = active_track.length();

    step_select = track;

    if (recording) {
      if (event->mask == EVENT_BUTTON_PRESSED) {
        reset_undo();
        config_encoders();
        device->triggerTrack(track, 127);
        last_rec_event = REC_EVENT_TRIG;
        last_step = track;

        if (MidiClock.state == 2) {
          step_track_for(track).record_track(127);
        }
        key_interface.send_trig_leds(TRIGLED_OVERLAY);
        return true;
      }
      if (event->mask == EVENT_BUTTON_RELEASED) {
        key_interface.send_trig_leds(TRIGLED_OVERLAY);
        return true;
      }
    }

    if (event->mask == EVENT_BUTTON_PRESSED) {
      update_params_queue = false;
      disable_paramupdate_events();

      if (step >= length) {
        return true;
      }

      send_locks(step);
      show_pitch = active_track.uses_step_pitch();

      // Cond
      uint8_t condition = active_track.knob_conditional_from_step(
          active_track.conditional_id(step),
          active_track.conditional_plock(step));
      seq_param1.cur = condition;
      seq_param1.old = seq_param1.cur;

      if (active_track.uses_note_pitch()) {
        uint8_t pitch_param = active_track.uses_kit_sound()
                                  ? 0
                                  : active_track.pitch_lock_param_id();
        uint8_t pitch = active_track.get_track_lock_implicit(step, pitch_param);
        if (pitch > 127) {
          pitch = active_track.default_pitch_lock();
        }
        uint8_t note_num = active_track.note_from_pitch_lock(pitch);
        seq_param4.cur = (note_num == 255) ? 0 : note_num;
      }
      seq_param4.old = seq_param4.cur;
      seq_param2.cur = active_track.timing_encoder_for_step(step);
      seq_param2.old = seq_param2.cur;
      seq_param2.min = active_track.timing_encoder_min();
      seq_param2.max = active_track.timing_encoder_max();
      if (!active_track.get_step(step, mask_type)) {
        reset_undo();
        bool cond_plock;
        uint8_t step_condition =
            active_track.step_conditional_from_knob(condition, &cond_plock);
        active_track.set_conditional(step, step_condition, cond_plock);
        active_track.set_timing_from_encoder(step, seq_param2.cur);
        active_track.clear_mute(step);
        active_track.set_step(step, mask_type, true);
        SET_BIT16(ignore_release, track);
      }
    } else if (event->mask == EVENT_BUTTON_RELEASED) {
#ifdef PLATFORM_TBD
      if (step < length && key_interface.is_key_down(MDX_KEY_FUNC)) {
        CLEAR_BIT16(ignore_release, track);
        retain_shift_step_selection(*this, track);
        return true;
      }
#endif
      disable_microtiming_overlay();
      show_pitch = false;
      if (IS_BIT_SET16(ignore_release, track)) {
        CLEAR_BIT16(ignore_release, track);
        return true;
      }
      if (step >= length) {
        return true;
      }

      if (active_track.get_step(step, mask_type)) {
        if (clock_diff(note_interface.note_hold[port], read_clock_ms()) <
            TRIG_HOLD_TIME) {
          reset_undo();
          active_track.set_step(step, mask_type, false);
          active_track.clear_conditional(step);
          active_track.reset_timing(step);
          if (active_track.clears_mute_on_pattern_clear() &&
              mask_type == MASK_PATTERN) {
            active_track.clear_mute(step);
          }
        }
      }
      return true;
    }
    return true;
  } // end EVENT_NOTE

  if (EVENT_CMD(event)) {
    SeqStepTrackRef active_track = active_step_track();
    uint8_t key = event->source;
    uint8_t first_note = note_interface.get_first_trig_note();
    uint8_t page_offset = page_step_offset();
    uint8_t step = (first_note == 255) ? 255 : first_note + page_offset;

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
      uint8_t param = 0;
      if (!active_track.param_from_key(key, &param)) {
        return true;
      }
      if (param >= active_track.lock_param_count()) {
        return true;
      }
      if (event->mask == EVENT_BUTTON_RELEASED) {
        for (uint8_t n = 0; n < kStepPageVisibleSteps; n++) {
          if (note_interface.is_note_on(n)) {
            active_track.clear_step_lock(n + page_offset, param);
          }
        }
      }
      if (event->mask == EVENT_BUTTON_PRESSED) {
        int8_t lock_idx = active_track.find_param(param);
        for (uint8_t n = 0; n < kStepPageVisibleSteps; n++) {
          if (note_interface.is_note_on(n)) {
            uint8_t s = n + page_offset;
            if (lock_idx == -1 || !active_track.step_has_lock(s, lock_idx)) {
              uint8_t value = active_track.current_lock_value(param);
              active_track.set_track_locks(s, param, value);
              key_interface.ignoreNextEvent(key);
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
          disable_microtiming_overlay();
        } else if (key_interface.is_key_down(MDX_KEY_SCALE)) {
          opt_copy_page_handler_cb();
          page_copy = 1;
          key_interface.ignoreNextEvent(MDX_KEY_SCALE);
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
          disable_microtiming_overlay();
          send_locks(step);
        } else if (key_interface.is_key_down(MDX_KEY_SCALE)) {
          if (!page_copy || (opt_undo != 255)) {
            break;
          }
          opt_paste_page_handler();
          key_interface.ignoreNextEvent(MDX_KEY_SCALE);
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
          disable_microtiming_overlay();
          last_step = step;
        } else if (key_interface.is_key_down(MDX_KEY_SCALE)) {
          page_copy = 1;
          opt_clear_page_handler();
          key_interface.ignoreNextEvent(MDX_KEY_SCALE);
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
        } else {
          for (uint8_t n = 0; n < kStepPageVisibleSteps; n++) {
            if (note_interface.is_note_on(n)) {
              active_track.toggle_mute(n + page_offset);
            }
          }
        }
        return true;
      }
      case MDX_KEY_BANKB: {
        if (toggle_mask(MASK_LOCK))
          return true;
      }
      case MDX_KEY_BANKC: {
        if (toggle_mask(MASK_MUTE))
          return true;
      }
      case MDX_KEY_BANKD: {
        if (toggle_mask(MASK_SLIDE))
          return true;
      }
      }
      if (step != 255) {
        switch (key) {
        case MDX_KEY_YES: {
          active_track.preview_step(step);
          reset_on_release = true;
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
  if (EVENT_BUTTON(event)) {
#ifndef PLATFORM_TBD
    if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
      toggle_record();
      return true;
    }
#endif
  }
  return false;
}

void SeqStepMidiEvents::onControlChangeCallback_Midi(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t param = msg[1];
  uint8_t value = msg[2];
  uint8_t track;
  uint8_t track_param;

  if (!seq_step_tracks_parse_kit_cc(channel, param, &track, &track_param) ||
      track >= kStepPageVisibleSteps) {
    return;
  }

  SeqStepTrackRef event_track = seq_step_track_for(track);
  // Engine, not device: the recorder writes into the active engine's lock
  // storage. If the device sends a param outside the active engine's lock
  // surface, drop it because there is nowhere to store it.
  if (track_param >= event_track.lock_param_count()) {
    return;
  }

  if (SeqPage::recording) {
    seq_step_page.last_rec_event = REC_EVENT_CC;
    if (MidiClock.state != 2)
      return;
    seq_step_page.last_param_id = track_param;
    seq_step_page.config_encoders();
    event_track.record_track_locks(track_param, value);
    return;
  }

  SeqStepTrackRef active_track = active_step_track();
  uint8_t store_lock = 255;
  for (uint8_t i = 0; i < kStepPageVisibleSteps; i++) {
    if ((note_interface.is_note_on(i))) {
      uint8_t step = i + page_step_offset();
      if (step < active_track.length()) {
        if (active_track.set_track_locks(step, track_param, value)) {
          store_lock = 0;
          uint8_t key = 0;
          if (active_track.key_for_param(track_param, &key)) {
            key_interface.ignoreNextEvent(key);
          }
        } else {
          store_lock = 1;
        }

        active_track.enable_step_locks(step);
        if (seq_step_page.mask_type == MASK_PATTERN) {
          active_track.set_pattern_step_from_edit(step, seq_param1.cur,
                                                  seq_param2.cur);
        }
      }
    }
  }
  if (store_lock == 0 && active_track.shows_lock_value_popup()) {
    char str[5];
    mclstr_copy_progmem(str, mclstr_dash_dash_space, sizeof(str));
    char str2[4];
    mclstr_copy_progmem(str2, mclstr_dash_space, sizeof(str2));
    active_track.copy_lock_param_label(track_param, str, sizeof(str));
    mcl_gui.put_value_at(value, str2);
    oled_display.textbox(str, str2);
  }
  if (store_lock == 1) {
    oled_display.textbox_P(mclstr_lock_params, mclstr_full);
    // seq_step_page.send_locks(step);
  }
}

void SeqStepMidiEvents::setup_callbacks() {
  if (state) {
    return;
  }
  if (!seq_step_tracks_parse_kit_cc_enabled()) {
    return;
  }

  MidiDevice *device = DeviceParamResolver::slot_device(1);
  if (device == nullptr || device->midi == nullptr) {
    return;
  }
  device->midi->addOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&SeqStepMidiEvents::onControlChangeCallback_Midi);
  state = true;
}

void SeqStepMidiEvents::remove_callbacks() {

  if (!state) {
    return;
  }
  MidiDevice *device = DeviceParamResolver::slot_device(1);
  if (device == nullptr || device->midi == nullptr) {
    state = false;
    return;
  }
  device->midi->removeOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&SeqStepMidiEvents::onControlChangeCallback_Midi);
  state = false;
}
