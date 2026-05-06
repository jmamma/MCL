#include "TBDSeqTrack.h"

#ifdef PLATFORM_TBD

#include "GridTrack.h"

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
  TBDSeqDataTrack::reset();
  mute_mask = 0;
}

void TBDSeqTrack::seq(MidiUartClass *uart_) {
  port = uart_;
  uint16_t tps = get_ticks_per_step();

  tick_counter++;
  if (tick_counter >= tps) {
    tick_counter = 0;
    cur_event_idx += stepseq_popcount(steps[step_count].locks);
    if (ignore_step == step_count) {
      ignore_step = 255;
    }
    step_count_inc();
  }

  if (count_down) {
    count_down--;
    if (count_down == 0) {
      reset();
      tick_counter = 0;
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
  const bool cond_ok = conditional(step.cond_id);

  if (cond_ok || !step.cond_plock) {
    send_parameter_locks(current_step, lock_idx);
  }
  if (cond_ok && trig) {
    send_trig(STEPSEQ_IS_BIT_SET64(accent_mask, current_step) ? 127 : 100);
  }
  record_trig_result(cond_ok && trig);
}

void TBDSeqTrack::send_trig(uint8_t velocity) {
  if (!port) {
    return;
  }
  uint8_t note = p4_sound.trig_note >= 0 ? (uint8_t)p4_sound.trig_note : 60;
  port->sendNoteOn(p4_sound.midi_channel, note, velocity);
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
    int16_t cc_value = scaled;
    if (cc_value < 0) cc_value = 0;
    if (cc_value > 127) cc_value = 127;
    port->sendCC(p4_sound.midi_channel, desc->ctrl, (uint8_t)cc_value);
  } else if (desc->ctrl_type == TBD_P4_CTRLTYPE_NRPM) {
    port->sendNRPN(p4_sound.midi_channel, desc->ctrl, (uint16_t)scaled);
  }
}

void TBDSeqTrack::send_parameter_locks(uint8_t step, uint16_t lock_idx) {
  if (!steps[step].locks_enabled) {
    return;
  }

  uint64_t mask = 1ULL;
  for (uint8_t c = 0; c < STEPSEQ_NUM_LOCKS; c++) {
    if (steps[step].locks & mask) {
      if (locks_params[c] != 0 && lock_idx < STEPSEQ_NUM_LOCK_SLOTS) {
        send_lock_value(locks_params[c] - 1, locks[lock_idx]);
      }
      lock_idx++;
    }
    mask <<= 1;
  }
}

bool TBDSeqTrack::get_default_lock_value(uint8_t param_id,
                                         uint8_t &value) const {
  const TbdP4ParamDescriptor *desc =
      tbd_p4_sound_param_for_lock(p4_sound, param_id);
  if (!desc || !desc->is_visible()) {
    return false;
  }
  value = tbd_p4_param_to_lock_value(*desc, desc->value);
  return true;
}

void TBDSeqTrack::dispatch_slide_value(uint8_t param, uint8_t value,
                                       uint8_t channel) {
  (void)channel;
  send_lock_value(param, value);
}

#endif // PLATFORM_TBD
