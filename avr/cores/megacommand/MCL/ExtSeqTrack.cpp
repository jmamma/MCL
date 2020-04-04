#include "ExtSeqTrack.h"
#include "MCL.h"

void ExtSeqTrack::set_length(uint8_t len) {
  length = len;
  if (step_count >= length) {
    step_count = length % step_count;
  }
  /*uint8_t step_count =
       ((MidiClock.div32th_counter / resolution) -
        (mcl_actions.start_clock32th / resolution)) -
       (length *
        ((MidiClock.div32th_counter / resolution -
          (mcl_actions.start_clock32th / resolution)) /
         (length)));
*/
}

void ExtSeqTrack::set_ext_track_step(uint8_t step, uint8_t note_num,
                                     uint8_t velocity) {
  uint8_t match = 255;
  // Look for matching note already on this step
  // If it's a note off, then disable the note
  // If it's a note on, set the note note-off.
  for (uint8_t c = 0; c < 4 && match == 255; c++) {
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
  for (uint8_t c = 0; c < 4 && match == 255; c++) {
    if (notes[c][step] == 0) {
      match = c;
      int8_t ons_and_offs = 0;
      // Check to see if we have same number of note offs as note ons.
      // If there are more note ons for given note, the next note entered
      // should be a note off.
      for (uint8_t a = 0; a < length; a++) {
        for (uint8_t b = 0; b < 4; b++) {
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
  /* uint8_t step_count =
       ((MidiClock.div32th_counter / resolution) -
        (mcl_actions.start_clock32th / resolution)) -
       (length * ((MidiClock.div32th_counter / resolution -
                                     (mcl_actions.start_clock32th /
     resolution)) / (length)));
 */
  uint8_t utiming =
      6 + MidiClock.mod12_counter - (6 * (MidiClock.mod12_counter / 6));

  if (resolution > 1) {
    utiming = (MidiClock.mod12_counter + 12);
  }

  uint8_t condition = 0;
  uint8_t match = 255;
  uint8_t c = 0;

  for (c = 0; c < 4 && match == 255; c++) {
    if (abs(notes[c][step_count]) == note_num + 1) {
      match = c;

      if (notes[c][step_count] > 0) {
        step_count = step_count + 1;
        if (step_count > length) {
          step_count = 0;
        }
        utiming = MidiClock.mod12_counter - (6 * (MidiClock.mod12_counter / 6));
        // timing = 0;
      }
    }
  }
  for (c = 0; c < 4 && match == 255; c++) {
    if (notes[c][step_count] == 0) {
      match = c;
    }
  }

  if (match != 255) {
    notes[match][step_count] = -1 * (note_num + 1);
    // SET_BIT64(ExtLockMasks[track], step_count);
  }
  conditional[step_count] = condition;
  timing[step_count] = utiming;
}

void ExtSeqTrack::record_ext_track_noteon(uint8_t note_num, uint8_t velocity) {
  /*uint8_t step_count =
      ((MidiClock.div32th_counter / resolution) -
       (mcl_actions.start_clock32th / resolution)) -
      (length * ((MidiClock.div32th_counter / resolution -
                                    (mcl_actions.start_clock32th /
     resolution)) / (length)));
*/
  uint8_t utiming =
      6 + MidiClock.mod12_counter - (6 * (MidiClock.mod12_counter / 6));

  if (resolution > 1) {
    utiming = (MidiClock.mod12_counter + 12);
  }
  uint8_t condition = 0;

  uint8_t match = 255;
  uint8_t c = 0;
  // Let's try and find an existing param

  for (c = 0; c < 4 && match == 255; c++) {
    if (notes[c][step_count] == note_num + 1) {
      match = c;
    }
  }
  for (c = 0; c < 4 && match == 255; c++) {
    if (notes[c][step_count] == 0) {
      match = c;
    }
  }
  if (match != 255) {
    // We dont want to record steps if note off mask is set
    //  if (notes[match][step_count] < 0) { return; }
    notes[match][step_count] = note_num + 1;
    // SET_BIT64(ExtLockMasks[track], step_count);

    conditional[step_count] = condition;
    timing[step_count] = utiming;
  }
}

void ExtSeqTrack::clear_ext_conditional() {
  for (uint8_t c = 0; c < 128; c++) {
    conditional[c] = 0;
    timing[c] = 0;
  }
}
void ExtSeqTrack::clear_ext_notes() {
  for (uint8_t c = 0; c < 4; c++) {
    for (uint8_t x = 0; x < 128; x++) {
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

void ExtSeqTrack::rotate_left() {

  int8_t new_pos = 0;

  ExtSeqTrackData temp_data;

  memcpy(&temp_data, this, sizeof(ExtSeqTrackData));

  for (uint8_t a = 0; a < 4; a++) {
  lock_masks[a] = 0;
  }
  //oneshot_mask = 0;

  for (uint8_t n = 0; n < length; n++) {
     if (n == 0) { new_pos = length - 1; }
     else { new_pos = n - 1; }

     for (uint8_t a = 0; a < 4; a++) {
       notes[a][new_pos] = temp_data.notes[a][n];
       locks[a][new_pos] = temp_data.locks[a][n];
       if (IS_BIT_SET64(temp_data.lock_masks[a], n)) {
        SET_BIT64(temp_data.lock_masks[a],new_pos);
       }
     }

     conditional[new_pos] = temp_data.conditional[n];
     timing[new_pos] = temp_data.timing[n];
  }
}
void ExtSeqTrack::rotate_right() {

  int8_t new_pos = 0;

  ExtSeqTrackData temp_data;

  memcpy(&temp_data, this, sizeof(ExtSeqTrackData));

  for (uint8_t a = 0; a < 4; a++) {
  lock_masks[a] = 0;
  }
  //oneshot_mask = 0;

  for (uint8_t n = 0; n < length; n++) {
     if (n == length - 1) { new_pos = 0; }
     else { new_pos = n + 1; }

     for (uint8_t a = 0; a < 4; a++) {
       notes[a][new_pos] = temp_data.notes[a][n];
       locks[a][new_pos] = temp_data.locks[0][n];
       if (IS_BIT_SET64(temp_data.lock_masks[a], n)) {
        SET_BIT64(temp_data.lock_masks[a],new_pos);
       }
     }

     conditional[new_pos] = temp_data.conditional[n];
     timing[new_pos] = temp_data.timing[n];
  }
}

void ExtSeqTrack::reverse() {

  int8_t new_pos = 0;

  ExtSeqTrackData temp_data;

  memcpy(&temp_data, this, sizeof(ExtSeqTrackData));

  for (uint8_t a = 0; a < 4; a++) {
  lock_masks[a] = 0;
  }
  //oneshot_mask = 0;

  for (uint8_t n = 0; n < length; n++) {
     new_pos = length - n - 1;

     for (uint8_t a = 0; a < 4; a++) {
       notes[a][new_pos] = temp_data.notes[a][n];
       locks[a][new_pos] = temp_data.locks[0][n];
       if (IS_BIT_SET64(temp_data.lock_masks[a], n)) {
        SET_BIT64(temp_data.lock_masks[a],new_pos);
       }
     }

     conditional[new_pos] = temp_data.conditional[n];
     timing[new_pos] = temp_data.timing[n];
  }
}
