#include "MCLSequencer.h"

void MCLSequencer::set_track_param(uint8_t track, uint8_t param, uint8_t value) {
 if ((track > 15) || (param > 33))
    return;

  uint8_t channel = (track >> 2);
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
    MidiUart.sendCC(channel + 3, cc, value);
  }
  else {
    MidiUart.sendCC(channel + 9, cc, value);
  }
}

void MCLSequencer::setup() {
  for (uint8_t i = 0; i < NUM_PARAM_PAGES; i++ 2) {
    seq_param_page[i].setEncoders(&seq_param1, &seq_param2, &seq_param3, &seq_param4);
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
  MidiClock.addOn192Callback(
      this, (midi_clock_callback_ptr_t)&MDSequencer::sequencer);
  MidiClock.addOnMidiStopCallback(
      this, (midi_clock_callback_ptr_t)&MDSequencer::onMidiStopCallback);
};
//   void MDSetup() {
//      if (MD.connected == false) { md_setup(); }
//   }
void MCLSequencer::onMidiStopCallback() {
  for (uint8_t i = 0; i < 6; i++) {
    seq_buffer_notesoff(i);
  }
}
void MCLSequencer::parameter_locks(uint8_t i, uint8_t step_count) {
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
void MCLSequencer::seq_buffer_notesoff(uint8_t track) {
  for (uint8_t c = 0; c < SEQ_NOTEBUF_SIZE; c++) {
    if (ExtPatternNoteBuffer[track][c] > 0) {
      MidiUart2.sendNoteOff(track, ExtPatternNoteBuffer[track][c], 0);
      ExtPatternNoteBuffer[track][c] = 0;
    }
  }
}
void MCLSequencer::seq_note_on(uint8_t track, uint8_t note) {
  MidiUart2.sendNoteOn(track, note, 100);

  for (uint8_t c = 0; c < SEQ_NOTEBUF_SIZE; c++) {
    if (ExtPatternNoteBuffer[track][c] == 0) {
      ExtPatternNoteBuffer[track][c] = note;
      return;
    }
  }
}
void MCLSequencer::seq_note_off(uint8_t track, uint8_t note) {
  MidiUart2.sendNoteOff(track, note, 0);

  for (uint8_t i = 0; i < SEQ_NOTEBUF_SIZE; i++) {
    if (ExtPatternNoteBuffer[track][i] == note) {
      ExtPatternNoteBuffer[track][i] = 0;
      return;
    }
  }
}

void MCLSequencer::noteon_conditional(uint8_t condition, uint8_t track,
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

void MCLSequencer::trig_conditional(uint8_t condition, uint8_t i) {
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

void MCLSequencer::sequencer() {
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

        parameter_locks(i, step_count);

        if (IS_BIT_SET64(PatternMasks[i], step_count)) {
          trig_conditional(condition, i);
        }
      }

      if ((utiming_next < 12) &&
          ((utiming_next) == (int8_t)MidiClock.mod12_counter)) {

        parameter_locks(i, next_step);

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

MCLSequencer mcl_seq;
