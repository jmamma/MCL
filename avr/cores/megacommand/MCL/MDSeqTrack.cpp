#include "MCL_impl.h"

uint16_t MDSeqTrack::gui_update = 0;
uint16_t MDSeqTrack::md_trig_mask = 0;
uint32_t MDSeqTrack::load_machine_cache = 0;

void MDSeqTrack::set_length(uint8_t len, bool expand) {
  uint8_t old_length = length;
  if (len == 0) {
    len = 1;
  }
  if (len > 64) {
    len = 16;
  }
  length = len;

  uint8_t step = step_count;
  if (step >= length) {
    step = step % length;
  }

  uint8_t idx = get_lockidx(step);

  USE_LOCK();
  SET_LOCK();
  cur_event_idx = idx;
  step_count = step;
  CLEAR_LOCK();

  if (expand && old_length <= 16 && length >= 16) {
    for (uint8_t n = old_length; n < length; n++) {
      if ((*(int *)&(steps[n])) != 0) {
        expand = false;
        return;
      }
    }
    MDSeqStep empty_step;
    memset(&empty_step, 0, sizeof(empty_step));
    uint8_t a = 0;
    for (uint8_t n = old_length; n < 64; n++) {
      copy_step(a++, &empty_step);
      paste_step(n, &empty_step);
      if (a == old_length) {
        a = 0;
      }
    }
  }
}

void MDSeqTrack::store_mute_state() {
  for (uint8_t n = 0; n < NUM_MD_STEPS; n++) {
    if (IS_BIT_SET64(mute_mask, n)) {
      set_step(n, MASK_PATTERN, 0);
      set_step(n, MASK_LOCK, 0);
    }
  }
  clear_mutes();
}

void MDSeqTrack::set_speed(uint8_t new_speed, uint8_t old_speed,
                           bool timing_adjust) {
  if (old_speed == 255) {
    old_speed = speed;
  }
  if (timing_adjust) {
    float mult =
        get_speed_multiplier(new_speed) / get_speed_multiplier(old_speed);
    for (uint8_t i = 0; i < NUM_MD_STEPS; i++) {
      timing[i] = round(mult * (float)timing[i]);
    }
  }
  speed = new_speed;
  uint8_t timing_mid = get_timing_mid();
  if (mod12_counter > timing_mid) {
    mod12_counter = mod12_counter - (mod12_counter / timing_mid) * timing_mid;
    // step_count_inc();
  }
  re_sync();
}

void MDSeqTrack::re_sync() {
  //  uint32_t q = length * 12;
  //  count_down = (MidiClock.div192th_counter / q) * q + q;
}

void MDSeqTrack::load_cache() {
  /*  MDTrackChunk t;
    DEBUG_PRINTLN("lc");
    for (uint8_t n = 0; n < t.get_chunk_count(); n++) {
      t.load_from_mem_chunk(track_number, n);
      t.load_chunk(data(), n);
    }*/

  MDTrack t;
  t.load_from_mem(track_number, MD_TRACK_TYPE);
  t.load_seq_data((SeqTrack *)this);
  if (load_sound) {
    MD.insertMachineInKit(track_number, &(t.machine), false);
    SET_BIT32(load_machine_cache, track_number);
    load_sound = 0;
  }
}

