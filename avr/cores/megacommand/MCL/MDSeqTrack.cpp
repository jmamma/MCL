#include "MDSeqTrack.h"

void MDSeqTrack::seq() {
  uint8_t step_count =
      (MidiClock.div16th_counter - mcl_actions_callbacks.start_clock32th / 2) -
      (length * ((MidiClock.div16th_counter -
                  mcl_actions_callbacks.start_clock32th / 2) /
                 length));

  int8_t utiming = timing[step_count];         // upper
  uint8_t condition = conditional[step_count]; // lower

  if (step_count == (length - 1)) {
    uint8_t next_step = 0;
  } else {
    uint8_t next_step = step_count + 1;
  }

  int8_t utiming_next = timing[next_step];         // upper
  uint8_t condition_next = conditional[next_step]; // lower

  //-5 -4 -3 -2 -1  0  1 2 3 4 5
  //   0 1  2  3  4  5  6  7 8 9 10 11
  ///  0  1  2  3  4  5  0  1 2 3 4 5

  if ((utiming >= 12) && (utiming - 12 == (int8_t)MidiClock.mod12_counter)) {

    send_parameter_locks(track_number, step_count);

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
void MDSeqTrack::send_parameter_locks(uint8_t step_count) {
  uint8_t c;
  if (IS_BIT_SET64(lockmask, step_count)) {
    for (c = 0; c < 4; c++) {
      if (locks[c][step_count] > 0) {
        MD.setTrackParam(track_number, locks_params[c] - 1,
                         locks[c][step_count] - 1);
      }
    }
  } else if (IS_BIT_SET64(pattern_mask, step_count)) {

    for (c = 0; c < 4; c++) {

      MD.setTrackParam(track_number, locks_params[c] - 1,
                       MD.kit.params[track_number][locks_params[c] - 1]);
    }
  }
}
void MDSeqTrack::trig_conditional(uint8_t condition, uint8_t i) {
  if ((condition == 0)) {
    MD.triggerTrack(i, 127);
  } else if (condition <= 8) {
    if (((MidiClock.div16th_counter -
          mcl_actions_callbacks.start_clock32th / 2 + length) /
         length) %
            ((condition)) ==
        0) {
      MD.triggerTrack(i, 127);
    }
  } else if ((condition == 9) && (random(100) <= 10)) {
    MD.triggerTrack(i, 127);
  } else if ((condition == 10) && (random(100) <= 25)) {
    MD.triggerTrack(i, 127);
  } else if ((condition == 11) && (random(100) <= 50)) {
    MD.triggerTrack(i, 127);
  } else if ((condition == 12) && (random(100) <= 75)) {
    MD.triggerTrack(i, 127);
  } else if ((condition == 13) && (random(100) <= 90)) {
    MD.triggerTrack(i, 127);
  }
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
      match = c;
    }
  }
  if (match != 254) {
    locks[match][step_count] = value;
  }
  if (MidiClock.state == 2) {
    SET_BIT64(lockmask, step_count);
  }
}
void MDSeqTrack::record_track_locks(uint8_t track_param, uint8_t value) {

  uint8_t step_count =
      (MidiClock.div16th_counter - mcl_actions_callbacks.start_clock32th / 2) -
      (length * ((MidiClock.div16th_counter -
                  mcl_actions_callbacks.start_clock32th / 2) /
                 length));

  set_track_locks(step_count, track_param, value);
}
void MDSeqTrack::set_track_pitch(uint8_t pitch) {
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
      match = c;
    }
  }
  if (match != 255) {
    locks[match][step_count] = realPitch + 1;
    SET_BIT64(lockmask, step_count);
  }
}
void MDSeqTrack::record_track(uint8_t note_num, uint8_t velocity) {
  uint8_t step_count =
      (MidiClock.div16th_counter - mcl_actions_callbacks.start_clock32th / 2) -
      (length * ((MidiClock.div16th_counter -
                  mcl_actions_callbacks.start_clock32th / 2) /
                 length));

  uint8_t utiming = MidiClock.mod12_counter + 12;
  set_track_step(step_count, utiming, note_num, velocity);
}

void MDSeqTrack::set_track_step(uint8_t track, uint8_t step, uint8_t utiming,
                                uint8_t note_num, uint8_t velocity) {
  uint8_t condition = 0;
  grid.cur_col = track;
  last_md_track = track;

  encoders[2]->cur = mcl_seq.md_tracks[grid.cur_col].length;
  //  timing = 3;
  // condition = 3;
  if (MidiClock.state != 2) {
    return;
  }

  SET_BIT64(PatternMasks[track], step);
  conditional[track][step] = condition;
  timing[track][step] = utiming;
}

void MDSeqTrack::clear_seq_conditional() {
  for (uint8_t c = 0; c < 64; c++) {
    conditional[c] = 0;
    timing[c] = 0;
  }
}
void MDSeqTrack::clear_seq_locks() {
  for (uint8_t c = 0; c < 4; c++) {
    for (uint8_t x = 0; x < 64; x++) {
      PatternLocks[c][x] = 0;
    }
    PatternLocksParams[c] = 0;
  }
  LockMasks = 0;
}

void MDSeqTrack::clear_seq_track() {
  uint8_t c;

  clear_seq_conditional();

  clear_seq_locks();

  lockmask = 0;
  patternmask = 0;
}
