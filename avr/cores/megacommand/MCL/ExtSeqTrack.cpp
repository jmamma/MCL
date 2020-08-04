#include "ExtSeqTrack.h"
#include "MCL.h"

float ExtSeqTrack::get_speed_multiplier() {
  return get_speed_multiplier(speed);
}

float ExtSeqTrack::get_speed_multiplier(uint8_t speed) {
  float multi;
  switch (speed) {
  default:
  case EXT_SPEED_1X:
    multi = 1;
    break;
  case EXT_SPEED_2X:
    multi = 0.5;
    break;
  case EXT_SPEED_3_4X:
    multi = (4.0 / 3.0);
    break;
  case EXT_SPEED_3_2X:
    multi = (2.0 / 3.0);
    break;
  case EXT_SPEED_1_2X:
    multi = 2.0;
    break;
  case EXT_SPEED_1_4X:
    multi = 4.0;
    break;
  case EXT_SPEED_1_8X:
    multi = 8.0;
    break;
  }
  return multi;
}

void ExtSeqTrack::set_speed(uint8_t _speed) {
  uint8_t old_speed = speed;
  float mult = get_speed_multiplier(_speed) / get_speed_multiplier(old_speed);
  for (uint8_t i = 0; i < NUM_EXT_STEPS; i++) {
    for (uint8_t a = 0; a < NUM_EXT_NOTES; a++) {
      notes_timing[a][i] = round(mult * (float)notes_timing[a][i]);
    }
  }
  speed = _speed;
  uint8_t timing_mid = get_timing_mid();
  if (mod12_counter > timing_mid) {
    mod12_counter = mod12_counter - (mod12_counter / timing_mid) * timing_mid;
    // step_count_inc();
  }
}

void ExtSeqTrack::set_length(uint8_t len) {
  length = len;
  if (step_count >= length) {
    step_count = length % step_count;
  }
  DEBUG_DUMP(step_count);
}

void ExtSeqTrack::re_sync() {
  uint16_t q = length;
  start_step = (MidiClock.div16th_counter / q) * q + q;
  start_step_offset = 0;
  mute_until_start = true;
}

