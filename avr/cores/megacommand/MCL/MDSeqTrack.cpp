#include "MCL_impl.h"

uint16_t MDSeqTrack::sync_cursor = 0;
uint16_t MDSeqTrack::md_trig_mask = 0;

void MDSeqTrack::set_length(uint8_t len, bool expand) {
  uint8_t old_length = length;
  length = len;
  while (step_count >= length && length > 0) {
    // re_sync();
    step_count = (step_count - length);
  }
  if (expand && old_length <= 16 && length > 16) {
    for (uint8_t n = 16; n < length; n++) {
      if ((*(int *)&(steps[n])) != 0) {
        expand = false;
        break;
      }
    }
    if (expand) {
      MDSeqStep empty_step;
      memset(&empty_step, 0, sizeof(empty_step));
      for (uint8_t y = 1; y < 4; y++) {
        for (uint8_t n = 0; n < 16; n++) {
          copy_step(n, &empty_step);
          paste_step(n + y * 16, &empty_step);
        }
      }
    }
  }
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

void MDSeqTrack::seq(MidiUartParent *uart_) {
  MidiUartParent *uart_old = uart;
  uart = uart_;

  if (count_down) {
    count_down--;
    if (count_down == 0) {
      reset();
      SET_BIT16(sync_cursor,track_number);
    }
  }

  uint8_t timing_mid = get_timing_mid_inline();

  if ((count_down == 0) && (mute_state == SEQ_MUTE_OFF)) {

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
        if (current_step == 0) { lock_idx = 0; }
        else { lock_idx += popcount(steps[step_count].locks); }
      }

      auto &step = steps[current_step];
      bool send_trig = trig_conditional(step.cond_id);
      if (send_trig || !step.cond_plock) {
        send_parameter_locks_inline(current_step, step.trig, lock_idx);
        if (step.slide) {
          locks_slides_recalc = current_step;
          locks_slides_idx = lock_idx;
        }
        if (send_trig && step.trig) {
          send_trig_inline();
        }
      }
    }
  }
  mod12_counter++;

  if (mod12_counter == timing_mid) {
    mod12_counter = 0;
    cur_event_idx += popcount(steps[step_count].locks);
    step_count_inc();
  }
  uart = uart_old;
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

void MDSeqTrack::update_param(uint8_t param_id, uint8_t value) {
  bool match = false;
  for (uint8_t c = 0; c < NUM_LOCKS && match == false; c++) {
    if (locks_params[c] > 0) {
      if (locks_params[c] - 1 == param_id) {
        locks_params_orig[c] = value;
        match = true;
      }
    }
  }
}

void MDSeqTrack::update_kit_params() {
  for (uint8_t c = 0; c < NUM_LOCKS; c++) {
    if (locks_params[c] > 0) {
      uint8_t param_id = locks_params[c] - 1;
      MD.kit.params[track_number][param_id] = locks_params_orig[c];
    }
  }
}

void MDSeqTrack::update_params() {
  for (uint8_t c = 0; c < NUM_LOCKS; c++) {
    if (locks_params[c] > 0) {
      uint8_t param_id = locks_params[c] - 1;
      locks_params_orig[c] = MD.kit.params[track_number][param_id];
    }
  }
}

void MDSeqTrack::reset_params() {
  MDTrack md_track;

  md_track.get_machine_from_kit(track_number);
  bool re_assign = false;
  for (uint8_t c = 0; c < NUM_LOCKS; c++) {
    if (locks_params[c] > 0) {
      re_assign = true;
      md_track.machine.params[locks_params[c] - 1] = locks_params_orig[c];
    }
  }
  if (re_assign) { MD.assignMachineBulk(track_number, &md_track.machine, 255, 1, true); }
}

