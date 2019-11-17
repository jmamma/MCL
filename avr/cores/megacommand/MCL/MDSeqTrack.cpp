#include "MCL.h"
#include "MCLSeq.h"

void MDSeqTrack::set_length(uint8_t len) {
  length = len;
  if (step_count >= length) {
    step_count = (step_count % length);
  }
  /*
  step_count =
      (MidiClock.div16th_counter - start_clock32th / 2) -
      (length * ((MidiClock.div16th_counter - start_clock32th / 2) /
                 length));
*/
}

void MDSeqTrack::seq() {
  if (mute_until_start) {

    if (clock_diff(MidiClock.div16th_counter, start_step) == 0) {
      DEBUG_PRINTLN("unmuting");
      DEBUG_PRINTLN(track_number);
      DEBUG_PRINTLN(MidiClock.div16th_counter);
      DEBUG_PRINTLN(start_step);
      DEBUG_PRINTLN(MidiClock.mod12_counter);
      step_count = 0;
      iterations = 1;
      mute_until_start = false;
    }
  }
  if ((MidiUart.uart_block == 0) && (mute_until_start == false) &&
      (mute_state == SEQ_MUTE_OFF)) {

    uint8_t next_step = 0;
    if (step_count == (length - 1)) {
      next_step = 0;
    } else {
      next_step = step_count + 1;
    }

    if ((timing[step_count] >= 12) &&
        (timing[step_count] - 12 == MidiClock.mod12_counter)) {

      // Dont transmit locks if MDExploit is on.
      if ((track_number != md_exploit.track_with_nolocks) ||
          (!md_exploit.state)) {
        send_parameter_locks(step_count);
      }

      if (IS_BIT_SET64(pattern_mask, step_count)) {
        trig_conditional(conditional[step_count]);
      }
    }
    if ((timing[next_step] < 12) &&
        ((timing[next_step]) == MidiClock.mod12_counter)) {

      if ((track_number != md_exploit.track_with_nolocks) ||
          (!md_exploit.state)) {
        send_parameter_locks(next_step);
      }

      if (IS_BIT_SET64(pattern_mask, next_step)) {
        trig_conditional(conditional[next_step]);
      }
    }
  }
  if (MidiClock.mod12_counter == 11) {
    if (step_count == length - 1) {
      step_count = 0;
      iterations++;
      if (iterations > 8) {
        iterations = 1;
      }
    } else {
      step_count++;
    }
    //       DEBUG_PRINT(step_count);
    //       DEBUG_PRINT(" ");
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
      if (locks[c][step] > 0) {
        send_param = locks[c][step] - 1;
      } else if (locks_params[c] > 0) {
        send_param = locks_params_orig[c];
      }
      MD.setTrackParam_inline(track_number, locks_params[c] - 1, send_param);
    }
  }

  else if (lock_mask_step) {
    for (c = 0; c < 4; c++) {
      if (locks[c][step] > 0) {
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

void MDSeqTrack::trig_conditional(uint8_t condition) {
  bool send_trig = false;
  switch (condition) {
  case 0:
    send_trig = true;
    break;
  case 1:
    send_trig = true;
    break;
  case 2:
    if (!IS_BIT_SET(iterations, 0)) {
      send_trig = true;
    }
  case 4:
    if ((iterations == 4) || (iterations == 8)) {
      send_trig = true;
    }
  case 8:
    if ((iterations == 8)) {
      send_trig = true;
    }
    break;
  case 3:
    if ((iterations == 3) || (iterations == 6)) {
      send_trig = true;
    }
    break;
  case 5:
    if (iterations == 5) {
      send_trig = true;
    }
    break;
  case 7:
    if (iterations == 7) {
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
  if (send_trig) {
    send_trig_inline();
  }
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

void MDSeqTrack::set_track_locks(uint8_t step, uint8_t track_param,
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
  if (match != 254) {
    locks[match][step] = value + 1;
  }
  if (MidiClock.state == 2) {
    SET_BIT64(lock_mask, step);
  }
}
void MDSeqTrack::record_track_locks(uint8_t track_param, uint8_t value) {
  /*
    uint8_t step_count =
        (MidiClock.div16th_counter - mcl_actions.start_clock32th / 2) -
        (length * ((MidiClock.div16th_counter - mcl_actions.start_clock32th / 2)
    / length));*/
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
  /* uint8_t step_count =
       (MidiClock.div16th_counter - mcl_actions.start_clock32th / 2) -
       (length * ((MidiClock.div16th_counter - mcl_actions.start_clock32th / 2)
     / length)); */
  if (step_count >= length) {
    return;
  }
  set_track_pitch(step_count, pitch);
}
void MDSeqTrack::record_track(uint8_t note_num, uint8_t velocity) {
  /*uint8_t step_count =
       (MidiClock.div16th_counter - mcl_actions.start_clock32th / 2) -
       (length * ((MidiClock.div16th_counter - mcl_actions.start_clock32th / 2)
     / length)); */

  if (step_count >= length) {
    return;
  }
  uint8_t utiming = MidiClock.mod12_counter + 12;
  set_track_step(step_count, utiming, note_num, velocity);
}

void MDSeqTrack::set_track_step(uint8_t step, uint8_t utiming, uint8_t note_num,
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

  if (md_track->trigPattern == 0 && ((pattern_mask | lock_mask) != 0)) {
    // If the MD sequencer data is empty, abort merge.
    // This will prevent unnecessary length change of internal seq pattern
    return;
  }
  if ((pattern_mask | lock_mask) == 0) {
    set_length(md_track->length);
  }

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
  if (md_track->kitextra.swingEditAll > 0) {
    swingpattern |= md_track->kitextra.swingPattern;
  } else {
    swingpattern |= md_track->swingPattern;
  }
  for (uint8_t a = 0; a < length; a++) {
    if (IS_BIT_SET64(md_track->trigPattern, a)) {
      conditional[a] = 0;
      timing[a] = 12;
    }
    if (IS_BIT_SET64(swingpattern, a)) {
      timing[a] = round(swing * 12.0) + 12;
    }
  }
}

#define DIR_LEFT 0
#define DIR_RIGHT 1

void MDSeqTrack::rotate_left() {

  ROTATE_LEFT(lock_mask, length);
  ROTATE_LEFT(pattern_mask, length);
  ROTATE_LEFT(oneshot_mask, length);

  int8_t new_pos = 0;

  MDSeqTrackData temp_data;

  memcpy(&temp_data, this, sizeof(MDSeqTrackData));

  for (uint8_t n = 0; n < length; n++) {
     if (n == 0) { new_pos = length - 1 - 1; }
     else { new_pos = n - 1; }

     for (uint8_t a = 0; a < NUM_MD_LOCKS; a++) {
       locks[a][new_pos] = temp_data.locks[a][n];
     }
     conditional[new_pos] = temp_data.conditional[n];
     timing[new_pos] = temp_data.timing[n];
  }

}

void MDSeqTrack::rotate_right() {

  ROTATE_RIGHT(lock_mask, length);
  ROTATE_RIGHT(pattern_mask, length);
  ROTATE_RIGHT(oneshot_mask, length);

  int8_t new_pos = 0;

  MDSeqTrackData temp_data;

  memcpy(&temp_data, this, sizeof(MDSeqTrackData));

  for (uint8_t n = 0; n < length; n++) {
     if (n == length - 1) { new_pos = 0; }
     else { new_pos = n + 1; }

     for (uint8_t a = 0; a < NUM_MD_LOCKS; a++) {
       locks[a][new_pos] = temp_data.locks[a][n];
     }

     conditional[new_pos] = temp_data.conditional[n];
     timing[new_pos] = temp_data.timing[n];
  }

}
