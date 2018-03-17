#include "MCLSeq.h"

void MCLSeq::setup() {
  for (uint8_t i = 0; i < NUM_PARAM_PAGES; i++ 2) {
    seq_param_page[i].setEncoders(&seq_param1, &seq_param2, &seq_param3,
                                  &seq_param4);
    seq_param_page[i].init(i, i + 1);
  }

  for (uint8_t i = 0; i < 16; i++) {
    PatternLengths[i] = 16;
  }
  for (uint8_t i = 0; i < 6; i++) {
    ExtPatternLengths[i] = 16;
    ExtPatternResolution[i] = 1;
    ExtPatternMutes[i] = SEQ_MUTE_OFF;
  }
  //   MidiClock.addOnClockCallback(this,
  //   (midi_clock_callback_ptr_t)&MDSequencer::MDSetup);
  MidiClock.addOn192Callback(this,
                             (midi_clock_callback_ptr_t)&MCLSeq::sequencer);
  MidiClock.addOnMidiStopCallback(
      this, (midi_clock_callback_ptr_t)&MCLSeq::onMidiStopCallback);
};

void MCLSeq::set_track_param(uint8_t track, uint8_t param, uint8_t value) {
  if ((track > 15) || (param > 33))
    return;

  uint8_t track = (track >> 2);
  uint8_t b = track & 3;
  uint8_t cc = 0;
  if (param == 32) { // MUTE
    cc = 12 + b;
  } else if (param == 33) { //
    cc = 8 + b;
  } else {
    cc = param;
    if (b < 2) {
      cc += 16 + b * 24;
    } else {
      cc += 24 + b * 24;
    }
  }
  if (md_exploit.state) {
    MidiUart.sendCC(track + 3, cc, value);
  } else {
    MidiUart.sendCC(track + 9, cc, value);
  }
}

void MCLSeq::set_track_locks(uint8_t track, uint8_t step, uint8_t track_param,
                             uint8_t value) {
  uint8_t match = 255;
  uint8_t c = 0;
  // Let's try and find an existing param
  for (c = 0; c < 4 && match == 255; c++) {
    if (PatternLocksParams[track][c] == (track_param + 1)) {
      match = c;
    }
  }
  //  PatternLocksParams[track][0] = track_param + 1;
  // PatternLocksParams[track][0] = track_param + 1;
  // match = 0;
  // We learn first 4 params then stop.
  for (c = 0; c < 4 && match == 255; c++) {
    if (PatternLocksParams[track][c] == 0) {
      PatternLocksParams[track][c] = track_param + 1;
      match = c;
    }
  }
  if (match != 254) {
    PatternLocks[track][match][step_count] = value;
  }
  if (MidiClock.state == 2) {
    SET_BIT64(LockMasks[track], step_count);
  }
}
void MCLSeq::record_track_locks(uint8_t track, uint8_t track_param,
                                uint8_t value) {

  uint8_t step_count =
      (MidiClock.div16th_counter - pattern_start_clock32th / 2) -
      (PatternLengths[track] *
       ((MidiClock.div16th_counter - pattern_start_clock32th / 2) /
        PatternLengths[track]));

  set_track_locks(track, step_count, track_param, value);
}
void MCLSeq::record_ext_track_noteoff(uint8_t track, uint8_t note_num,
                                      uint8_t velocity) {
  uint8_t step_count =
      ((MidiClock.div32th_counter / ExtPatternResolution[track]) -
       (pattern_start_clock32th / ExtPatternResolution[track])) -
      (ExtPatternLengths[track] *
       ((MidiClock.div32th_counter / ExtPatternResolution[track] -
         (pattern_start_clock32th / ExtPatternResolution[track])) /
        (ExtPatternLengths[track])));

  uint8_t utiming =
      6 + MidiClock.mod12_counter - (6 * (MidiClock.mod12_counter / 6));

  if (ExtPatternResolution[track] > 1) {
    utiming = (MidiClock.mod12_counter + 12);
  }

  uint8_t condition = 0;
  //  cur_col = note_num;
  //  timing = 3;
  // condition = 3;

  uint8_t match = 255;
  uint8_t c = 0;

  for (c = 0; c < 4 && match == 255; c++) {
    if (abs(ExtPatternNotes[track][c][step_count]) == note_num + 1) {
      match = c;

      if (ExtPatternNotes[track][c][step_count] > 0) {
        step_count = step_count + 1;
        if (step_count > ExtPatternLengths[track]) {
          step_count = 0;
        }
        utiming = MidiClock.mod12_counter - (6 * (MidiClock.mod12_counter / 6));
        // timing = 0;
      }
    }
  }
  for (c = 0; c < 4 && match == 255; c++) {
    if (ExtPatternNotes[track][c][step_count] == 0) {
      match = c;
    }
  }

  if (match != 255) {
    ExtPatternNotes[track][match][step_count] = -1 * (note_num + 1);
    // SET_BIT64(ExtLockMasks[track], step_count);
  }
  Extconditional[track][step_count] = condition;
  Exttiming[track][step_count] = utiming;
}