void MDSeqTrack::seq(MidiUartParent *uart_, MidiUartParent *uart2_) {
  MidiUartParent *uart_old = uart;
  MidiUartParent *uart2_old = uart2;

  uart = uart_;
  uart2 = uart2_;

  uint8_t timing_mid = get_timing_mid();

  mod12_counter++;

  if (mod12_counter == timing_mid) {
    mod12_counter = 0;
    cur_event_idx += popcount(steps[step_count].locks);
    if (ignore_step == step_count) {
      ignore_step = 255;
    }
    step_count_inc();
  }
  if (count_down) {
    count_down--;
    if (count_down == 0) {
      reset();
      mod12_counter = 0;
      SET_BIT16(gui_update, track_number);
    } else if (count_down <= track_number / 4 + 1) {
      if (!cache_loaded) {
        load_cache();
        cache_loaded = true;
      }
      goto end;
    }
  }

  if (notes.count_down) {
    notes.count_down--;
    if (notes.count_down == 0) {
      send_notes_off();
    }
  }

  if (record_mutes) {
    uint8_t u = 0;
    uint8_t q = 0;
    uint8_t s = get_quantized_step(u, q);
    SET_BIT64(mute_mask, s);
  }

  if ((mute_state == SEQ_MUTE_OFF) && (ignore_step != step_count)) {

    uint8_t next_step = 0;
    if (step_count == (length - 1)) {
      next_step = 0;
    } else {
      next_step = step_count + 1;
    }
    uint8_t current_step;

    send_slides(locks_params);

    if (((timing[step_count] >= timing_mid) &&
         (timing[current_step = step_count] - timing_mid == mod12_counter)) ||
        ((timing[next_step] < timing_mid) &&
         ((timing[current_step = next_step]) == mod12_counter))) {

      uint16_t lock_idx = cur_event_idx;
      if (current_step == next_step) {
        if (current_step == 0) {
          lock_idx = 0;
        } else {
          lock_idx += popcount(steps[step_count].locks);
        }
      }
      auto &step = steps[current_step];
      uint8_t send_trig = trig_conditional(step.cond_id);
      bool is_midi_model =
          ((MD.kit.models[track_number] & 0xF0) == MID_01_MODEL);
      if (send_trig == TRIG_TRUE ||
          (!step.cond_plock && send_trig != TRIG_ONESHOT)) {
        if (is_midi_model && send_trig == TRIG_TRUE && step.trig) {
          send_notes_off();
          init_notes();
        }
        send_parameter_locks_inline(current_step, step.trig, lock_idx);
        if (step.slide) {
          locks_slides_recalc = current_step;
          locks_slides_idx = lock_idx;
        }
        if (send_trig == TRIG_TRUE && step.trig) {
          if (is_midi_model) {
            notes.count_down = notes.len == 0 ? timing_mid / 4 : (notes.len * timing_mid / 2);
            send_notes_on();
          }
          send_trig_inline();
        }
      }
    }
  }
end:
  uart = uart_old;
  uart2 = uart2_old;
}

bool MDSeqTrack::is_param(uint8_t param_id) {
  for (uint8_t c = 0; c < NUM_LOCKS; c++) {
    if (locks_params[c] > 0) {
      if (locks_params[c] - 1 == param_id) {
        return true;
      }
    }
  }
  return false;
}

void MDSeqTrack::recalc_slides() {
  if (locks_slides_recalc == 255) {
    return;
  }
  DEBUG_PRINT_FN();
  int16_t x0, x1;
  int8_t y0, y1;
  uint8_t step = locks_slides_recalc;
  uint8_t timing_mid = get_timing_mid();

  uint8_t find_mask = 0;
  uint8_t cur_mask = 1;
  for (uint8_t i = 0; i < NUM_LOCKS; i++) {
    if (locks_params[i] && (steps[step].locks & cur_mask)) {
      find_mask |= cur_mask;
    }
    cur_mask <<= 1;
  }

  auto lockidx = locks_slides_idx;
  if (find_mask == 0) {
    goto end;
  }
  find_next_locks(lockidx, step, find_mask);

  for (uint8_t c = 0; c < NUM_LOCKS; c++) {
    if (!locks_params[c] || !steps[step].is_lock_bit(c)) {
      continue;
    }
    auto cur_lockidx = lockidx++;
    if (!steps[step].locks_enabled) {
      continue;
    }
    auto next_lockstep = locks_slide_next_lock_step[c];
    if (step == next_lockstep) {
      locks_slide_data[c].init();
      continue;
    }
    x0 = step * timing_mid + timing[step] - timing_mid + 1;
    if (next_lockstep < step) {
      x1 = (length + next_lockstep) * timing_mid + timing[next_lockstep] -
           timing_mid - 1;
    } else {
      x1 = next_lockstep * timing_mid + timing[next_lockstep] - timing_mid - 1;
    }
    DEBUG_DUMP(timing[step]);
    DEBUG_DUMP(timing_mid);
    y0 = locks[cur_lockidx];
    y1 = locks_slide_next_lock_val[c];
    prepare_slide(c, x0, x1, y0, y1);
  }
end:
  locks_slides_recalc = 255;
}

void MDSeqTrack::find_next_locks(uint8_t curidx, uint8_t step, uint8_t mask) {
  DEBUG_PRINT_FN();
  DEBUG_DUMP(step);
  // caller ensures step < length
  uint8_t next_step = step + 1;
  uint8_t max_len = length;
  curidx += popcount(steps[step].locks);

again:
  for (; next_step < max_len; next_step++) {
    uint8_t cur_mask = 1;
    auto lcks = get_step_locks(next_step);
    for (uint8_t i = 0; i < NUM_LOCKS; ++i) {

      if (mask & cur_mask) {
        if (lcks & cur_mask) {
          locks_slide_next_lock_val[i] = locks[curidx];
          locks_slide_next_lock_step[i] = next_step;
          mask &= ~cur_mask;
          // all targets hit?
        } else if (steps[next_step].trig) {
          locks_slide_next_lock_val[i] =
              MD.kit.params[track_number][locks_params[i] - 1];
          locks_slide_next_lock_step[i] = next_step;
          mask &= ~cur_mask;
        }
        if (!mask)
          return;
      }
      if (lcks & cur_mask) {
        curidx++;
      }
      cur_mask <<= 1;
    }
  }

  if (next_step >= length) {
    next_step = 0;
    curidx = 0;
    max_len = step;
    goto again;
  }
}

