#include "MCL.h"
#include "MCLSeq.h"

void MDSeqTrack::set_length(uint8_t len) {
  length = len;
  if (step_count >= length) {
    // re_sync();
    step_count = (step_count % length);
  }
}

void MDSeqTrack::set_speed(uint8_t _speed) {
  uint8_t old_speed = speed;
  float mult = get_speed_multiplier(_speed) / get_speed_multiplier(old_speed);
  for (uint8_t i = 0; i < NUM_MD_STEPS; i++) {
    timing[i] = round(mult * (float)timing[i]);
  }
  speed = _speed;
  uint8_t timing_mid = get_timing_mid();
  if (mod12_counter > timing_mid) {
    mod12_counter = mod12_counter - (mod12_counter / timing_mid) * timing_mid;
    // step_count_inc();
  }
  re_sync();
}

void MDSeqTrack::re_sync() {
  uint16_t q = length;
  start_step = (MidiClock.div16th_counter / q) * q + q;
  start_step_offset = 0;
  mute_until_start = true;
}

void MDSeqTrack::seq() {
  if (mute_until_start) {

    if ((clock_diff(MidiClock.div16th_counter, start_step) == 0)) {
      if (start_step_offset > 0) {
        start_step_offset--;
      } else {
        reset();
      }
    }
  }

  uint8_t timing_mid = get_timing_mid_inline();

  if ((MidiUart.uart_block == 0) && (mute_until_start == false) &&
      (mute_state == SEQ_MUTE_OFF)) {

    uint8_t next_step = 0;
    if (step_count == (length - 1)) {
      next_step = 0;
    } else {
      next_step = step_count + 1;
    }
    uint8_t current_step;

    send_slides();

    if (((timing[step_count] >= timing_mid) &&
         (timing[current_step = step_count] - timing_mid == mod12_counter)) ||
        ((timing[next_step] < timing_mid) &&
         ((timing[current_step = next_step]) == mod12_counter))) {

      auto &step = steps[current_step];
      bool send_trig = trig_conditional(step.cond_id);
      if (send_trig || !step.cond_plock) {
        send_parameter_locks(current_step, step.trig);
        if (step.slide) {
          locks_slides_recalc = current_step;
        }
        if (send_trig && step.trig) {
          send_trig_inline();
        }
      }
    }

    mod12_counter++;

    if (mod12_counter == timing_mid) {
      mod12_counter = 0;
      step_count_inc();
    }
  }
}