void MCLSeq::record_ext_track_noteon(uint8_t track, uint8_t note_num,
                                     uint8_t velocity) {
  uint8_t step_count =
      ((MidiClock.div32th_counter / ExtPatternResolution[track]) -
       (pattern_start_clock32th / ExtPatternResolution[track])) -
      (ExtPatternLengths[track] *
       ((MidiClock.div32th_counter / ExtPatternResolution[track] -
         (pattern_start_clock32th / ExtPatternResolution[track])) /
        (ExtPatternLengths[track])));

  uint8_t utiming =
      6 + MidiClock.mod12_counter - (6 * (MidiClock.mod12_counter / 6));

  if (ExtPatternResolution[track] > 1) {
    utiming = (MidiClock.mod12_counter + 12);
  }
  uint8_t condition = 0;

  uint8_t match = 255;
  uint8_t c = 0;
  // Let's try and find an existing param

  for (c = 0; c < 4 && match == 255; c++) {
    if (ExtPatternNotes[track][c][step_count] == note_num + 1) {
      match = c;
    }
  }
  for (c = 0; c < 4 && match == 255; c++) {
    if (ExtPatternNotes[track][c][step_count] == 0) {
      match = c;
    }
  }

  if (match != 255) {

    // We dont want to record steps if note off mask is set

    //  if (ExtPatternNotes[track][match][step_count] < 0) { return; }

    ExtPatternNotes[track][match][step_count] = note_num + 1;
    // SET_BIT64(ExtLockMasks[track], step_count);

    Extconditional[track][step_count] = condition;
    Exttiming[track][step_count] = utiming;
  }
}

void MCLSeq::set_ext_track_step(uint8_t track, uint8_t step, uint8_t note_num,
                                uint8_t velocity) {

  uint8_t match = 255;
  // Look for matching note already on this step
  // If it's a note off, then disable the note
  // If it's a note on, set the note note-off.
  for (uint8_t c = 0; c < 4 && match == 255; c++) {
    if (ExtPatternNotes[track][c][step] == -(1 * (note_num + 1))) {
      ExtPatternNotes[track][c][step] = 0;
      seq_buffer_notesoff(track);
      match = c;
    }
    if (ExtPatternNotes[track][c][step] == (1 * (note_num + 1))) {
      ExtPatternNotes[track][c][step] = (-1 * (note_num + 1));
      match = c;
    }
  }

  // No matches are found, we count number of on and off to determine next note
  // type.
  for (uint8_t c = 0; c < 4 && match == 255; c++) {
    if (ExtPatternNotes[track][c][step] == 0) {
      match = c;
      int8_t ons_and_offs = 0;
      // Check to see if we have same number of note offs as note ons.
      // If there are more note ons for given note, the next note entered should
      // be a note off.
      for (uint8_t a = 0; a < ExtPatternLengths[track]; a++) {
        for (uint8_t b = 0; b < 4; b++) {
          if (ExtPatternNotes[track][b][a] == -(1 * (note_num + 1))) {
            ons_and_offs -= 1;
          }
          if (ExtPatternNotes[track][b][a] == (1 * (note_num + 1))) {
            ons_and_offs += 1;
          }
        }
      }
      if (ons_and_offs <= 0) {
        ExtPatternNotes[track][c][step] = (note_num + 1);
      } else {
        ExtPatternNotes[track][c][step] = -1 * (note_num + 1);
      }
    }
  }
}

void MCLSeq::set_track_pitch(uint8_t track, uint8_t pitch) {
  uint8_t match = 255;
  uint8_t c = 0;
  // Let's try and find an existing param
  for (c = 0; c < 4 && match == 255; c++) {
    if (PatternLocksParams[last_md_track][c] == 1) {
      match = c;
    }
  }
  // We learn first 4 params then stop.
  for (c = 0; c < 4 && match == 255; c++) {
    if (PatternLocksParams[last_md_track][c] == 0) {
      PatternLocksParams[last_md_track][c] = 1;
      match = c;
    }
  }
  if (match != 255) {
    PatternLocks[last_md_track][match][step_count] = realPitch + 1;
    SET_BIT64(LockMasks[last_md_track], step_count);
  }
}
void MCLSeq::record_track(uint8_t track, uint8_t note_num, uint8_t velocity) {
  uint8_t step_count =
      (MidiClock.div16th_counter - pattern_start_clock32th / 2) -
      (PatternLengths[last_md_track] *
       ((MidiClock.div16th_counter - pattern_start_clock32th / 2) /
        PatternLengths[last_md_track]));

  uint8_t utiming = MidiClock.mod12_counter + 12;
  set_track_step(track, step_count, utiming, note_num, velocity);
}