void MDSeqTrack::get_mask(uint64_t *_pmask, uint8_t mask_type) const {
  *_pmask = 0;
  for (uint8_t i = 0; i < NUM_MD_STEPS; i++) {
    bool set_bit = false;
    switch (mask_type) {
    case MASK_PATTERN:
      if (steps[i].trig) {
        set_bit = true;
      }
      break;
    case MASK_LOCKS_ON_STEP:
      if (steps[i].locks) {
        set_bit = true;
      }
      break;
    case MASK_LOCK:
      if (steps[i].locks_enabled) {
        set_bit = true;
      }
      break;
    case MASK_SLIDE:
      if (steps[i].slide) {
        set_bit = true;
      }
      break;
    case MASK_MUTE:
      if (IS_BIT_SET64(mute_mask, i)) {
        set_bit = true;
      }
      break;
    }
    if (set_bit) {
      SET_BIT64_P(_pmask, i);
    }
  }
}

bool MDSeqTrack::get_step(uint8_t step, uint8_t mask_type) const {
  switch (mask_type) {
  case MASK_PATTERN:
    return steps[step].trig;
  case MASK_LOCK:
    return steps[step].locks_enabled;
  case MASK_MUTE:
    return IS_BIT_SET64(mute_mask, step);
  case MASK_SLIDE:
    return steps[step].slide;
  default:
    return false;
  }
}

void MDSeqTrack::set_step(uint8_t step, uint8_t mask_type, bool val) {
  switch (mask_type) {
  case MASK_PATTERN:
    steps[step].trig = val;
    break;
  case MASK_LOCK:
    steps[step].locks_enabled = val;
    break;
  case MASK_MUTE:
    if (val) {
      SET_BIT64(mute_mask, step);
    } else {
      CLEAR_BIT64(mute_mask, step);
    }
    break;
  case MASK_SLIDE:
    steps[step].slide = val;
    break;
  }
}

void MDSeqTrack::send_parameter_locks(uint8_t step, bool trig,
                                      uint16_t lock_idx) {
  uint16_t idx, end;
  if (lock_idx == 0xFFFF) {
    idx = get_lockidx(step);
  } else {
    idx = lock_idx;
  }
  send_parameter_locks_inline(step, trig, idx);
}

void MDSeqTrack::send_notes_ccs(uint8_t *ccs, bool send_ccs) {
  uint8_t channel = MD.kit.models[track_number] - MID_01_MODEL;
  if (send_ccs) {
    for (uint8_t n = 0; n < number_midi_cc; n++) {
    if (ccs[n] == 255) continue;
    switch (n) {
      case 1:
          uart2->sendPitchBend(channel, ccs[1] << 7);
        break;
      case 2:
          uart2->sendCC(channel, 0x1, ccs[2]);
        break;
      case 3:
          uart2->sendChannelPressure(channel, ccs[3]);
        break;
      case 0:
          notes.prog = ccs[0];
          uart2->sendProgramChange(channel, ccs[0]);
        break;
      default:
        if (!(n & 1)) continue;
        uint8_t a = ccs[n - 1];
        if (a > 0 && a != 255) {
          uint8_t v = ccs[n];
          // 0 = off
          // 1 = bank (0)
          // 2 = 2
          if (a == 1) {
            a = 0;
          };
          uart2->sendCC(channel, a, v);
          break;
        }
      }
    }
  }
}

void MDSeqTrack::process_note_locks(uint8_t param, uint8_t val, uint8_t *ccs,
                                    bool is_lock) {
  uint8_t channel = MD.kit.models[track_number] - MID_01_MODEL;

  uint8_t i = param - 5;
  switch (param) {
  case 0:
  case 1:
  case 2:
  case 3:
  case 4:
    //note values are set via init_notes initially.
    ((uint8_t *)&notes)[param] = val;
    //ccs[param] = val; not required.
    break;
  case 5:
  case 6:
  case 7:
    ccs[i + 1] = val;
    break;
  case 20:
    if (notes.prog != val || is_lock) {
      ccs[0] = val;
    }
    else {
      ccs[0] = 255;
    }
    break;
  default:
    i = param - 8 + 4;
    if (param < 20) {
      // If the parameter is CC value and the CC dest is not yet set, use the
      // kit value for CC dest
      uint8_t j = i - 1;
      if ((param & 1) && ccs[j] == 255) {
        ccs[j] = MD.kit.params[track_number][param - 1];
      }
      ccs[i] = val;
    }
    break;
  }
}