void ExtSeqTrack::seq() {
  if (mute_until_start) {

    if (clock_diff(MidiClock.div16th_counter, start_step) == 0) {
      if (MidiClock.mod12_counter == start_step_offset) {
        reset();
      }
    }
  }
  uint8_t timing_mid = get_timing_mid_inline();
  if ((MidiUart2.uart_block == 0) && (mute_until_start == false) &&
      (mute_state == SEQ_MUTE_OFF)) {

    uint8_t next_step = 0;
    if (step_count == length) {
      next_step = 0;
    } else {
      next_step = step_count + 1;
    }

    for (uint8_t c = 0; c < NUM_EXT_NOTES; c++) {
      uint8_t current_step;
      if (((notes_timing[c][step_count] >= timing_mid) &&
           ((notes_timing[c][current_step = step_count] - timing_mid) ==
            mod12_counter)) ||
          ((notes_timing[c][next_step] < timing_mid) &&
           ((notes_timing[c][current_step = next_step]) == mod12_counter))) {

        if (notes[c][current_step] < 0) {
          note_off(abs(notes[c][current_step]) - 1);
        } else if (notes[c][current_step] > 0) {
          noteon_conditional(notes_conditional[c][current_step],
                             abs(notes[c][current_step]) - 1);
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
void ExtSeqTrack::note_on(uint8_t note) {
  uart->sendNoteOn(channel, note, 100);
  DEBUG_PRINTLN("note on");
  DEBUG_DUMP(note);
  // Greater than 64
  if (IS_BIT_SET(note, 6)) {
    SET_BIT64(note_buffer[1], note - 64);
  } else {
    SET_BIT64(note_buffer[0], note);
  }
}

void ExtSeqTrack::note_off(uint8_t note) {
  uart->sendNoteOff(channel, note, 0);

  // Greater than 64
  if (IS_BIT_SET(note, 6)) {
    CLEAR_BIT64(note_buffer[1], note - 64);
  } else {
    CLEAR_BIT64(note_buffer[0], note);
  }
}

void ExtSeqTrack::noteon_conditional(uint8_t condition, uint8_t note) {
  if (condition > 64) {
    condition -= 64;
  }

  switch (condition) {
  case 0:
  case 1:
    if (!IS_BIT_SET128(oneshot_mask, step_count)) {
      note_on(note);
    }
    break;
  case 2:
    if (!IS_BIT_SET(iterations_8, 0)) {
      note_on(note);
    }
    break;
  case 3:
    if ((iterations_6 == 3) || (iterations_6 == 6)) {
      note_on(note);
    }
    break;
  case 6:
    if (iterations_6 == 6) {
      note_on(note);
    }
    break;
  case 4:
    if ((iterations_8 == 4) || (iterations_8 == 8)) {
      note_on(note);
    }
    break;
  case 8:
    if ((iterations_8 == 8)) {
      note_on(note);
    }
  case 5:
    if (iterations_5 == 5) {
      note_on(note);
    }
    break;
  case 7:
    if (iterations_7 == 7) {
      note_on(note);
    }
    break;
  case 9:
    if (get_random_byte() <= 13) {
      note_on(note);
    }
    break;
  case 10:
    if (get_random_byte() <= 32) {
      note_on(note);
    }
    break;
  case 11:
    if (get_random_byte() <= 64) {
      note_on(note);
    }
    break;
  case 12:
    if (get_random_byte() <= 96) {
      note_on(note);
    }
    break;
  case 13:
    if (get_random_byte() <= 115) {
      note_on(note);
    }
    break;
  case 14:
    if (!IS_BIT_SET128(oneshot_mask, step_count)) {
      SET_BIT128(oneshot_mask, step_count);
      note_on(note);
    }
  }
}

void ExtSeqTrack::set_ext_track_step(uint8_t step, uint8_t note_num,
                                     uint8_t velocity) {
  uint8_t match = 255;
  // Look for matching note already on this step
  // If it's a note off, then disable the note
  // If it's a note on, set the note note-off.
  for (uint8_t c = 0; c < NUM_EXT_NOTES && match == 255; c++) {
    if (notes[c][step] == -(1 * (note_num + 1))) {
      notes[c][step] = 0;
      buffer_notesoff();
      match = c;
    }
    if (notes[c][step] == (1 * (note_num + 1))) {
      notes[c][step] = (-1 * (note_num + 1));
      match = c;
    }
  }
  // No matches are found, we count number of on and off to determine next
  // note type.
  for (uint8_t c = 0; c < NUM_EXT_NOTES && match == 255; c++) {
    if (notes[c][step] == 0) {
      match = c;
      int8_t ons_and_offs = 0;
      // Check to see if we have same number of note offs as note ons.
      // If there are more note ons for given note, the next note entered
      // should be a note off.
      for (uint8_t a = 0; a < length; a++) {
        for (uint8_t b = 0; b < NUM_EXT_NOTES; b++) {
          if (notes[b][a] == -(1 * (note_num + 1))) {
            ons_and_offs -= 1;
          }
          if (notes[b][a] == (1 * (note_num + 1))) {
            ons_and_offs += 1;
          }
        }
      }
      if (ons_and_offs <= 0) {
        notes[c][step] = (note_num + 1);
      } else {
        notes[c][step] = -1 * (note_num + 1);
      }
    }
  }
}
void ExtSeqTrack::record_ext_track_noteoff(uint8_t note_num, uint8_t velocity) {

  uint8_t timing_mid = get_timing_mid() - 1;
  uint8_t utiming = (mod12_counter + timing_mid);

  uint8_t condition = 0;
  uint8_t match = 255;
  uint8_t c = 0;

  uint8_t step = step_count;

  for (c = 0; c < NUM_EXT_NOTES && match == 255; c++) {
    //if current step already has this note, then we'll use the next step over
    if (abs(notes[c][step]) == note_num + 1) {
      match = c;

      if (notes[c][step] > 0) {
        step = step + 1;
        if (step > length) {
          step = 0;
        }
        utiming = (timing_mid - mod12_counter);
        // timing = 0;
      }
    }
  }
  for (c = 0; c < NUM_EXT_NOTES && match == 255; c++) {
    if (notes[c][step_count] == 0) {
      match = c;
    }
  }

  if (match != 255) {
    notes[match][step] = -1 * (note_num + 1);
    // SET_BIT64(ExtLockMasks[track], step_count);
  }
  notes_conditional[match][step] = condition;
  notes_timing[match][step_count] = utiming;

}

void ExtSeqTrack::record_ext_track_noteon(uint8_t note_num, uint8_t velocity) {

  uint8_t utiming = (mod12_counter + get_timing_mid() - 1);
  uint8_t condition = 0;

  uint8_t match = 255;
  uint8_t c = 0;
  // Let's try and find an existing param

  for (c = 0; c < NUM_EXT_NOTES && match == 255; c++) {
    if (notes[c][step_count] == note_num + 1) {
      match = c;
    }
  }
  for (c = 0; c < NUM_EXT_NOTES && match == 255; c++) {
    if (notes[c][step_count] == 0) {
      match = c;
    }
  }
  if (match != 255) {
    // We dont want to record steps if note off mask is set
    //  if (notes[match][step_count] < 0) { return; }
    notes[match][step_count] = note_num + 1;
    // SET_BIT64(ExtLockMasks[track], step_count);

    notes_conditional[match][step_count] = condition;
    notes_timing[match][step_count] = utiming;
  }
}

void ExtSeqTrack::clear_ext_conditional() {
  for (uint8_t x = 0; x < NUM_EXT_STEPS; x++) {
    for (uint8_t c = 0; c < NUM_EXT_NOTES; c++) {
       notes_conditional[c][x] = 0;
       notes_timing[c][x] = 0;
    }
  }
}
void ExtSeqTrack::clear_ext_notes() {
  for (uint8_t c = 0; c < NUM_EXT_NOTES; c++) {
    for (uint8_t x = 0; x < NUM_EXT_STEPS; x++) {
      notes[c][x] = 0;
    }
    // ExtPatternNotesParams[i][c] = 0;
  }
}

void ExtSeqTrack::clear_track() {
  clear_ext_notes();
  clear_ext_conditional();
  buffer_notesoff();
}

void ExtSeqTrack::modify_track(uint8_t dir) {

  int8_t new_pos = 0;

  ExtSeqTrackData temp_data;

  memcpy(&temp_data, this, sizeof(ExtSeqTrackData));

  for (uint8_t a = 0; a < NUM_EXT_NOTES; a++) {
    locks_masks[a][0] = 0;
    locks_masks[a][1] = 0;
  }
  oneshot_mask[0] = 0;
  oneshot_mask[1] = 0;

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

    for (uint8_t a = 0; a < NUM_EXT_NOTES; a++) {
      notes[a][new_pos] = temp_data.notes[a][n];
      notes_timing[a][new_pos] = temp_data.notes_timing[a][n];
      notes_conditional[a][new_pos] = temp_data.notes_conditional[a][n];
      if (IS_BIT_SET128(temp_data.locks_masks[a], n)) {
        SET_BIT128(temp_data.locks_masks[a], new_pos);
      }
    }

  }
}
