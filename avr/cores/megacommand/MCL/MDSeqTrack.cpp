#include "MCL.h"
#include "MCLSeq.h"

void MDSeqTrack::set_length(uint8_t len) {
  length = len;
  if (step_count >= length) {
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

  uint8_t timing_mid = get_timing_mid();

  if ((MidiUart.uart_block == 0) && (mute_until_start == false) &&
      (mute_state == SEQ_MUTE_OFF)) {

    uint8_t next_step = 0;
    if (step_count == (length - 1)) {
      next_step = 0;
    } else {
      next_step = step_count + 1;
    }
    uint8_t current_step;

    if (((timing[step_count] >= timing_mid) &&
         (timing[current_step = step_count] - timing_mid == mod12_counter)) ||
        ((timing[next_step] < timing_mid) &&
         ((timing[current_step = next_step]) == mod12_counter))) {
      bool send_trig = false;
      send_trig = trig_conditional(conditional[current_step]);

      if (send_trig) {

        send_parameter_locks(current_step);
        if (IS_BIT_SET64(pattern_mask, current_step)) {
          send_trig_inline();
        }
      }
    }
  }

  mod12_counter++;

  if (mod12_counter == timing_mid) {
    mod12_counter = 0;
    step_count_inc();
  }
}

bool MDSeqTrack::is_param(uint8_t param_id) {
  bool match = false;
  for (uint8_t c = 0; c < 4; c++) {
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
  for (uint8_t c = 0; c < 4 && match == false; c++) {
    if (locks_params[c] > 0) {
      if (locks_params[c] - 1 == param_id) {
        locks_params_orig[c] = value;
        match = true;
      }
    }
  }
}

void MDSeqTrack::update_kit_params() {
  for (uint8_t c = 0; c < 4; c++) {
    if (locks_params[c] > 0) {
      uint8_t param_id = locks_params[c] - 1;
      MD.kit.params[track_number][param_id] = locks_params_orig[c];
    }
  }
}

void MDSeqTrack::update_params() {

  for (uint8_t c = 0; c < 4; c++) {
    if (locks_params[c] > 0) {
      uint8_t param_id = locks_params[c] - 1;
      locks_params_orig[c] = MD.kit.params[track_number][param_id];
    }
  }
}

void MDSeqTrack::reset_params() {
  for (uint8_t c = 0; c < 4; c++) {
    if (locks_params[c] > 0) {
      MD.setTrackParam(track_number, locks_params[c] - 1, locks_params_orig[c]);
      //    MD.setTrackParam(track_number, locks_params[c] - 1,
      //                   MD.kit.params[track_number][locks_params[c] - 1]);
    }
  }
}

void MDSeqTrack::send_parameter_locks(uint8_t step) {
  uint8_t c;
  bool lock_mask_step = IS_BIT_SET64(lock_mask, step);
  bool pattern_mask_step = IS_BIT_SET64(pattern_mask, step);
  uint8_t send_param = 255;

  if (lock_mask_step && pattern_mask_step) {
    for (c = 0; c < 4; c++) {
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
    for (c = 0; c < 4; c++) {
      if (locks[c][step] > 0 && locks_params[c] > 0) {
        send_param = locks[c][step] - 1;
        MD.setTrackParam_inline(track_number, locks_params[c] - 1, send_param);
      }
    }
  }

  else if (pattern_mask_step) {

    for (c = 0; c < 4; c++) {
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
  for (c = 0; c < 4 && match == 255; c++) {
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
  for (c = 0; c < 4 && match == 255; c++) {
    if (locks_params[c] == (track_param + 1)) {
      match = c;
    }
  }
  //  locks_params[0] = track_param + 1;
  // locks_params[0] = track_param + 1;
  // match = 0;
  // We learn first 4 params then stop.
  for (c = 0; c < 4 && match == 255; c++) {
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
  for (c = 0; c < 4 && match == 255; c++) {
    if (locks_params[c] == 1) {
      match = c;
    }
  }
  // We learn first 4 params then stop.
  for (c = 0; c < 4 && match == 255; c++) {
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
  for (uint8_t c = 0; c < 4 && match == false; c++) {
    if (locks[c][step] > 0) {
      match = true;
    }
  }
  return match;
}

void MDSeqTrack::clear_param_locks(uint8_t param_id) {
  uint8_t match = 255;
  for (uint8_t c = 0; c < 4 && match == 255; c++) {
    if (locks_params[c] > 0) {
      if (locks_params[c] - 1 == param_id) {
        match = c;
      }
    }
  }

  if (match != 255) {
    for (uint8_t x = 0; x < 64; x++) {
      locks[match][x] = 0;
      if (!is_locks(x)) {
        CLEAR_BIT64(lock_mask, x);
      }
    }

    MD.setTrackParam(track_number, param_id, locks_params_orig[match]);
  }
}

void MDSeqTrack::clear_step_locks(uint8_t step) {
  for (uint8_t c = 0; c < 4; c++) {
    locks[c][step] = 0;
  }
  CLEAR_BIT64(lock_mask, step);
}

void MDSeqTrack::clear_conditional() {
  for (uint8_t c = 0; c < 64; c++) {
    conditional[c] = 0;
    timing[c] = 0;
  }
  oneshot_mask = 0;
}

void MDSeqTrack::clear_locks(bool reset_params) {
  uint8_t locks_params_buf[4];

  // Need to buffer this, as we dont want sequencer interrupt
  // to access it whilst we're cleaning up

  for (uint8_t c = 0; c < 4; c++) {
    for (uint8_t x = 0; x < 64; x++) {
      locks[c][x] = 0;
    }

    locks_params_buf[c] = locks_params[c];
    locks_params[c] = 0;
  }
  lock_mask = 0;
  if (reset_params) {
    for (uint8_t c = 0; c < 4; c++) {
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

  uint64_t swingpattern;
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

#define DIR_LEFT 0
#define DIR_RIGHT 1

void MDSeqTrack::rotate_left() {

  int8_t new_pos = 0;

  MDSeqTrackData temp_data;

  memcpy(&temp_data, this, sizeof(MDSeqTrackData));
  oneshot_mask = 0;
  pattern_mask = 0;
  lock_mask = 0;

  for (uint8_t n = 0; n < length; n++) {
    if (n == 0) {
      new_pos = length - 1;
    } else {
      new_pos = n - 1;
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

void MDSeqTrack::rotate_right() {

  int8_t new_pos = 0;

  MDSeqTrackData temp_data;

  memcpy(&temp_data, this, sizeof(MDSeqTrackData));
  oneshot_mask = 0;
  pattern_mask = 0;
  lock_mask = 0;

  for (uint8_t n = 0; n < length; n++) {
    if (n == length - 1) {
      new_pos = 0;
    } else {
      new_pos = n + 1;
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

void MDSeqTrack::reverse() {

  int8_t new_pos = 0;

  MDSeqTrackData temp_data;

  memcpy(&temp_data, this, sizeof(MDSeqTrackData));
  oneshot_mask = 0;
  pattern_mask = 0;
  lock_mask = 0;

  for (uint8_t n = 0; n < length; n++) {
    new_pos = length - n - 1;

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
}