void MDSeqTrack::send_parameter_locks_inline(uint8_t step, bool trig,
                                             uint16_t lock_idx) {

  uint8_t ccs[midi_cc_array_size];
  bool send_ccs = false;
  bool is_midi_model = (MD.kit.models[track_number] & 0xF0) == MID_01_MODEL;

  if (is_midi_model) {
    if (notes.first_trig) {
      // first note, we want to send all CCs regardless if they dont have locks.
      memcpy(ccs + 1, &MD.kit.params[track_number][5], sizeof(ccs) - 1);
      //prevent re-transmission of program change.
      //process_note_locks(20, MD.kit.params[track_number][20],ccs);
      send_ccs = true;
      notes.first_trig = false;
    } else {
      memset(ccs, 255, sizeof(ccs));
    }
  }
  for (uint8_t c = 0; c < NUM_LOCKS; c++) {
    bool lock_bit = steps[step].is_lock_bit(c);
    bool lock_present = steps[step].is_lock(c);
    bool send = false;
    uint8_t val;
    uint8_t p = locks_params[c] - 1;
    if (locks_params[c]) {
      if (lock_present) {
        val = locks[lock_idx];
        send = true;
      } else if (trig) {
        val = MD.kit.params[track_number][p];
        send = true;
      }
    }
    lock_idx += lock_bit;
    if (send) {
      if (is_midi_model && p < 21) {
        process_note_locks(p, val, ccs, true);
        send_ccs |= (p > 4 && p < 8) | (p > 8) && (p & 1) | (p == 20);
      }

      else {
        bool update_kit = false;
        MD.setTrackParam_inline(track_number, p, val, uart, update_kit);
      }
    }
  }
  if (is_midi_model) {
    send_notes_ccs(ccs, send_ccs);
  }
}

void MDSeqTrack::reset_params() {
  bool re_assign = false;
  bool is_midi_model = ((MD.kit.models[track_number] & 0xF0) == MID_01_MODEL);
  if (is_midi_model) {
    uint8_t ccs[midi_cc_array_size];
    bool send_ccs = true;
    memcpy(ccs + 1, &MD.kit.params[track_number][5], sizeof(ccs) - 1);
    ccs[0] = 255; //disable program change
    //notes.prog = MD.kit.params[track_number][20];
    //process_note_locks(20, MD.kit.params[track_number][20],ccs);
    send_notes_ccs(ccs, send_ccs);
  } else {
    MDTrack md_track;
    md_track.get_machine_from_kit(track_number);
    MD.assignMachineBulk(track_number, &md_track.machine, 255, 1, true);
  }

  /*
  for (uint8_t c = 0; c < NUM_LOCKS; c++) {
    bool send = false;
    uint8_t send_param;
    if (locks_params[c]) {
      uint8_t p = locks_params[c] - 1;
      uint8_t send_param = MD.kit.params[track_number][p];
      bool update_kit = false;
      MD.setTrackParam_inline(track_number, p, send_param, uart, update_kit);
    }
  }
*/
}

void MDSeqTrack::get_step_locks(uint8_t step, uint8_t *params,
                                bool ignore_locks_enabled) {
  uint16_t lock_idx = get_lockidx(step);
  for (uint8_t c = 0; c < NUM_LOCKS; c++) {
    bool lock_bit = steps[step].is_lock_bit(c);
    bool lock_present =
        lock_bit & (steps[step].locks_enabled || ignore_locks_enabled);
    if (locks_params[c]) {
      uint8_t param = locks_params[c] - 1;
      if (lock_present) {
        params[param] = locks[lock_idx];
      }
    }
    lock_idx += lock_bit;
  }
}

void MDSeqTrack::send_notes(uint8_t note1, MidiUartParent *uart2_) {
  if (!uart2_) { uart2_ = uart2; }
  if (notes.count_down) {
    send_notes_off(uart2_);
  }
  init_notes();
  if (note1 != 255) { notes.note1 = note1; }
  if (notes.first_trig) { reset_params(); notes.first_trig = false; }
  uint8_t timing_mid = get_timing_mid();
  notes.count_down = notes.len == 0 ? timing_mid / 4 : (notes.len * timing_mid / 2);
  send_notes_on(uart2_);
}

