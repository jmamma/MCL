#include "MCL.h"
#include "MCLSeq.h"

void MDSeqTrack::set_length(uint8_t len) {
  length = len;
  if (step_count >= length) {
    // re_sync();
    step_count = (step_count % length);
  }
}

float MDSeqTrack::get_speed_multiplier() { return get_speed_multiplier(speed); }

float MDSeqTrack::get_speed_multiplier(uint8_t speed) {
  float multi;
  switch (speed) {
  default:
  case MD_SPEED_1X:
    multi = 1;
    break;
  case MD_SPEED_2X:
    multi = 0.5;
    break;
  case MD_SPEED_3_4X:
    multi = (4.0 / 3.0);
    break;
  case MD_SPEED_3_2X:
    multi = (2.0 / 3.0);
    break;
  case MD_SPEED_1_2X:
    multi = 2.0;
    break;
  case MD_SPEED_1_4X:
    multi = 4.0;
    break;
  case MD_SPEED_1_8X:
    multi = 8.0;
    break;
  }
  return multi;
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

      bool send_trig = false;
      bool lock_obey_cond = false;

      uint8_t cond = conditional[current_step];

      if (cond > 64) {
        // Locks only sent if trig_condition matches
        cond -= 64;
        lock_obey_cond = true;
      }

      send_trig = trig_conditional(cond);

      if (send_trig || lock_obey_cond == false) {
        bool pattern_mask_step = IS_BIT_SET64(pattern_mask, current_step);
        send_parameter_locks(current_step, pattern_mask_step);
        if (IS_BIT_SET64(slide_mask, current_step)) {
          locks_slides_recalc = current_step;
        }
        if (send_trig && pattern_mask_step) {
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
  bool match = false;
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
  uint8_t step, next_step;
  uint8_t timing_mid = get_timing_mid_inline();

  step = locks_slides_recalc;
  for (uint8_t c = 0; c < NUM_MD_LOCKS; c++) {
    if (locks_params[c] > 0) {
      if (locks[c][step] > 0) {
        next_step = find_next_lock(step, c);
        if (step != next_step) {
          x0 = step * timing_mid + timing[step] - timing_mid + 1;
          if (next_step < step) {
            x1 = (length + next_step) * timing_mid + timing[next_step] -
                 timing_mid - 1;
          } else {
            x1 = next_step * timing_mid + timing[next_step] - timing_mid - 1;
          }
          DEBUG_DUMP(timing[step]);
          DEBUG_DUMP(timing[next_step]);
          DEBUG_DUMP(timing_mid);
          y0 = locks[c][step] - 1;
          y1 = locks[c][next_step] - 1;

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
            locks_slide_data[c].err =
                locks_slide_data[c].dy - locks_slide_data[c].dx;
            locks_slide_data[c].dx *= 2;
          } else {
            if (locks_slide_data[c].dx < 0) {
              locks_slide_data[c].inc = -1;
              locks_slide_data[c].dx *= -1;
            }

            locks_slide_data[c].dx *= 2;
            locks_slide_data[c].err =
                locks_slide_data[c].dx - locks_slide_data[c].dy;
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
        } else {
          locks_slide_data[c].init();
        }
      }
    }
  }

  locks_slides_recalc = 255;
}

uint8_t MDSeqTrack::find_next_lock(uint8_t step, uint8_t param) {
  DEBUG_PRINT_FN();
  DEBUG_DUMP(step);
  uint8_t next_step = step + 1;
  uint8_t max_len = length;

again:
  for (; next_step < max_len; next_step++) {
    if (locks[param][next_step] > 0) {
      if (IS_BIT_SET64(lock_mask, next_step)) {
        DEBUG_DUMP(next_step);
        return next_step;
      }
      if (next_step % 8 == 0) {
        if (((uint8_t *)&(lock_mask))[((uint8_t)(next_step)) / 8] == 0) {
          next_step += 8;
        }
      }
    }
  }
  if ((next_step != step) || (next_step > length)) {
    next_step = 0;
    max_len = step;
    goto again;
  }
  DEBUG_DUMP(step);
  DEBUG_PRINTLN("exit");
  return step;
}

void MDSeqTrack::send_parameter_locks(uint8_t step, bool pattern_mask_step) {
  uint8_t c;
  bool lock_mask_step = IS_BIT_SET64(lock_mask, step);
  uint8_t send_param = 255;

  if (lock_mask_step && pattern_mask_step) {
    for (c = 0; c < NUM_MD_LOCKS; c++) {
      if (locks_params[c] > 0) {
        if (locks[c][step] > 0) {
          send_param = locks[c][step] - 1;
        } else {
          send_param = locks_params_orig[c];
        }
        MD.setTrackParam_inline(track_number, locks_params[c] - 1, send_param);
      }
    }
  }

  else if (lock_mask_step) {
    for (c = 0; c < NUM_MD_LOCKS; c++) {
      if (locks[c][step] > 0 && locks_params[c] > 0) {
        send_param = locks[c][step] - 1;
        MD.setTrackParam_inline(track_number, locks_params[c] - 1, send_param);
      }
    }
  }

  else if (pattern_mask_step) {

    for (c = 0; c < NUM_MD_LOCKS; c++) {
      if (locks_params[c] > 0) {
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

uint8_t MDSeqTrack::get_track_lock(uint8_t step, uint8_t track_param) {
  uint8_t match = 255;
  uint8_t c = 0;
  // Let's try and find an existing param
  for (c = 0; c < NUM_MD_LOCKS && match == 255; c++) {
    if (locks_params[c] == (track_param + 1)) {
      match = c;
    }
  }
  if (match != 255) {
    return locks[match][step];
  }
  return 255;
}

bool MDSeqTrack::set_track_locks(uint8_t step, uint8_t track_param,
                                 uint8_t value) {
  uint8_t match = 255;
  uint8_t c = 0;
  // Let's try and find an existing param
  for (c = 0; c < NUM_MD_LOCKS && match == 255; c++) {
    if (locks_params[c] == (track_param + 1)) {
      match = c;
    }
  }
  //  locks_params[0] = track_param + 1;
  // locks_params[0] = track_param + 1;
  // match = 0;
  // We learn first NUM_MD_LOCKS params then stop.
  for (c = 0; c < NUM_MD_LOCKS && match == 255; c++) {
    if (locks_params[c] == 0) {
      locks_params[c] = track_param + 1;
      locks_params_orig[c] = MD.kit.params[track_number][track_param];
      match = c;
    }
  }
  if (match != 255) {
    locks[match][step] = value + 1;
    if (MidiClock.state == 2) {
      SET_BIT64(lock_mask, step);
    }
    return true;
  }

  return false;
}
void MDSeqTrack::record_track_locks(uint8_t track_param, uint8_t value) {

  if (step_count >= length) {
    return;
  }

  set_track_locks(step_count, track_param, value);
}
void MDSeqTrack::set_track_pitch(uint8_t step, uint8_t pitch) {
  uint8_t match = 255;
  uint8_t c = 0;
  // Let's try and find an existing param
  for (c = 0; c < NUM_MD_LOCKS && match == 255; c++) {
    if (locks_params[c] == 1) {
      match = c;
    }
  }
  // We learn first NUM_MD_LOCKS params then stop.
  for (c = 0; c < NUM_MD_LOCKS && match == 255; c++) {
    if (locks_params[c] == 0) {
      locks_params[c] = 1;
      locks_params_orig[c] = MD.kit.params[track_number][0];
      match = c;
    }
  }
  if (match != 255) {
    locks[match][step] = pitch + 1;
    SET_BIT64(lock_mask, step);
  }
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
  SET_BIT64(pattern_mask, step);
  conditional[step] = condition;
  timing[step] = utiming;
}

bool MDSeqTrack::is_locks(uint8_t step) {
  bool match = false;
  for (uint8_t c = 0; c < NUM_MD_LOCKS && match == false; c++) {
    if (locks[c][step] > 0) {
      match = true;
    }
  }
  return match;
}

void MDSeqTrack::clear_param_locks(uint8_t param_id) {
  uint8_t match = 255;
  for (uint8_t c = 0; c < NUM_MD_LOCKS && match == 255; c++) {
    if (locks_params[c] > 0) {
      if (locks_params[c] - 1 == param_id) {
        match = c;
      }
    }
  }

  if (match != 255) {
    for (uint8_t x = 0; x < NUM_MD_STEPS; x++) {
      locks[match][x] = 0;
      if (!is_locks(x)) {
        CLEAR_BIT64(lock_mask, x);
      }
    }

    MD.setTrackParam(track_number, param_id, locks_params_orig[match]);
  }
}

void MDSeqTrack::clear_step_locks(uint8_t step) {
  for (uint8_t c = 0; c < NUM_MD_LOCKS; c++) {
    locks[c][step] = 0;
  }
  CLEAR_BIT64(lock_mask, step);
}

void MDSeqTrack::clear_conditional() {
  for (uint8_t c = 0; c < NUM_MD_STEPS; c++) {
    conditional[c] = 0;
    timing[c] = 0;
  }
  oneshot_mask = 0;
}

void MDSeqTrack::clear_locks(bool reset_params) {
  uint8_t locks_params_buf[NUM_MD_LOCKS];

  // Need to buffer this, as we dont want sequencer interrupt
  // to access it whilst we're cleaning up

  for (uint8_t c = 0; c < NUM_MD_LOCKS; c++) {
    for (uint8_t x = 0; x < NUM_MD_STEPS; x++) {
      locks[c][x] = 0;
    }

    locks_params_buf[c] = locks_params[c];
    locks_params[c] = 0;
  }
  lock_mask = 0;
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
  uint8_t c;

  clear_conditional();
  if (locks) {
    clear_locks(reset_params);
  }
  lock_mask = 0;
  pattern_mask = 0;
  slide_mask = 0;
}

void MDSeqTrack::merge_from_md(MDTrack *md_track) {
  DEBUG_PRINT_FN();

  for (int n = 0; n < md_track->arraysize; n++) {
    set_track_locks(md_track->locks[n].step, md_track->locks[n].param_number,
                    md_track->locks[n].value);
    SET_BIT64(lock_mask, md_track->locks[n].step);
  }
  pattern_mask |= md_track->trigPattern;
  // 32770.0 is scalar to get MD swing amount in to readible percentage
  // MD sysex docs are not clear on this one so i had to hax it.

  float swing = (float)md_track->kitextra.swingAmount / 16385.0;

  if (md_track->kitextra.slideEditAll > 0) {
    slide_mask |= md_track->kitextra.slidePattern;
  } else {
    slide_mask |= md_track->slidePattern;
  }

  uint64_t swingpattern = 0;
  uint8_t timing_mid = get_timing_mid();
  if (md_track->kitextra.swingEditAll > 0) {
    swingpattern |= md_track->kitextra.swingPattern;
  } else {
    swingpattern |= md_track->swingPattern;
  }
  for (uint8_t a = 0; a < length; a++) {
    if (IS_BIT_SET64(md_track->trigPattern, a)) {
      conditional[a] = 0;
      timing[a] = timing_mid;
      if (IS_BIT_SET64(swingpattern, a)) {
        timing[a] = round(swing * timing_mid) + timing_mid;
      }
    }
  }
}

void MDSeqTrack::modify_track(uint8_t dir) {

  int8_t new_pos = 0;

  MDSeqTrackData temp_data;

  memcpy(&temp_data, this, sizeof(MDSeqTrackData));
  oneshot_mask = 0;
  pattern_mask = 0;
  lock_mask = 0;

  for (uint8_t n = 0; n < length; n++) {
    switch (dir) {
    case DIR_LEFT:
      if (n == 0) {
        new_pos = length - 1;
      } else {
        new_pos = n - 1;
      }
      break;
    case DIR_RIGHT:
      if (n == length - 1) {
        new_pos = 0;
      } else {
        new_pos = n + 1;
      }
      break;
    case DIR_REVERSE:
      new_pos = length - n - 1;
      break;
    }

    for (uint8_t a = 0; a < NUM_MD_LOCKS; a++) {
      locks[a][new_pos] = temp_data.locks[a][n];
    }
    conditional[new_pos] = temp_data.conditional[n];
    timing[new_pos] = temp_data.timing[n];
    if (IS_BIT_SET64(temp_data.pattern_mask, n)) {
      SET_BIT64(pattern_mask, new_pos);
    }
    if (IS_BIT_SET64(temp_data.lock_mask, n)) {
      SET_BIT64(lock_mask, new_pos);
    }
  }
}

void MDSeqTrack::copy_step(uint8_t n, MDSeqStep *step) {
  step->active = true;
  for (uint8_t a = 0; a < NUM_MD_LOCKS; a++) {
    step->locks[a] = locks[a][n];
  }
  step->conditional = conditional[n];
  step->timing = timing[n];
  step->lock_mask = IS_BIT_SET64(lock_mask, n);
  step->pattern_mask = IS_BIT_SET64(pattern_mask, n);
  step->slide_mask = IS_BIT_SET64(slide_mask, n);
}

void MDSeqTrack::paste_step(uint8_t n, MDSeqStep *step) {
  for (uint8_t a = 0; a < NUM_MD_LOCKS; a++) {
    locks[a][n] = step->locks[a];
  }
  conditional[n] = step->conditional;
  timing[n] = step->timing;
  if (step->lock_mask) {
    SET_BIT64(lock_mask, n);
  }
  if (step->pattern_mask) {
    SET_BIT64(pattern_mask, n);
  }
  if (step->slide_mask) {
    SET_BIT64(slide_mask, n);
  }
}
