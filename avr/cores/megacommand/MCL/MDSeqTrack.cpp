#include "MCL.h"
#include "MCLSeq.h"

void MDSeqTrack::seq() {
  uint8_t step_count =
      (MidiClock.div16th_counter - mcl_actions_callbacks.start_clock32th / 2) -
      (length * ((MidiClock.div16th_counter -
                  mcl_actions_callbacks.start_clock32th / 2) /
                 length));

  int8_t utiming = timing[step_count];         // upper
  uint8_t condition = conditional[step_count]; // lower
  uint8_t next_step = 0;
  if (step_count == (length - 1)) {
    next_step = 0;
  } else {
    next_step = step_count + 1;
  }

  int8_t utiming_next = timing[next_step];         // upper
  uint8_t condition_next = conditional[next_step]; // lower

  //-5 -4 -3 -2 -1  0  1 2 3 4 5
  //   0 1  2  3  4  5  6  7 8 9 10 11
  ///  0  1  2  3  4  5  0  1 2 3 4 5

  if ((utiming >= 12) && (utiming - 12 == (int8_t)MidiClock.mod12_counter)) {

    send_parameter_locks(step_count);

    if (IS_BIT_SET64(pattern_mask, step_count)) {
      trig_conditional(condition);
    }
  }

  if ((utiming_next < 12) &&
      ((utiming_next) == (int8_t)MidiClock.mod12_counter)) {

    send_parameter_locks(next_step);

    if (IS_BIT_SET64(pattern_mask, next_step)) {
      trig_conditional(condition_next);
    }
  }
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

void MDSeqTrack::update_params() {

      DEBUG_PRINT_FN();
  for (uint8_t c = 0; c < 4; c++) {
    if (locks_params[c] > 0) {
      uint8_t param_id = locks_params[c] - 1;
      DEBUG_PRINTLN(c);
      DEBUG_PRINTLN(param_id);
      DEBUG_PRINTLN(MD.kit.params[track_number][param_id]);
      locks_params_orig[c] = MD.kit.params[track_number][param_id];
    }
  }
}

void MDSeqTrack::reset_params() {
   DEBUG_PRINT_FN();
  for (uint8_t c = 0; c < 4; c++) {
    if (locks_params[c] > 0) {
      DEBUG_PRINTLN(locks_params_orig[c]);
      MD.setTrackParam(track_number, locks_params[c] - 1, locks_params_orig[c]);
      //    MD.setTrackParam(track_number, locks_params[c] - 1,
      //                   MD.kit.params[track_number][locks_params[c] - 1]);
    }
  }
}

void MDSeqTrack::send_parameter_locks(uint8_t step_count) {
  uint8_t c;
  if (IS_BIT_SET64(lock_mask, step_count)) {
    for (c = 0; c < 4; c++) {
      if (locks[c][step_count] > 0) {
        MD.setTrackParam(track_number, locks_params[c] - 1,
                         locks[c][step_count] - 1);
      } else if (locks_params[c] > 0) {
        MD.setTrackParam(track_number, locks_params[c] - 1,
                         locks_params_orig[c]);
        //        MD.setTrackParam(track_number, locks_params[c] - 1,
        //                 MD.kit.params[track_number][locks_params[c] - 1]);
      }
    }
  } else if (IS_BIT_SET64(pattern_mask, step_count)) {

    for (c = 0; c < 4; c++) {
      if (locks_params[c] > 0) {

        MD.setTrackParam(track_number, locks_params[c] - 1,
                         locks_params_orig[c]);
        //        MD.setTrackParam(track_number, locks_params[c] - 1,
        //       MD.setTrackParam(track_number, locks_params[c] - 1,
        //                      MD.kit.params[track_number][locks_params[c] -
        //                      1]);
      }
    }
  }
}
void MDSeqTrack::trig_conditional(uint8_t condition) {
  if (condition == 0) {

    mixer_page.disp_levels[track_number] = MD.kit.levels[track_number];
    MD.triggerTrack(track_number, 127);
  } else if (condition <= 8) {
    if (((MidiClock.div16th_counter -
          mcl_actions_callbacks.start_clock32th / 2 + length) /
         length) %
            ((condition)) ==
        0) {
      mixer_page.disp_levels[track_number] = MD.kit.levels[track_number];
      MD.triggerTrack(track_number, 127);
    }
  } else {

    uint8_t rnd = random(100);
    switch (condition) {
    case 9:
      if (rnd <= 10) {
        mixer_page.disp_levels[track_number] = MD.kit.levels[track_number];
        MD.triggerTrack(track_number, 127);
      }
      break;
    case 10:
      if (rnd <= 25) {
        mixer_page.disp_levels[track_number] = MD.kit.levels[track_number];
        MD.triggerTrack(track_number, 127);
      }
      break;
    case 11:
      if (rnd <= 50) {
        mixer_page.disp_levels[track_number] = MD.kit.levels[track_number];
        MD.triggerTrack(track_number, 127);
      }
      break;
    case 12:
      if (rnd <= 75) {
        mixer_page.disp_levels[track_number] = MD.kit.levels[track_number];
        MD.triggerTrack(track_number, 127);
      }
      break;
    case 13:
      if (rnd <= 90) {
        mixer_page.disp_levels[track_number] = MD.kit.levels[track_number];
        MD.triggerTrack(track_number, 127);
      }
      break;
    }
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
      locks_params_orig[c] = MD.kit.params[track_param];
      match = c;
    }
  }
  if (match != 254) {
    locks[match][step] = value;

  }
  if (MidiClock.state == 2) {
    SET_BIT64(lock_mask, step);
  }
}
void MDSeqTrack::record_track_locks(uint8_t track_param, uint8_t value) {

  uint8_t step_count =
      (MidiClock.div16th_counter - mcl_actions_callbacks.start_clock32th / 2) -
      (length * ((MidiClock.div16th_counter -
                  mcl_actions_callbacks.start_clock32th / 2) /
                 length));
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
      locks_params_orig[c] = MD.kit.params[0];
      match = c;
    }
  }
  if (match != 255) {
    locks[match][step] = pitch + 1;
    SET_BIT64(lock_mask, step);
  }
}

void MDSeqTrack::record_track_pitch(uint8_t pitch) {
  uint8_t step_count =
      (MidiClock.div16th_counter - mcl_actions_callbacks.start_clock32th / 2) -
      (length * ((MidiClock.div16th_counter -
                  mcl_actions_callbacks.start_clock32th / 2) /
                 length));
  if (step_count >= length) {
    return;
  }
  set_track_pitch(step_count, pitch);
}
void MDSeqTrack::record_track(uint8_t note_num, uint8_t velocity) {
  uint8_t step_count =
      (MidiClock.div16th_counter - mcl_actions_callbacks.start_clock32th / 2) -
      (length * ((MidiClock.div16th_counter -
                  mcl_actions_callbacks.start_clock32th / 2) /
                 length));

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

  SET_BIT64(pattern_mask, step);
  conditional[step] = condition;
  timing[step] = utiming;
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
}
void MDSeqTrack::clear_locks() {
  for (uint8_t c = 0; c < 4; c++) {
    for (uint8_t x = 0; x < 64; x++) {
      locks[c][x] = 0;
    }
    locks_params[c] = 0;
  }
  lock_mask = 0;
}

void MDSeqTrack::clear_track(bool locks) {
  uint8_t c;

  clear_conditional();
  if (locks) {
  clear_locks();
  }
  lock_mask = 0;
  pattern_mask = 0;
}