void MDSeqTrack::send_notes_on(MidiUartParent *uart2_) {
  if (!uart2_) { uart2_ = uart2; }
  TrigNotes *n = &notes;
  uint8_t channel = MD.kit.models[track_number] - MID_01_MODEL;

  if (n->note1 != 255) {
    mixer_page.trig(track_number);
    uart2_->sendNoteOn(channel, n->note1, n->vel);
    if (n->note2 != 64) {
      uart2_->sendNoteOn(channel, n->note1 + n->note2 - 64, n->vel);
    }
    if (n->note3 != 64) {
      uart2_->sendNoteOn(channel, n->note1 + n->note3 - 64, n->vel);
    }
  }
}

void MDSeqTrack::send_notes_off(MidiUartParent *uart2_) {
  if (!uart2_) { uart2_ = uart2; }
  TrigNotes *n = &notes;
  uint8_t channel = MD.kit.models[track_number] - MID_01_MODEL;

  if (n->note1 != 255) {
    uart2_->sendNoteOff(channel, n->note1);
    if (n->note2 != 64) {
      uart2_->sendNoteOff(channel, n->note1 + n->note2 - 64);
    }
    if (n->note3 != 64) {
      uart2_->sendNoteOff(channel, n->note1 + n->note3 - 64);
    }
    n->note1 = 255;
  }
  n->count_down = 0;
}

void MDSeqTrack::onControlChangeCallback_Midi(uint8_t track_param,
                                              uint8_t value) {
  bool is_midi_model = ((MD.kit.models[track_number] & 0xF0) == MID_01_MODEL);
  if (!is_midi_model || MD.encoder_interface) {
    return;
  }
  uint8_t ccs[midi_cc_array_size];
  // memset(ccs, 0, sizeof(ccs));
  // Ignore notes, len and vel (those will be obtained from kit upon init_notes
  if (track_param > 4 && track_param < 21) {
    if (!(track_param & 1) && track_param > 7 && track_param < 20) {
      return;
    } // ignore cc destination
    // memcpy(ccs, &MD.kit.params[track_number][8], sizeof(ccs));
    memset(ccs, 255, sizeof(ccs));
    process_note_locks(track_param, value, ccs);
    send_notes_ccs(ccs, true);
  }
}

void MDSeqTrack::send_trig() { send_trig_inline(); }

void MDSeqTrack::send_trig_inline() {
  mixer_page.trig(track_number);
  // MD.triggerTrack(track_number, 127, uart);
  // Parallel trig:
  SET_BIT16(MDSeqTrack::md_trig_mask, track_number);
}

uint8_t MDSeqTrack::trig_conditional(uint8_t condition) {

  bool send_trig = TRIG_FALSE;
  if (IS_BIT_SET64(oneshot_mask, step_count) ||
      IS_BIT_SET64(mute_mask, step_count)) {
    return TRIG_ONESHOT;
  }
  if (condition == 14) {
    if (!IS_BIT_SET64(oneshot_mask, step_count)) {
      SET_BIT64(oneshot_mask, step_count);
      send_trig = TRIG_TRUE;
    }
  } else {
    send_trig = SeqTrack::conditional(condition);
  }
  return send_trig;
}

uint8_t MDSeqTrack::get_track_lock_implicit(uint8_t step, uint8_t param) {
  uint8_t lock_idx = find_param(param);
  if (lock_idx < NUM_LOCKS) {
    return get_track_lock(step, lock_idx);
  }
  return 255;
}

uint8_t MDSeqTrack::get_track_lock(uint8_t step, uint8_t lock_idx) {
  auto idx = get_lockidx(step, lock_idx);
  if (idx < NUM_MD_LOCK_SLOTS && steps[step].locks_enabled) {
    return locks[idx];
  } else {
    return 255;
  }
}

bool MDSeqTrack::set_track_locks(uint8_t step, uint8_t track_param,
                                 uint8_t value) {
  // Stopwatch sw;

  // Let's try and find an existing param
  uint8_t match = find_param(track_param);
  // Then, we learn first NUM_LOCKS params then stop.
  for (uint8_t c = 0; c < NUM_LOCKS && match == 255; c++) {
    if (locks_params[c] == 0) {
      locks_params[c] = track_param + 1;
      match = c;
    }
  }

  if (match != 255) {
    auto ret = set_track_locks_i(step, match, value);
    // auto set_lock = sw.elapsed();
    // DIAG_MEASURE(1, set_lock);
    return ret;
  } else {
    return false;
  }
}

