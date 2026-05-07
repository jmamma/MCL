#include "TBDSeqTrack.h"

#ifdef PLATFORM_TBD

#include "AuxPages.h"
#include "GridTrack.h"
#include "MidiClock.h"
#include "TBD.h"

namespace {

uint8_t tbd_p4_param_to_lock_value(const TbdP4ParamDescriptor &desc,
                                   int16_t value) {
  if (desc.max_value <= desc.min_value) {
    return 0;
  }
  if (value < desc.min_value) value = desc.min_value;
  if (value > desc.max_value) value = desc.max_value;
  uint16_t range = (uint16_t)(desc.max_value - desc.min_value);
  uint16_t offset = (uint16_t)(value - desc.min_value);
  return (uint8_t)(((uint32_t)offset * 127u + (range / 2u)) / range);
}

} // namespace

TBDSeqTrack::TBDSeqTrack() : TBDSeqDataTrack() {
  active = TBD_TRACK_TYPE;
  p4_sound.clear();
}

void TBDSeqTrack::reset() {
  send_active_note_off();
  TBDSeqDataTrack::reset();
  oneshot_mask_ = 0;
  gate_ticks_remaining_ = 0;
}

void TBDSeqTrack::seq(MidiUartClass *uart_) {
  port = uart_;
  uint16_t tps = get_ticks_per_step();

  tick_counter++;
  service_gate();
  if (tick_counter >= tps) {
    tick_counter = 0;
    cur_event_idx += stepseq_popcount(steps[step_count].locks);
    if (ignore_step == step_count) {
      ignore_step = 255;
    }
    step_count_inc();
  }
  update_legacy_progress_counter(tps);

  if (count_down) {
    count_down--;
    if (count_down == 0) {
      reset();
      tick_counter = 0;
      mod12_counter = 0;
    }
    return;
  }

  if (mute_state == STEPSEQ_MUTE_ON || ignore_step == step_count) {
    return;
  }

  send_slides(locks_params, p4_sound.midi_channel);

  uint8_t next_step = (step_count == (length - 1)) ? 0 : step_count + 1;
  uint8_t current_step = 255;

  int8_t mt_current = microtiming[step_count];
  if (mt_current >= 0) {
    int16_t tick_offset = stepseq_microtiming_to_ticks(mt_current, tps);
    if (tick_counter == (uint16_t)(tick_offset + 1)) {
      current_step = step_count;
    }
  }

  int8_t mt_next = microtiming[next_step];
  if (mt_next < 0 && current_step == 255) {
    int16_t trigger_tick =
        (int16_t)(tps + stepseq_microtiming_to_ticks(mt_next, tps));
    if (trigger_tick < 1) {
      trigger_tick = 1;
    }
    if (tick_counter == (uint16_t)trigger_tick) {
      current_step = next_step;
    }
  }

  if (current_step == 255) {
    return;
  }

  uint16_t lock_idx = cur_event_idx;
  if (current_step == next_step) {
    lock_idx = current_step == 0
                   ? 0
                   : (uint16_t)(cur_event_idx +
                                stepseq_popcount(steps[step_count].locks));
  }

  auto &step = steps[current_step];
  const bool trig = STEPSEQ_IS_BIT_SET64(trig_mask, current_step);
  const bool slide = STEPSEQ_IS_BIT_SET64(slide_mask, current_step);
  const uint8_t trig_result = trig_conditional(current_step, step.cond_id);
  const bool cond_ok = trig_result == STEPSEQ_TRIG_TRUE;

  if (cond_ok || (!step.cond_plock && trig_result != STEPSEQ_TRIG_ONESHOT)) {
    send_parameter_locks(current_step, lock_idx, trig);
    if (slide) {
      locks_slides_recalc = current_step;
      locks_slides_idx = lock_idx;
    }
  }
  if (cond_ok && trig) {
    send_trig(current_step,
              STEPSEQ_IS_BIT_SET64(accent_mask, current_step) ? 127 : 100);
  }
  record_trig_result(cond_ok && trig);
}

uint8_t TBDSeqTrack::trig_conditional(uint8_t step, uint8_t condition) {
  if (STEPSEQ_IS_BIT_SET64(mute_mask, step)) {
    return STEPSEQ_TRIG_ONESHOT;
  }

  if (condition == STEPSEQ_COND_ONESHOT) {
    if (STEPSEQ_IS_BIT_SET64(oneshot_mask_, step)) {
      return STEPSEQ_TRIG_ONESHOT;
    }
    STEPSEQ_SET_BIT64(oneshot_mask_, step);
    return STEPSEQ_TRIG_TRUE;
  }

  return conditional(condition) ? STEPSEQ_TRIG_TRUE : STEPSEQ_TRIG_FALSE;
}

bool TBDSeqTrack::preview_step(uint8_t step) {
  if (step >= length) {
    return false;
  }

  MidiUartClass *old_port = port;
  port = uart ? uart : TBD.uart;
  if (port == nullptr) {
    port = old_port;
    return false;
  }

  send_parameter_locks(step, get_lockidx(step), true);
  send_trig(step, STEPSEQ_IS_BIT_SET64(accent_mask, step) ? 127 : 100);

  port = old_port;
  return true;
}

bool TBDSeqTrack::trigger(uint8_t velocity, MidiUartClass *uart_) {
  MidiUartClass *old_port = port;
  port = uart_ ? uart_ : (uart ? uart : TBD.uart);
  if (port == nullptr) {
    port = old_port;
    return false;
  }

  uint8_t step = step_count;
  if (length == 0 || step >= length) step = 0;
  send_trig(step, velocity);

  port = old_port;
  return true;
}

