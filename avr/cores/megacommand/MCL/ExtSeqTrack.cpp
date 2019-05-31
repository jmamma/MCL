#include "ExtSeqTrack.h"
#include "MCL.h"

void ExtSeqTrack::set_length(uint8_t len) {
  length = len;
  if (step_count >= length) {
    step_count = length % step_count;
  }
  DEBUG_PRINTLN(step_count);
  /*uint8_t step_count =
       ((MidiClock.div32th_counter / resolution) -
        (mcl_actions.start_clock32th / resolution)) -
       (length *
        ((MidiClock.div32th_counter / resolution -
          (mcl_actions.start_clock32th / resolution)) /
         (length)));
*/
}
void ExtSeqTrack::seq() {
  if (mute_until_start) {

    if (clock_diff(MidiClock.div16th_counter, start_step) == 0) {
      step_count = 0;
      mute_until_start = false;
    }
  }
  if ((MidiUart2.uart_block == 0) && (mute_until_start == false) &&
      (mute_state == SEQ_MUTE_OFF)) {

    int8_t timing_counter = MidiClock.mod12_counter;

    if ((resolution == 1)) {
      if (MidiClock.mod12_counter < 6) {
      timing_counter =
          MidiClock.mod12_counter;
      }
      else {
      timing_counter =
          MidiClock.mod12_counter - 6;
      }
    }

    uint8_t next_step = 0;
    if (step_count == length) {
      next_step = 0;
    } else {
      next_step = step_count + 1;
    }

    int8_t timing_mid = 6 * resolution;
    for (uint8_t c = 0; c < 4; c++) {
      if ((timing[step_count] >= timing_mid) &&
          (((int8_t)timing[step_count] - timing_mid) == (int8_t)timing_counter)) {

        if (notes[c][step_count] < 0) {
          note_off(abs(notes[c][step_count]) - 1);
        }

        else if (notes[c][step_count] > 0) {
          noteon_conditional(conditional[step_count], abs(notes[c][step_count]) - 1);
        }
      }

      if ((timing[next_step] < timing_mid) &&
          ((timing[next_step]) == (int8_t)timing_counter)) {

        if (notes[c][step_count + 1] < 0) {
          note_off(abs(notes[c][next_step]) - 1);
        } else if (notes[c][step_count + 1] > 0) {
          noteon_conditional(conditional[next_step], abs(notes[c][next_step]) - 1);
        }
      }
    }
  }
  if (((MidiClock.mod12_counter == 11) || (MidiClock.mod12_counter == 5)) &&
      (resolution == 1)) {
    step_count++;
  }
  if ((MidiClock.mod12_counter == 11) && (resolution == 2)) {
    step_count++;
  }
  if (step_count == length) {
    step_count = 0;
    iterations++;
    if (iterations > 8) {
      iterations = 1;
    }
  }
}

void ExtSeqTrack::buffer_notesoff() {
  for (uint8_t c = 0; c < SEQ_NOTEBUF_SIZE; c++) {
    if (notebuffer[c] > 0) {
      uart->sendNoteOff(channel, notebuffer[c], 0);
      notebuffer[c] = 0;
    }
  }
}
void ExtSeqTrack::note_on(uint8_t note) {
  uart->sendNoteOn(channel, note, 100);

  for (uint8_t c = 0; c < SEQ_NOTEBUF_SIZE; c++) {
    if (notebuffer[c] == 0) {
      notebuffer[c] = note;
      return;
    }
  }
}
void ExtSeqTrack::note_off(uint8_t note) {
  uart->sendNoteOff(channel, note, 0);

  for (uint8_t i = 0; i < SEQ_NOTEBUF_SIZE; i++) {
    if (notebuffer[i] == note) {
      notebuffer[i] = 0;
      return;
    }
  }
}

void ExtSeqTrack::noteon_conditional(uint8_t condition, uint8_t note) {
  switch (condition) {
  case 0:
    note_on(note);
    break;
  case 1:
    note_on(note);
    break;
  case 2:
    if (!IS_BIT_SET(iterations, 0)) {
      note_on(note);
    }
  case 4:
    if ((iterations == 4) || (iterations == 8)) {
      note_on(note);
    }
  case 8:
    if ((iterations == 8)) {
      note_on(note);
    }
    break;
  case 3:
    if ((iterations == 3) || (iterations == 6)) {
      note_on(note);
    }
    break;
  case 5:
    if (iterations == 5) {
      note_on(note);
    }
    break;
  case 7:
    if (iterations == 7) {
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
  }
}

void ExtSeqTrack::set_ext_track_step(uint8_t step, uint8_t note_num,
                                     uint8_t velocity) {
  DEBUG_PRINTLN("recording notes");
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