bool MDSeqTrack::set_track_locks_i(uint8_t step, uint8_t lockidx,
                                   uint8_t value) {
  auto lock_slot = get_lockidx(step, lockidx);
  if (lock_slot == NUM_MD_LOCK_SLOTS) {
    auto idx = get_lockidx(step);
    auto nlock = popcount(steps[step].locks & ((1 << lockidx) - 1));
    lock_slot = idx + nlock;

    if (lock_slot >= NUM_MD_LOCK_SLOTS) {
      return false; // memory full!
    }

    memmove(locks + lock_slot + 1, locks + lock_slot,
            NUM_MD_LOCK_SLOTS - lock_slot - 1);
    if (step < step_count) {
      cur_event_idx++;
    }
    steps[step].locks |= (1 << lockidx);
  }
  locks[lock_slot] = min(127, value);
  steps[step].locks_enabled = true;
  return true;
}

void MDSeqTrack::record_track_locks(uint8_t track_param, uint8_t value) {

  if (step_count >= length) {
    return;
  }
  uint8_t utiming = 0;
  set_track_locks(get_quantized_step(utiming), track_param, value);
}

void MDSeqTrack::set_track_pitch(uint8_t step, uint8_t pitch) {
  set_track_locks(step, 0, pitch);
}

void MDSeqTrack::record_track_pitch(uint8_t pitch) {

  if (step_count >= length) {
    return;
  }
  uint8_t utiming = 0;
  set_track_pitch(get_quantized_step(utiming), pitch);
}

void MDSeqTrack::record_track(uint8_t velocity) {

  if (step_count >= length) {
    return;
  }
  uint8_t utiming = 0;
  uint8_t step = get_quantized_step(utiming);
  ignore_step = step;

  set_track_step(step, utiming, velocity);
}

void MDSeqTrack::set_track_step(uint8_t step, uint8_t utiming,
                                uint8_t velocity) {
  uint8_t condition = 0;

  //  timing = 3;
  // condition = 3;
  if (MidiClock.state != 2) {
    return;
  }

  CLEAR_BIT64(oneshot_mask, step);
  steps[step].trig = true;
  // TODO cond value?
  steps[step].cond_id = 0;
  steps[step].cond_plock = false;
  timing[step] = utiming;
  if (velocity < 127) {
    set_track_locks(step, MODEL_VOL, velocity);
  }
}

void MDSeqTrack::clear_slide_data() {
  for (uint8_t i = 0; i < NUM_MD_STEPS; ++i) {
    steps[i].slide = false;
  }
}

void MDSeqTrack::clear_step_lock(uint8_t step, uint8_t param_id) {
  uint8_t match = find_param(param_id);

  if (match == 255)
    return;

  uint8_t mask = (1 << match);
  uint8_t idx = get_lockidx(step);
  uint8_t locks_ = steps[step].locks;

  if (!(steps[step].locks & mask)) {
    return;
  }

  uint8_t offset = popcount(locks_ & (mask - 1));

  memmove(locks + idx + offset, locks + idx + offset + 1,
          NUM_MD_LOCK_SLOTS - idx - offset - 1);

  steps[step].locks &= ~(mask);

  if (steps[step].locks == 0) {
    steps[step].locks_enabled = false;
  }
  if (step < step_count) {
    cur_event_idx -= 1;
  }

  clean_params();
}

void MDSeqTrack::clear_param_locks(uint8_t param_id) {
  uint8_t match = find_param(param_id);
  if (match == 255)
    return;

  uint8_t mask = 1 << match;
  uint8_t nmask = ~mask;
  uint8_t rmask = mask - 1;
  uint8_t idx = 0;
  bool remove[NUM_MD_STEPS];

  // pass1, mark
  for (uint8_t x = 0; x < NUM_MD_STEPS; x++) {
    uint8_t _locks = steps[x].locks;
    uint8_t nlocks = popcount(_locks);
    if (_locks & mask) {
      remove[x] = true;
      steps[x].locks &= nmask;
    } else {
      remove[x] = false;
    }
    idx += nlocks;
  }

  // pass2, sweep
  uint8_t rd = 0;
  uint8_t wr = 0;
  for (uint8_t i = 0; i < NUM_MD_STEPS; ++i) {
    uint8_t _locks = steps[i].locks;
    uint8_t nlocks = popcount(_locks);
    uint8_t skip = NUM_LOCKS;
    if (remove[i]) {
      // how many before me?
      skip = popcount(_locks & rmask);
    }
    for (uint8_t j = 0; j < nlocks; ++j) {
      if (skip == j) {
        ++rd;
      } else {
        locks[wr++] = locks[rd++];
      }
    }
  }

  MD.setTrackParam(track_number, param_id,
                   MD.kit.params[track_number][locks_params[match] - 1]);
}

void MDSeqTrack::clear_step_locks(uint8_t step) {
  uint8_t idx = get_lockidx(step);
  uint8_t cnt = popcount(steps[step].locks);
  if (cnt != 0) {
    memmove(locks + idx, locks + idx + cnt, NUM_MD_LOCK_SLOTS - idx - cnt);
    if (step < step_count) {
      cur_event_idx -= cnt;
    }
  }
  steps[step].locks = 0;
  steps[step].locks_enabled = false;
}