void TBDSeqTrack::send_notes_off() {
  MidiUartClass *old_port = port;
  port = uart ? uart : TBD.uart;
  send_active_note_off();
  port = old_port;
}

void TBDSeqTrack::send_trig(uint8_t step, uint8_t velocity) {
  if (!port) {
    return;
  }
  mixer_page.disp_levels[track_number] = velocity;

  uint8_t note = TBD_P4_DEFAULT_STEP_NOTE;
  if (tbd_p4_sound_uses_step_note(p4_sound)) {
    uint8_t locked_note = get_track_lock_implicit(step, TBD_P4_LOCK_NOTE_PARAM);
    if (locked_note <= 127) {
      note = locked_note;
    }
  } else if (p4_sound.trig_note >= 0) {
    note = (uint8_t)p4_sound.trig_note;
  }

  send_active_note_off();
  if (p4_sound.note_cc >= 0 && p4_sound.note_cc <= 127) {
    port->sendCC(p4_sound.midi_channel, (uint8_t)p4_sound.note_cc, note);
  }
  port->sendNoteOn(p4_sound.midi_channel, note, velocity);
  active_note_ = note;
  active_note_channel_ = p4_sound.midi_channel;

  uint16_t tick_time = MidiClock.div192_time;
  gate_ticks_remaining_ =
      tick_time == 0
          ? 1
          : (uint16_t)(((uint32_t)p4_sound.trig_gate_time_ms * 5u +
                        tick_time - 1u) /
                       tick_time);
  if (gate_ticks_remaining_ == 0) {
    gate_ticks_remaining_ = 1;
  }
}

void TBDSeqTrack::service_gate() {
  if (gate_ticks_remaining_ == 0) {
    return;
  }
  gate_ticks_remaining_--;
  if (gate_ticks_remaining_ == 0) {
    send_active_note_off();
  }
}

void TBDSeqTrack::send_active_note_off() {
  if (!port || active_note_ == 255) {
    active_note_ = 255;
    return;
  }
  port->sendNoteOff(active_note_channel_, active_note_);
  active_note_ = 255;
}

void TBDSeqTrack::send_lock_value(uint8_t param, uint8_t value) {
  if (!port) {
    return;
  }

  const TbdP4ParamDescriptor *desc =
      tbd_p4_sound_param_for_lock(p4_sound, param);
  if (!desc || !desc->is_sendable()) {
    return;
  }

  const uint16_t value14 = (uint16_t)(((uint32_t)value * 0x3FFFu + 63u) / 127u);
  const int16_t scaled = tbd_p4_scale_lock_value(*desc, value14);
  if (desc->ctrl_type == TBD_P4_CTRLTYPE_CC) {
    tbd_p4_send_param_value(port, p4_sound.midi_channel, *desc, scaled);
  } else if (desc->ctrl_type == TBD_P4_CTRLTYPE_NRPM) {
    tbd_p4_send_param_value(port, p4_sound.midi_channel, *desc, scaled);
  }
}

void TBDSeqTrack::send_parameter_locks(uint8_t step, uint16_t lock_idx,
                                       bool trig) {
  uint64_t mask = 1ULL;
  for (uint8_t c = 0; c < STEPSEQ_NUM_LOCKS; c++) {
    const bool lock_bit = (steps[step].locks & mask) != 0;
    const bool lock_present = lock_bit && steps[step].locks_enabled;
    if (locks_params[c] != 0) {
      const uint8_t param = locks_params[c] - 1;
      uint8_t value = 0;
      bool send = false;

      if (lock_present && lock_idx < STEPSEQ_NUM_LOCK_SLOTS) {
        value = locks[lock_idx];
        send = true;
      } else if (trig && get_default_lock_value(param, value)) {
        send = true;
      }

      if (send) {
        send_lock_value(param, value);
      }
    }
    if (lock_bit) {
      lock_idx++;
    }
    mask <<= 1;
  }
}

bool TBDSeqTrack::get_default_lock_value(uint8_t param_id,
                                         uint8_t &value) const {
  if (param_id == TBD_P4_LOCK_NOTE_PARAM) {
    if (!tbd_p4_sound_uses_step_note(p4_sound)) {
      return false;
    }
    value = TBD_P4_DEFAULT_STEP_NOTE;
    return true;
  }

  const TbdP4ParamDescriptor *desc =
      tbd_p4_sound_param_for_lock(p4_sound, param_id);
  if (!desc || !desc->is_visible()) {
    return false;
  }
  value = tbd_p4_param_to_lock_value(*desc, desc->value);
  return true;
}

uint8_t TBDSeqTrack::pitch_lock_param() const {
  return TBD_P4_LOCK_NOTE_PARAM;
}

void TBDSeqTrack::clear_step_oneshot(uint8_t step) {
  STEPSEQ_CLEAR_BIT64(oneshot_mask_, step);
}

void TBDSeqTrack::on_modify_track_begin() {
  oneshot_mask_ = 0;
}

void TBDSeqTrack::clear_mutes() {
  oneshot_mask_ = 0;
  TBDSeqDataTrack::clear_mutes();
}

void TBDSeqTrack::dispatch_slide_value(uint8_t param, uint8_t value,
                                       uint8_t channel) {
  (void)channel;
  send_lock_value(param, value);
}

#endif // PLATFORM_TBD