void MDSeqTrack::recalc_slides() {
  if (locks_slides_recalc == 255) {
    return;
  }
  DEBUG_PRINT_FN();
  int16_t x0, x1;
  int8_t y0, y1;
  uint8_t step = locks_slides_recalc;
  uint8_t timing_mid = get_timing_mid_inline();

  uint8_t find_mask = 0;
  uint8_t cur_mask = 1;
  for (uint8_t i = 0; i < NUM_LOCKS; i++) {
    if (locks_params[i] && (steps[step].locks & cur_mask)) {
      find_mask |= cur_mask;
    }
    cur_mask <<= 1;
  }

  auto lockidx = locks_slides_idx;
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
          locks_slide_next_lock_val[i] = locks_params_orig[i];
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
  for (int i = 0; i < NUM_MD_STEPS; i++) {
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
      if (IS_BIT_SET64(oneshot_mask, i)) {
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
    return IS_BIT_SET64(oneshot_mask, step);
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
      SET_BIT64(oneshot_mask, step);
    } else {
      CLEAR_BIT64(oneshot_mask, step);
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

void MDSeqTrack::send_parameter_locks_inline(uint8_t step, bool trig,
                                             uint16_t lock_idx) {
  for (uint8_t c = 0; c < NUM_LOCKS; c++) {
    bool lock_bit = steps[step].is_lock_bit(c);
    bool lock_present = steps[step].is_lock(c);
    bool send = false;
    uint8_t send_param;
    if (locks_params[c]) {
      if (lock_present) {
        send_param = locks[lock_idx];
        send = true;
      } else if (trig) {
        send_param = locks_params_orig[c];
        send = true;
      }
    }
    lock_idx += lock_bit;
    if (send) {
      MD.setTrackParam_inline(track_number, locks_params[c] - 1, send_param,
                              uart);
    }
  }
}

void MDSeqTrack::get_step_locks(uint8_t step, uint8_t *params, bool ignore_locks_enabled) {
  uint16_t lock_idx = get_lockidx(step);
  for (uint8_t c = 0; c < NUM_LOCKS; c++) {
    bool lock_bit = steps[step].is_lock_bit(c);
    bool lock_present = lock_bit & (steps[step].locks_enabled || ignore_locks_enabled);
    if (locks_params[c]) {
      uint8_t param = locks_params[c] - 1;
      if (lock_present) {
        params[param] = locks[lock_idx];
      }
    }
    lock_idx += lock_bit;
  }
}

void MDSeqTrack::send_trig() { send_trig_inline(); }

void MDSeqTrack::send_trig_inline() {
  mixer_page.disp_levels[track_number] = MD.kit.levels[track_number];
  if (MD.kit.trigGroups[track_number] < 16) {
    mixer_page.disp_levels[MD.kit.trigGroups[track_number]] =
        MD.kit.levels[MD.kit.trigGroups[track_number]];
  }
  //  MD.triggerTrack(track_number, 127, uart);
  SET_BIT16(MDSeqTrack::md_trig_mask, track_number);
}

bool MDSeqTrack::trig_conditional(uint8_t condition) {
  bool send_trig = false;
  if (IS_BIT_SET64(oneshot_mask, step_count)) {
    return false;
  }
  switch (condition) {
  case 0:
  case 1:
    send_trig = true;
    break;
  case 2:
    if (!IS_BIT_SET(iterations_8, 0)) {
      send_trig = true;
    }
    break;
  case 3:
    if ((iterations_6 == 3) || (iterations_6 == 6)) {
      send_trig = true;
    }
    break;
  case 6:
    if (iterations_6 == 6) {
      send_trig = true;
    }
    break;
  case 4:
    if ((iterations_8 == 4) || (iterations_8 == 8)) {
      send_trig = true;
    }
    break;
  case 8:
    if ((iterations_8 == 8)) {
      send_trig = true;
    }
    break;
  case 5:
    if (iterations_5 == 5) {
      send_trig = true;
    }
    break;
  case 7:
    if (iterations_7 == 7) {
      send_trig = true;
    }
    break;
  case 9:
    if (get_random_byte() <= 13) {
      send_trig = true;
    }
    break;
  case 10:
    if (get_random_byte() <= 32) {
      send_trig = true;
    }
    break;
  case 11:
    if (get_random_byte() <= 64) {
      send_trig = true;
    }
    break;
  case 12:
    if (get_random_byte() <= 96) {
      send_trig = true;
    }
    break;
  case 13:
    if (get_random_byte() <= 115) {
      send_trig = true;
    }
    break;
  case 14:
    if (!IS_BIT_SET64(oneshot_mask, step_count)) {
      SET_BIT64(oneshot_mask, step_count);
      send_trig = true;
    }
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
    return locks_params_orig[lock_idx];
  }
  return 255;
}

bool MDSeqTrack::set_track_locks(uint8_t step, uint8_t track_param,
                                 uint8_t value) {
  Stopwatch sw;

  // Let's try and find an existing param
  uint8_t match = find_param(track_param);
  // Then, we learn first NUM_LOCKS params then stop.
  for (uint8_t c = 0; c < NUM_LOCKS && match == 255; c++) {
    if (locks_params[c] == 0) {
      locks_params[c] = track_param + 1;
      locks_params_orig[c] = MD.kit.params[track_number][track_param];
      match = c;
    }
  }

  if (match != 255) {
    auto ret = set_track_locks_i(step, match, value);
    auto set_lock = sw.elapsed();
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

  set_track_locks(step_count, track_param, value);
}

void MDSeqTrack::set_track_pitch(uint8_t step, uint8_t pitch) {
  set_track_locks(step, 0, pitch);
}

void MDSeqTrack::record_track_pitch(uint8_t pitch) {

  if (step_count >= length) {
    return;
  }
  set_track_pitch(step_count, pitch);
}

void MDSeqTrack::record_track(uint8_t velocity) {

  if (step_count >= length) {
    return;
  }
  uint8_t utiming = mod12_counter + get_timing_mid() - 1;
  set_track_step(step_count, utiming, velocity);
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

  for (uint8_t n = 0; n < NUM_MD_STEPS; n++) {
    if (steps[n].locks & mask) {
      return;
    }
  }

  // If no more locks on any step, unset the param
  locks_params[match] = 0;
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

  MD.setTrackParam(track_number, param_id, locks_params_orig[match]);
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

void MDSeqTrack::clear_conditional() {
  for (uint8_t c = 0; c < NUM_MD_STEPS; c++) {
    steps[c].cond_id = 0;
    steps[c].cond_plock = 0;
    timing[c] = 0;
  }
  oneshot_mask = 0;
}

void MDSeqTrack::clear_locks(bool reset_params_) {
  // Need to buffer this, as we dont want sequencer interrupt
  // to access it whilst we're cleaning up
  DEBUG_DUMP("Clear these locks");
  uint8_t locks_params_buf[NUM_LOCKS];
  for (uint8_t c = 0; c < NUM_LOCKS; c++) {
    locks_params_buf[c] = locks_params[c];
    locks_params[c] = 0;
  }

  memset(locks, 0, sizeof(locks));
  if (reset_params_) {
  reset_params();
  }
  cur_event_idx = 0;
}

void MDSeqTrack::clear_track(bool locks, bool reset_params_) {
  clear_conditional();
  if (locks) {
    DEBUG_DUMP("clear locks");
    clear_locks(reset_params_);
  }
  memset(steps, 0, sizeof(steps));
}

void MDSeqTrack::merge_from_md(uint8_t track_number, MDPattern *pattern) {
  DEBUG_PRINT_FN();
  for (int i = 0; i < 24; i++) {
    if (!IS_BIT_SET32(pattern->lockPatterns[track_number], i)) {
      continue;
    }
    int8_t idx = pattern->paramLocks[track_number][i];
    if (idx < 0) {
      continue;
    }
    for (int s = 0; s < 64; s++) {
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
    break;
  }
  case DIR_REVERSE: {
    uint8_t rev_locks[NUM_MD_LOCK_SLOTS];
    memcpy(rev_locks, locks, sizeof(locks));
    uint16_t l = 0, r = 0;
    // reverse steps & locks
    for (int i = 0; i <= length / 2; ++i) {
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
      step->locks[a] = locks[idx++];
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
    if (step->locks[a]) {
      set_track_locks(n, locks_params[a] - 1, step->locks[a]);
    }
  }
  memcpy(&(steps[n]), &step->data, sizeof(MDSeqStepDescriptor));
}