void MDSeqTrack::disable_step_locks(uint8_t step) {
  steps[step].locks_enabled = false;
}

void MDSeqTrack::enable_step_locks(uint8_t step) {
  steps[step].locks_enabled = true;
}

uint8_t MDSeqTrack::get_step_locks(uint8_t step) {
  return steps[step].locks_enabled ? steps[step].locks : 0;
}

void MDSeqTrack::clear_mute() { mute_mask = 0; }

void MDSeqTrack::clear_mutes() {
  oneshot_mask = 0;
  mute_mask = 0;
}

void MDSeqTrack::clear_conditional() {
  for (uint8_t c = 0; c < NUM_MD_STEPS; c++) {
    steps[c].cond_id = 0;
    steps[c].cond_plock = 0;
    timing[c] = 0;
  }
  clear_mutes();
  ignore_step = 255;
}

void MDSeqTrack::clear_locks() {
  // Need to buffer this, as we dont want sequencer interrupt
  // to access it whilst we're cleaning up
  DEBUG_DUMP("Clear these locks");
  for (uint8_t c = 0; c < NUM_LOCKS; c++) {
    locks_params[c] = 0;
  }

  memset(locks, 0, sizeof(locks));
  cur_event_idx = 0;
  notes.first_trig = true;
}

void MDSeqTrack::clear_track(bool locks) {
  clear_conditional();
  if (locks) {
    DEBUG_DUMP("clear locks");
    clear_locks();
  }
  memset(steps, 0, sizeof(steps));
  notes.first_trig = true;
}

void MDSeqTrack::merge_from_md(uint8_t track_number, MDPattern *pattern) {
  DEBUG_PRINT_FN();
  for (uint8_t i = 0; i < 24; i++) {
    if (!IS_BIT_SET32(pattern->lockPatterns[track_number], i)) {
      continue;
    }
    int8_t idx = pattern->paramLocks[track_number][i];
    if (idx < 0) {
      continue;
    }
    for (uint8_t s = 0; s < 64; s++) {
      int8_t lockval = pattern->locks[idx][s];
      if (lockval >= 0 &&
          IS_BIT_SET64(pattern->trigPatterns[track_number], s)) {
        set_track_locks(s, i, lockval);
      }
    }
  }

  uint8_t *ppattern = (uint8_t *)&pattern->trigPatterns[track_number];
  uint8_t *pslide;

  if (pattern->slideEditAll > 0) {
    pslide = (uint8_t *)&pattern->slidePattern;
  } else {
    pslide = (uint8_t *)&pattern->slidePatterns[track_number];
  }

  // 32770.0 is scalar to get MD swing amount in to readible percentage
  // MD sysex docs are not clear on this one so i had to hax it.

  float swing = (float)pattern->swingAmount / 16385.0;

  uint8_t *pswingpattern;
  uint8_t timing_mid = get_timing_mid();
  if (pattern->swingEditAll > 0) {
    pswingpattern = (uint8_t *)&pattern->swingPattern;
  } else {
    pswingpattern = (uint8_t *)&pattern->swingPatterns[track_number];
  }

  for (uint8_t a = 0; a < length; a++) {
    if (IS_BIT_SET64_P(pslide, a)) {
      steps[a].slide = true;
    }
    if (IS_BIT_SET64_P(ppattern, a)) {
      steps[a].trig = true;
      steps[a].cond_id = 0;
      steps[a].cond_plock = false;
      timing[a] = timing_mid;
      if (IS_BIT_SET64_P(pswingpattern, a)) {
        timing[a] += round(swing * timing_mid);
      }
    }
  }
}