bool MDSeqTrack::is_param(uint8_t param_id) {
  for (uint8_t c = 0; c < NUM_MD_LOCKS; c++) {
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
  for (uint8_t c = 0; c < NUM_MD_LOCKS && match == false; c++) {
    if (locks_params[c] > 0) {
      if (locks_params[c] - 1 == param_id) {
        locks_params_orig[c] = value;
        match = true;
      }
    }
  }
}

void MDSeqTrack::update_kit_params() {
  for (uint8_t c = 0; c < NUM_MD_LOCKS; c++) {
    if (locks_params[c] > 0) {
      uint8_t param_id = locks_params[c] - 1;
      MD.kit.params[track_number][param_id] = locks_params_orig[c];
    }
  }
}

void MDSeqTrack::update_params() {
  for (uint8_t c = 0; c < NUM_MD_LOCKS; c++) {
    if (locks_params[c] > 0) {
      uint8_t param_id = locks_params[c] - 1;
      locks_params_orig[c] = MD.kit.params[track_number][param_id];
    }
  }
}

void MDSeqTrack::reset_params() {
  for (uint8_t c = 0; c < NUM_MD_LOCKS; c++) {
    if (locks_params[c] > 0) {
      MD.setTrackParam(track_number, locks_params[c] - 1, locks_params_orig[c]);
      //    MD.setTrackParam(track_number, locks_params[c] - 1,
      //                   MD.kit.params[track_number][locks_params[c] - 1]);
    }
  }
}
#define _swap_int16_t(a, b)                                                    \
  {                                                                            \
    int16_t t = a;                                                             \
    a = b;                                                                     \
    b = t;                                                                     \
  }
#define _swap_int8_t(a, b)                                                     \
  {                                                                            \
    int8_t t = a;                                                              \
    a = b;                                                                     \
    b = t;                                                                     \
  }

void MDSeqTrack::send_slides() {
  for (uint8_t c = 0; c < NUM_MD_LOCKS; c++) {
    if ((locks_params[c] > 0) && (locks_slide_data[c].dy > 0)) {

      uint8_t val;
      val = locks_slide_data[c].y0;
      if (locks_slide_data[c].steep) {
        if (locks_slide_data[c].err > 0) {
          locks_slide_data[c].y0 += locks_slide_data[c].inc;
          locks_slide_data[c].err -= locks_slide_data[c].dx;
        }
        locks_slide_data[c].err += locks_slide_data[c].dy;
        locks_slide_data[c].x0++;
        if (locks_slide_data[c].x0 > locks_slide_data[c].x1) {
          locks_slide_data[c].init();
          break;
        }
      } else {
        uint16_t x0_old = locks_slide_data[c].x0;
        while (locks_slide_data[c].x0 == x0_old) {
          if (locks_slide_data[c].err > 0) {
            locks_slide_data[c].x0 += locks_slide_data[c].inc;
            locks_slide_data[c].err -= locks_slide_data[c].dy;
          }
          locks_slide_data[c].err += locks_slide_data[c].dx;
          locks_slide_data[c].y0++;
          if (locks_slide_data[c].y0 > locks_slide_data[c].y1) {
            locks_slide_data[c].init();
            break;
          }
        }
        if (locks_slide_data[c].yflip != 255) {
          val = locks_slide_data[c].y1 - val + locks_slide_data[c].yflip;
        }
      }

      MD.setTrackParam_inline(track_number, locks_params[c] - 1, 0x7F & val);
    }
  }
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
  for (uint8_t i = 0; i < NUM_MD_LOCKS; i++) {
    if (locks_params[i] && (steps[i].locks & cur_mask)) {
      find_mask |= cur_mask;
    }
    cur_mask <<= 1;
  }

  auto lockidx = get_lockidx(step);
  find_next_locks(lockidx, step, find_mask);

  for (uint8_t c = 0; c < NUM_MD_LOCKS; c++) {
    if (!locks_params[c] || !steps[c].is_lock(c)) {
      continue;
    }
    auto cur_lockidx = lockidx++;
    auto next_lockstep = locks_slide_next_lock_step[c];
    auto next_lockidx = locks_slide_next_lock_idx[c];
    if (step == next_lockstep) {
      locks_slide_data[c].init();
      continue;
    }
    x0 = step * timing_mid + timing[step] - timing_mid + 1;
    if (next_lockstep < step) {
      x1 = (length + next_lockstep) * timing_mid + timing[next_lockstep] - timing_mid -
           1;
    } else {
      x1 = next_lockstep * timing_mid + timing[next_lockstep] - timing_mid - 1;
    }
    DEBUG_DUMP(timing[step]);
    DEBUG_DUMP(timing[next_step]);
    DEBUG_DUMP(timing_mid);
    y0 = locks[cur_lockidx] - 1;
    y1 = locks[next_lockidx] - 1;

    locks_slide_data[c].steep = abs(y1 - y0) < abs(x1 - x0);
    locks_slide_data[c].yflip = 255;
    if (locks_slide_data[c].steep) {
      /* Disable as this use case will not exist.
      if (x0 > x1) {
            _swap_int16_t(x0, x1);
           _swap_int16_t(y0, y1);
      }
      */
    } else {
      if (y0 > y1) {
        _swap_int8_t(y0, y1);
        _swap_int16_t(x0, x1);
        locks_slide_data[c].yflip = y0;
      }
    }
    locks_slide_data[c].dx = x1 - x0;
    locks_slide_data[c].dy = y1 - y0;
    locks_slide_data[c].inc = 1;

    if (locks_slide_data[c].steep) {
      if (locks_slide_data[c].dy < 0) {
        locks_slide_data[c].inc = -1;
        locks_slide_data[c].dy *= -1;
      }
      locks_slide_data[c].dy *= 2;
      locks_slide_data[c].err = locks_slide_data[c].dy - locks_slide_data[c].dx;
      locks_slide_data[c].dx *= 2;
    } else {
      if (locks_slide_data[c].dx < 0) {
        locks_slide_data[c].inc = -1;
        locks_slide_data[c].dx *= -1;
      }

      locks_slide_data[c].dx *= 2;
      locks_slide_data[c].err = locks_slide_data[c].dx - locks_slide_data[c].dy;
      locks_slide_data[c].dy *= 2;
    }
    locks_slide_data[c].y0 = y0;
    locks_slide_data[c].x0 = x0;
    locks_slide_data[c].x1 = x1;
    locks_slide_data[c].y1 = y1;
    DEBUG_DUMP(step);
    DEBUG_DUMP(next_step);
    DEBUG_DUMP(locks_slide_data[c].x0);
    DEBUG_DUMP(locks_slide_data[c].y0);
    DEBUG_DUMP(x1);
    DEBUG_DUMP(y1);
    DEBUG_DUMP(locks_slide_data[c].dx);
    DEBUG_DUMP(locks_slide_data[c].dy);
    DEBUG_DUMP(locks_slide_data[c].steep);
    DEBUG_DUMP(locks_slide_data[c].inc);
    DEBUG_DUMP(locks_slide_data[c].yflip);

  }

  locks_slides_recalc = 255;
}

void MDSeqTrack::find_next_locks(uint8_t curidx, uint8_t step, uint8_t mask) {
  DEBUG_PRINT_FN();
  DEBUG_DUMP(step);
  // caller ensures step < length
  uint8_t next_step = step + 1;
  uint8_t max_len = length;

again:
  for (; next_step < max_len; next_step++) {
    uint8_t cur_mask = 1;
    auto lcks = steps[next_step].locks;
    for(uint8_t i = 0; i < NUM_MD_LOCKS; ++i) {
      auto step_is_lock = lcks & cur_mask;
      if (step_is_lock) {
        if (step_is_lock & mask) {
          mask &= ~cur_mask;
          locks_slide_next_lock_idx[i] = curidx;
          locks_slide_next_lock_step[i] = next_step;
          // all targets hit?
          if (!mask) return;
        }
        ++curidx;
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

void MDSeqTrack::send_parameter_locks(uint8_t step, bool trig) {
  uint8_t c;
  bool lock_mask_step = steps[step].locks;
  uint8_t send_param = 255;

  auto idx = get_lockidx(step);

  if (lock_mask_step && trig) {
    for (c = 0; c < NUM_MD_LOCKS; c++) {
      if (locks_params[c] > 0) {
        if (steps[step].is_lock(c)) {
          send_param = locks[idx++] - 1;
        } else {
          send_param = locks_params_orig[c];
        }
        MD.setTrackParam_inline(track_number, locks_params[c] - 1, send_param);
      }
    }
  }
  else if (lock_mask_step) {
    for (c = 0; c < NUM_MD_LOCKS; c++) {
      if (steps[step].is_lock(c) && locks_params[c]) {
        send_param = locks[idx++] - 1;
        MD.setTrackParam_inline(track_number, locks_params[c] - 1, send_param);
      }
    }
  }
  else if (trig) {
    for (c = 0; c < NUM_MD_LOCKS; c++) {
      if (locks_params[c]) {
        send_param = locks_params_orig[c];
        MD.setTrackParam_inline(track_number, locks_params[c] - 1, send_param);
      }
    }
  }
}

void MDSeqTrack::send_trig() { send_trig_inline(); }

void MDSeqTrack::send_trig_inline() {
  mixer_page.disp_levels[track_number] = MD.kit.levels[track_number];
  if (MD.kit.trigGroups[track_number] < 16) {
    mixer_page.disp_levels[MD.kit.trigGroups[track_number]] =
        MD.kit.levels[MD.kit.trigGroups[track_number]];
  }
  MD.triggerTrack(track_number, 127);
}

bool MDSeqTrack::trig_conditional(uint8_t condition) {
  bool send_trig = false;
  switch (condition) {
  case 0:
  case 1:
    if (!IS_BIT_SET64(oneshot_mask, step_count)) {
      send_trig = true;
    }
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

uint8_t MDSeqTrack::get_track_lock(uint8_t step, uint8_t lock_idx) {
  auto idx = get_lockidx(step, lock_idx);
  if (idx < NUM_MD_LOCK_SLOTS) {
    return locks[idx] - 1;
  } else {
    return locks_params_orig[lock_idx];
  }
  return 255;
}

bool MDSeqTrack::set_track_locks(uint8_t step, uint8_t track_param,
                                 uint8_t value) {
  // Let's try and find an existing param
  uint8_t match = find_param(track_param);
  // no existing param, or not locked at current step
  bool add_new = (match == 255) || !steps[step].is_lock(match);
  // Then, we learn first NUM_MD_LOCKS params then stop.
  for (uint8_t c = 0; c < NUM_MD_LOCKS && match == 255; c++) {
    if (locks_params[c] == 0) {
      locks_params[c] = track_param + 1;
      locks_params_orig[c] = MD.kit.params[track_number][track_param];
      match = c;
    }
  }

  if (match != 255) {
    if (add_new) {
      auto idx = get_lockidx(step);
      auto nlock = popcount(steps[step].locks);
      memmove(locks+idx+nlock+1, locks+idx+nlock, NUM_MD_LOCK_SLOTS - idx - nlock - 1);
      locks[idx+nlock] = value+1;
    } else {
      locks[get_lockidx(step, match)] = value + 1;
    }
    return true;
  }

  return false;
}

bool MDSeqTrack::set_track_locks_i(uint8_t step, uint8_t lockidx, uint8_t velocity) {
  return set_track_locks(step, locks_params[lockidx], velocity);
}

void MDSeqTrack::record_track_locks(uint8_t track_param, uint8_t value) {

  if (step_count >= length) {
    return;
  }

  set_track_locks(step_count, track_param, value);
}

void MDSeqTrack::set_track_pitch(uint8_t step, uint8_t pitch) {
  set_track_locks(step, 1, pitch);
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
  for(uint8_t i = 0; i < NUM_MD_STEPS; ++i) {
    steps[i].slide = false;
  }
}

void MDSeqTrack::clear_param_locks(uint8_t param_id) {
  uint8_t match = find_param(param_id);
  if (match == 255)
    return;

  uint8_t mask = 1 << match;
  uint8_t nmask = ~mask;
  uint8_t rmask = ~(mask - 1);
  uint8_t idx = 0;
  bool remove[NUM_MD_STEPS];

  // pass1, mark
  for (uint8_t x = 0; x < NUM_MD_STEPS; x++) {
    uint8_t locks = steps[x].locks;
    uint8_t nlocks = popcount(locks);
    if (locks & mask) {
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
    uint8_t skip = NUM_MD_LOCKS;
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
  }
  steps[step].locks = 0;
}

void MDSeqTrack::clear_conditional() {
  for (uint8_t c = 0; c < NUM_MD_STEPS; c++) {
    steps[c].cond_id = 0;
    steps[c].cond_plock = 0;
    timing[c] = 0;
  }
  oneshot_mask = 0;
}

void MDSeqTrack::clear_locks(bool reset_params) {
  // Need to buffer this, as we dont want sequencer interrupt
  // to access it whilst we're cleaning up
  uint8_t locks_params_buf[NUM_MD_LOCKS];
  for (uint8_t c = 0; c < NUM_MD_LOCKS; c++) {
    locks_params_buf[c] = locks_params[c];
    locks_params[c] = 0;
  }

  memset(locks, 0, sizeof(locks));
  if (reset_params) {
    for (uint8_t c = 0; c < NUM_MD_LOCKS; c++) {
      if (locks_params_buf[c] > 0) {
        MD.setTrackParam(track_number, locks_params_buf[c] - 1,
                         locks_params_orig[c]);
      }
    }
  }
}

void MDSeqTrack::clear_track(bool locks, bool reset_params) {
  clear_conditional();
  if (locks) {
    clear_locks(reset_params);
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
    if (idx == 0) {
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

  uint8_t idx = 0;
  for (uint8_t i = 0; i < NUM_MD_STEPS / 8; ++i) {
    auto pattern = *ppattern++;
    auto slide = *pslide++;
    steps[idx].trig |= pattern & 1;
    steps[idx++].slide |= slide & 1;
    steps[idx].trig |= pattern & 2;
    steps[idx++].slide |= slide & 2;
    steps[idx].trig |= pattern & 4;
    steps[idx++].slide |= slide & 4;
    steps[idx].trig |= pattern & 8;
    steps[idx++].slide |= slide & 8;
    steps[idx].trig |= pattern & 16;
    steps[idx++].slide |= slide & 16;
    steps[idx].trig |= pattern & 32;
    steps[idx++].slide |= slide & 32;
    steps[idx].trig |= pattern & 64;
    steps[idx++].slide |= slide & 64;
    steps[idx].trig |= pattern & 128;
    steps[idx++].slide |= slide & 128;
  }

  // 32770.0 is scalar to get MD swing amount in to readible percentage
  // MD sysex docs are not clear on this one so i had to hax it.

  float swing = (float)pattern->swingAmount / 16385.0;

  uint64_t swingpattern = 0;
  uint8_t timing_mid = get_timing_mid();
  if (pattern->swingEditAll > 0) {
    swingpattern |= pattern->swingPattern;
  } else {
    swingpattern |= pattern->swingPatterns[track_number];
  }

  for (uint8_t a = 0; a < length; a++) {
    if (IS_BIT_SET64(pattern->trigPatterns[track_number], a)) {
      steps[a].cond_id = 0;
      steps[a].cond_plock = false;
      timing[a] = timing_mid;
      if (IS_BIT_SET64(swingpattern, a)) {
        timing[a] = round(swing * timing_mid) + timing_mid;
      }
    }
  }
}

void MDSeqTrack::modify_track(uint8_t dir) {

  oneshot_mask = 0;
  constexpr size_t ncopy = sizeof(steps) - sizeof(MDSeqStepDescriptor);
  uint8_t lock_buf[NUM_MD_LOCKS];
  MDSeqStepDescriptor step_buf;
  uint8_t timing_buf;
  uint16_t total_nlock = get_lockidx(length);

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
}

void MDSeqTrack::copy_step(uint8_t n, MDSeqStep *step) {
  step->active = true;
  uint8_t idx = get_lockidx(n);
  uint8_t lcks = steps[n].locks;
  uint8_t mask = 1;
  for (uint8_t a = 0; a < NUM_MD_LOCKS; a++) {
    if (lcks & mask) {
      step->locks[a] = locks[idx++];
    } else {
      step->locks[a] = 0;
    }
    mask <<= 1;
  }
  step->conditional = steps[n].cond_id;
  step->conditional_plock = steps[n].cond_plock;
  step->timing = timing[n];
  step->pattern_mask = steps[n].trig;
  step->slide_mask = steps[n].slide;
}

void MDSeqTrack::paste_step(uint8_t n, MDSeqStep *step) {
  uint8_t idx = get_lockidx(n);
  uint8_t nlock = popcount(steps[n].locks);
  int8_t delta = -nlock;
  uint8_t new_locks = 0;

  for (uint8_t a = 0; a < NUM_MD_LOCKS; a++) {
    if (step->locks[a]) {
      ++delta;
      new_locks |= 1;
    }
    new_locks <<= 1;
  }

  if (delta < 0) { // shrink
    memmove(locks + idx + nlock + delta, locks + idx + nlock,
            NUM_MD_LOCK_SLOTS - idx - nlock);
  } else if (delta > 0) { // grow
    memmove(locks + idx + nlock + delta, locks + idx + nlock,
            NUM_MD_LOCK_SLOTS - idx - nlock - delta);
  }

  steps[n].cond_id = step->conditional;
  steps[n].cond_plock = step->conditional_plock;
  timing[n] = step->timing;
  steps[n].trig = step->pattern_mask;
  steps[n].slide = step->slide_mask;
}