void MCLSeq::set_track_step(uint8_t track, uint8_t step, uint8_t utiming,
                            uint8_t note_num, uint8_t velocity) {
  uint8_t condition = 0;
  grid.cur_col = track;
  last_md_track = track;

  encoders[3]->cur = PatternLengths[cur_col];
  //  timing = 3;
  // condition = 3;
  if (MidiClock.state != 2) {
    return;
  }

  SET_BIT64(PatternMasks[track], step);
  conditional[track][step] = condition;
  timing[track][step] = utiming;
}

void MCLSeq::onMidiStopCallback() {
  for (uint8_t i = 0; i < 6; i++) {
    seq_buffer_notesoff(i);
  }
}
void MCLSeq::send_send_parameter_locks(uint8_t i, uint8_t step_count) {
  uint8_t c;
  if (IS_BIT_SET64(LockMasks[i], step_count)) {
    for (c = 0; c < 4; c++) {
      if (PatternLocks[i][c][step_count] > 0) {
        setTrackParam(i, PatternLocksParams[i][c] - 1,
                      PatternLocks[i][c][step_count] - 1);
      }
    }
  } else if (IS_BIT_SET64(PatternMasks[i], step_count)) {

    for (c = 0; c < 4; c++) {

      setTrackParam(i, PatternLocksParams[i][c] - 1,
                    MD.kit.params[i][PatternLocksParams[i][c] - 1]);
    }
  }
}
void MCLSeq::seq_buffer_notesoff(uint8_t track) {
  for (uint8_t c = 0; c < SEQ_NOTEBUF_SIZE; c++) {
    if (ExtPatternNoteBuffer[track][c] > 0) {
      MidiUart2.sendNoteOff(track, ExtPatternNoteBuffer[track][c], 0);
      ExtPatternNoteBuffer[track][c] = 0;
    }
  }
}
void MCLSeq::seq_note_on(uint8_t track, uint8_t note) {
  MidiUart2.sendNoteOn(track, note, 100);

  for (uint8_t c = 0; c < SEQ_NOTEBUF_SIZE; c++) {
    if (ExtPatternNoteBuffer[track][c] == 0) {
      ExtPatternNoteBuffer[track][c] = note;
      return;
    }
  }
}
void MCLSeq::seq_note_off(uint8_t track, uint8_t note) {
  MidiUart2.sendNoteOff(track, note, 0);

  for (uint8_t i = 0; i < SEQ_NOTEBUF_SIZE; i++) {
    if (ExtPatternNoteBuffer[track][i] == note) {
      ExtPatternNoteBuffer[track][i] = 0;
      return;
    }
  }
}

void MCLSeq::noteon_conditional(uint8_t condition, uint8_t track,
                                uint8_t note) {

  if ((condition == 0)) {
    seq_note_on(track, note);
  }

  else if (condition <= 8) {
    //  if ((((MidiClock.div32th_counter / ExtPatternResolution[track]) -
    //  pattern_start_clock32th * ExtPatternResolution[track] / 2) +
    //  PatternLengths[i]) / PatternLengths[i]) % ( (condition)) == 0) {
    // if ((((MidiClock.div32th_counter / ExtPatternResolution[track]) -
    // (pattern_start_clock32th / ExtPatternResolution[track]) +
    // PatternLengths[track] * (2 / ExtPatternResolution[track])) /
    // PatternLengths[track] * (2 / ExtPatternResolution[track])) % (
    // (condition)) == 0) {

    if (ExtPatternResolution[track] == 2) {
      if (((MidiClock.div16th_counter - pattern_start_clock32th / 2 +
            PatternLengths[track]) /
           PatternLengths[track]) %
              ((condition)) ==
          0) {
        seq_note_on(track, note);
      }
    } else {
      if (((MidiClock.div32th_counter - pattern_start_clock32th +
            PatternLengths[track]) /
           PatternLengths[track]) %
              ((condition)) ==
          0) {
        seq_note_on(track, note);
      }
    }
  } else if ((condition == 9) && (random(100) <= 10)) {
    seq_note_on(track, note);
  } else if ((condition == 10) && (random(100) <= 25)) {
    seq_note_on(track, note);
  } else if ((condition == 11) && (random(100) <= 50)) {
    seq_note_on(track, note);
  } else if ((condition == 12) && (random(100) <= 75)) {
    seq_note_on(track, note);
  } else if ((condition == 13) && (random(100) <= 90)) {
    seq_note_on(track, note);
  }
}