void MDSeqTrack::modify_track(uint8_t dir) {

  uint8_t old_mute_state = mute_state;

  oneshot_mask = 0;
  constexpr size_t ncopy = sizeof(steps) - sizeof(MDSeqStepDescriptor);
  uint8_t lock_buf[NUM_LOCKS];
  MDSeqStepDescriptor step_buf;
  uint8_t timing_buf;
  uint16_t total_nlock = get_lockidx(length);

  mute_state = SEQ_MUTE_ON;
  switch (dir) {
  case DIR_LEFT: {
    // shift locks
    uint8_t nlock = popcount(steps[0].locks);
    memcpy(lock_buf, locks, nlock);
    memmove(locks, locks + nlock, total_nlock - nlock);
    memcpy(locks + total_nlock - nlock, lock_buf, nlock);

    // shift steps
    step_buf = steps[0];
    timing_buf = timing[0];
    memmove(steps, steps + 1, ncopy);
    memmove(timing, timing + 1, length - 1);
    steps[length - 1] = step_buf;
    timing[length - 1] = timing_buf;
    ROTATE_LEFT(mute_mask, length);
    break;
  }
  case DIR_RIGHT: {
    // shift locks
    uint8_t nlock = popcount(steps[length - 1].locks);
    memcpy(lock_buf, locks + total_nlock - nlock, nlock);
    memmove(locks + nlock, locks, total_nlock - nlock);
    memcpy(locks, lock_buf, nlock);

    // shift steps
    step_buf = steps[length - 1];
    timing_buf = timing[length - 1];
    memmove(steps + 1, steps, ncopy);
    memmove(timing + 1, timing, length - 1);
    steps[0] = step_buf;
    timing[0] = timing_buf;
    ROTATE_RIGHT(mute_mask, length);
    break;
  }
  case DIR_REVERSE: {
    uint8_t rev_locks[NUM_MD_LOCK_SLOTS];
    memcpy(rev_locks, locks, sizeof(locks));
    uint16_t l = 0, r = 0;
    // mute_mask = 0; //unimplemented
    //  reverse steps & locks
    for (uint8_t i = 0; i <= length / 2; ++i) {
      int j = length - i - 1;
      if (j < i) {
        break;
      }
      uint8_t ni = popcount(steps[i].locks);
      uint8_t nj = popcount(steps[j].locks);
      memcpy(locks + l, rev_locks + total_nlock - l - nj, nj);
      memcpy(locks + total_nlock - r - ni, rev_locks + r, ni);
      l += nj;
      r += ni;
      step_buf = steps[i];
      steps[i] = steps[j];
      steps[j] = step_buf;
      timing_buf = timing[i];
      timing[i] = timing[j];
      timing[j] = timing_buf;
      bool a = IS_BIT_SET64(mute_mask, i);
      bool b = IS_BIT_SET64(mute_mask, j);
      if (a) {
        SET_BIT64(mute_mask, j);
      } else {
        CLEAR_BIT64(mute_mask, j);
      }
      if (b) {
        SET_BIT64(mute_mask, i);
      } else {
        CLEAR_BIT64(mute_mask, i);
      }
    }
    break;
  }
  }
  cur_event_idx = get_lockidx(step_count);
  mute_state = old_mute_state;
}

void MDSeqTrack::copy_step(uint8_t n, MDSeqStep *step) {
  step->active = true;
  step->timing = timing[n];

  uint8_t idx = get_lockidx(n);
  uint8_t lcks = steps[n].locks;
  uint8_t mask = 1;
  for (uint8_t a = 0; a < NUM_LOCKS; a++) {
    if (lcks & mask) {
      step->locks[a] = locks[idx++] + 1;
    } else {
      step->locks[a] = 0;
    }
    mask <<= 1;
  }

  memcpy(&step->data, &(steps[n]), sizeof(MDSeqStepDescriptor));
}

void MDSeqTrack::paste_step(uint8_t n, MDSeqStep *step) {
  clear_step_locks(n);
  timing[n] = step->timing;

  for (uint8_t a = 0; a < NUM_LOCKS; a++) {
    if (step->locks[a] != 0) {
      set_track_locks(n, locks_params[a] - 1, step->locks[a] - 1);
    }
  }
  memcpy(&(steps[n]), &step->data, sizeof(MDSeqStepDescriptor));
}

uint8_t MDSeqTrack::transpose_pitch(uint8_t pitch, int8_t offset) {
 uint8_t note_num = seq_ptc_page.get_note_from_machine_pitch(track_number,pitch);
 if (note_num == 255) { return pitch; }
 int16_t new_note = note_num + offset;
 new_note = max(0,min(127,new_note));
 uint8_t new_pitch = seq_ptc_page.get_machine_pitch(track_number, new_note);
 if (new_pitch == 255) { new_pitch = pitch; }
 return new_pitch;
}

void MDSeqTrack::transpose(int8_t offset) {
 bool is_midi_model = ((MD.kit.models[track_number] & 0xF0) == MID_01_MODEL);
 tuning_t const *tuning = MD.getKitModelTuning(track_number);
 if (!tuning && !is_midi_model) { return; }
 for (uint8_t n = 0; n < 64; n++) {
   uint8_t pitch = get_track_lock_implicit(n, 0);
   if (pitch == 255) { continue; }
   set_track_pitch(n, transpose_pitch(pitch, offset));
 }
 MD.setTrackParam(track_number,0,transpose_pitch(MD.kit.params[track_number][0], offset), nullptr, true);
}