void MCLSeq::trig_conditional(uint8_t condition, uint8_t i) {
  if ((condition == 0)) {
    MD.triggerTrack(i, 127);
  } else if (condition <= 8) {
    if (((MidiClock.div16th_counter - pattern_start_clock32th / 2 +
          PatternLengths[i]) /
         PatternLengths[i]) %
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

void MCLSeq::sequencer() {
  // if (IS_BIT_SET8(UCSR1A, RXC1)) { isr_usart1(1); }
  uint8_t next_step;

  //   setTrackParam(1,0,random(127));
  if (in_sysex == 0) {

    for (uint8_t i = 0; i < 16; i++) {
      // isr_usart1(1);
      // if (MidiClock.mod6_counter == 0) { MD.triggerTrack(i, 127); }
      uint8_t step_count =
          (MidiClock.div16th_counter - pattern_start_clock32th / 2) -
          (PatternLengths[i] *
           ((MidiClock.div16th_counter - pattern_start_clock32th / 2) /
            PatternLengths[i]));

      int8_t utiming = timing[i][step_count];         // upper
      uint8_t condition = conditional[i][step_count]; // lower

      if (step_count == (PatternLengths[i] - 1)) {
        next_step = 0;
      } else {
        next_step = step_count + 1;
      }

      int8_t utiming_next = timing[i][next_step];         // upper
      uint8_t condition_next = conditional[i][next_step]; // lower

      //-5 -4 -3 -2 -1  0  1 2 3 4 5
      //   0 1  2  3  4  5  6  7 8 9 10 11
      ///  0  1  2  3  4  5  0  1 2 3 4 5

      if ((utiming >= 12) &&
          (utiming - 12 == (int8_t)MidiClock.mod12_counter)) {

        send_parameter_locks(i, step_count);

        if (IS_BIT_SET64(PatternMasks[i], step_count)) {
          trig_conditional(condition, i);
        }
      }

      if ((utiming_next < 12) &&
          ((utiming_next) == (int8_t)MidiClock.mod12_counter)) {

        send_parameter_locks(i, next_step);

        if (IS_BIT_SET64(PatternMasks[i], next_step)) {
          trig_conditional(condition_next, i);
        }
      }
    }
  }
  for (uint8_t i = 0; i < 4; i++) {
    //  isr_usart1(1);

    //   uint8_t step_count = ((MidiClock.div32th_counter *
    //   ExtPatternResolution[i]) - (pattern_start_clock32th /
    //   ExtPatternResolution[i])) - (ExtPatternLengths[i] * (2 /
    //   ExtPatternResolution[i]) * ((MidiClock.div32th_counter *
    //   ExtPatternResolution[i] - (pattern_start_clock32th /
    //   ExtPatternResolution[i])) / (ExtPatternLengths[i] * (2 /
    //   ExtPatternResolution[i]))));
    //  uint8_t step_count = (MidiClock.div16th_counter -
    //  pattern_start_clock32th / 2) - (PatternLengths[i] *
    //  ((MidiClock.div16th_counter - pattern_start_clock32th / 2) /
    //  PatternLengths[i]));

    //     uint8_t step_count = ( (MidiClock.div32th_counter /
    //     ExtPatternResolution[i]) - (pattern_start_clock32th *
    //     ExtPatternResolution[i] / 2)) - (ExtPatternLengths[i] * (2 /
    //     ExtPatternResolution[i]) * ((MidiClock.div32th_counter /
    //     ExtPatternResolution[i] - (pattern_start_clock32th *
    //     ExtPatternResolution[i] / 2)) / (ExtPatternLengths[i] * (2 /
    //     ExtPatternResolution[i]))));
    // uint8_t step_count = ( (MidiClock.div32th_counter /
    // ExtPatternResolution[i]) - (pattern_start_clock32th /
    // ExtPatternResolution[i])) - (ExtPatternLengths[i] * (2 /
    // ExtPatternResolution[i]) * ((MidiClock.div32th_counter /
    // ExtPatternResolution[i] - (pattern_start_clock32th /
    // ExtPatternResolution[i])) / (ExtPatternLengths[i] * (2 /
    // ExtPatternResolution[i]))));

    uint8_t step_count =
        ((MidiClock.div32th_counter / ExtPatternResolution[i]) -
         (pattern_start_clock32th / ExtPatternResolution[i])) -
        (ExtPatternLengths[i] *
         ((MidiClock.div32th_counter / ExtPatternResolution[i] -
           (pattern_start_clock32th / ExtPatternResolution[i])) /
          (ExtPatternLengths[i])));

    int8_t utiming = Exttiming[i][step_count];         // upper
    uint8_t condition = Extconditional[i][step_count]; // lower

    int8_t timing_counter = MidiClock.mod12_counter;

    if ((ExtPatternResolution[i] == 1)) {
      timing_counter =
          MidiClock.mod12_counter - (6 * (MidiClock.mod12_counter / 6));
    }
    //        if (step_count == (ExtPatternLengths[i] * (2 /
    //        ExtPatternResolution[i]) - 1)) {

    if (step_count == ExtPatternLengths[i]) {
      next_step = 0;
    } else {
      next_step = step_count + 1;
    }

    int8_t utiming_next = Exttiming[i][next_step];         // upper
    uint8_t condition_next = Extconditional[i][next_step]; // lower
    if (!in_sysex2) {

      if ((utiming >= (6 * ExtPatternResolution[i])) &&
          (utiming - (6 * ExtPatternResolution[i]) == (int8_t)timing_counter)) {

        for (uint8_t c = 0; c < 4; c++) {
          if (ExtPatternNotes[i][c][step_count] < 0) {
            seq_note_off(i, abs(ExtPatternNotes[i][c][step_count]) - 1);
          }

          else if (ExtPatternNotes[i][c][step_count] > 0) {
            if ((ExtPatternMutes[i] == SEQ_MUTE_OFF)) {
              noteon_conditional(condition, i,
                                 abs(ExtPatternNotes[i][c][step_count]) - 1);
            }
          }
        }
      }

      if ((utiming_next < (6 * ExtPatternResolution[i])) &&
          ((utiming_next) == (int8_t)timing_counter)) {

        for (uint8_t c = 0; c < 4; c++) {

          if (ExtPatternNotes[i][c][step_count + 1] < 0) {
            seq_note_off(i, abs(ExtPatternNotes[i][c][next_step]) - 1);
          } else if (ExtPatternNotes[i][c][step_count + 1] > 0) {
            if (ExtPatternMutes[i] == SEQ_MUTE_OFF) {
              noteon_conditional(condition, i,
                                 abs(ExtPatternNotes[i][c][next_step]) - 1);
            }
          }
        }
      }
    }
  }
}

void MCLSeq::onMidiStopCallback() {
  for (uint8_t i = 0; i < 6; i++) {
    seq_buffer_notesoff(i);
  }
}

void MCLSeqMidiEvents::onNoteOnCallback(uint8_t *msg) {}
void MCLSeqMidiEvents::onNoteOffCallback(uint8_t *msg) {}

void MCLSeqMidiEvents::onNoteOffCallback(uint8_t *msg) {}

void MCLSeqMidiEvents::onControlChangeCallback(uint8_t *msg) {}

void MCLSeqMidiEvents::onControlChangeCallbackMidi2(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t param = msg[1];
  uint8_t value = msg[2];
  if (param == 0x5E) {
    ExtPatternMutes[channel] = value;
  }
}

void MCLSeqMidiEvents::setup_callbacks() {
  if (state) {
    return;
  }
  Midi.addOnNoteOnCallback(
      this, (midi_callback_ptr_t)&MCLSeqMidiEvents::onNoteOnCallback);
  Midi.addOnNoteOffCallback(
      this, (midi_callback_ptr_t)&MCLSeqMidiEvents::onNoteOffCallback);
  Midi.addOnControlChangeCallback(
      this, (midi_callback_ptr_t)&MCLSeqMidiEvents::onControlChangeCallback);
  Midi2.addOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&MCLSeqMidiEvents::onControlChangeCallbackMidi2);

  state = true;
}

void MCLSeqMidiEvents::remove_callbacks() {

  if (!state) {
    return;
  }
  Midi.removeOnNoteOnCallback(
      this, (midi_callback_ptr_t)&MCLSeqMidiEvents::onNoteOnCallback);
  Midi.removeOnNoteOffCallback(
      this, (midi_callback_ptr_t)&MCLSeqMidiEvents::onNoteOffCallback);
  Midi.removeOnControlChangeCallback(
      this, (midi_callback_ptr_t)&MCLSeqMidiEvents::onControlChangeCallback);
  Midi2.removeOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&MCLSeqMidiEvents::onControlChangeCallbackMidi2);

  state = false;
}

MCLSeq mcl_seq;
