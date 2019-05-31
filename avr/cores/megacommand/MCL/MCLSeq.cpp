#include "MCL.h"
#include "MCLSeq.h"
void MCLSeq::setup() {

  for (uint8_t i = 0; i < NUM_PARAM_PAGES; i++) {

    seq_param_page[i].setEncoders(&seq_param1, &seq_lock1, &seq_param3,
                                  &seq_lock2);
    seq_param_page[i].construct(i * 2, 1 + i * 2);
    seq_param_page[i].page_id = i;
  }
  /*  for (uint8_t i = 0; i < NUM_LFO_PAGES; i++) {
      seq_lfo_page[i].id = i;
      seq_lfo_page[i].setEncoders(&seq_param1, &seq_param2, &seq_param3,
                                    &seq_param4);
      for (uint8_t n = 0; n < 48; n++) {
      mcl_seq.lfos[0].samples[n] = n;
              //(uint8_t) (((float) n / (float)48) * (float)96);
      }    } */
  for (uint8_t i = 0; i < num_md_tracks; i++) {
    md_tracks[i].track_number = i;
    md_tracks[i].set_length(16);
  }
  for (uint8_t i = 0; i < num_ext_tracks; i++) {
    ext_tracks[i].channel = i;
    ext_tracks[i].set_length(16);
  }

  //   MidiClock.addOnClockCallback(this,
  //   (midi_clock_callback_ptr_t)&MDSequencer::MDSetup);
  enable();

  MidiClock.addOnMidiStopCallback(
      this, (midi_clock_callback_ptr_t)&MCLSeq::onMidiStopCallback);
  MidiClock.addOnMidiStartCallback(
      this, (midi_clock_callback_ptr_t)&MCLSeq::onMidiStartCallback);
  MidiClock.addOnMidiStartImmediateCallback(
      this, (midi_clock_callback_ptr_t)&MCLSeq::onMidiStartImmediateCallback);

  MidiClock.addOnMidiContinueCallback(
      this, (midi_clock_callback_ptr_t)&MCLSeq::onMidiContinueCallback);
  midi_events.setup_callbacks();
};

void MCLSeq::enable() {
  if (state) {
    return;
  }
  MidiClock.addOn192Callback(this, (midi_clock_callback_ptr_t)&MCLSeq::seq);
  state = true;
}
void MCLSeq::disable() {
  if (!state) {
    return;
  }
  MidiClock.removeOn192Callback(this, (midi_clock_callback_ptr_t)&MCLSeq::seq);
  state = false;
}

void MCLSeq::onMidiContinueCallback() {
  for (uint8_t i = 0; i < num_md_tracks; i++) {
    md_tracks[i].update_params();
  }
}

void MCLSeq::onMidiStartImmediateCallback() {
  for (uint8_t i = 0; i < num_ext_tracks; i++) {
    // ext_tracks[i].start_clock32th = 0;
    ext_tracks[i].step_count = 0;
    ext_tracks[i].iterations = 1;
  }

  for (uint8_t i = 0; i < num_md_tracks; i++) {

    // md_tracks[i].start_clock32th = 0;
    md_tracks[i].step_count = 0;
    md_tracks[i].iterations = 1;
  }
}

void MCLSeq::onMidiStartCallback() {
  for (uint8_t i = 0; i < num_md_tracks; i++) {
    md_tracks[i].update_params();
    md_tracks[i].mute_state = SEQ_MUTE_OFF;
  }

  for (uint8_t n = 0; n < 20; n++) {
    if (grid_page.active_slots[n] >= 0) {
      mcl_actions.next_transitions[n] = 0;
      mcl_actions.next_transitions_old[n] = 0;
      mcl_actions.calc_next_slot_transition(n);
    }
  }
  mcl_actions.calc_next_transition();
}

void MCLSeq::onMidiStopCallback() {
  for (uint8_t i = 0; i < num_ext_tracks; i++) {
    ext_tracks[i].buffer_notesoff();
  }

  for (uint8_t i = 0; i < num_md_tracks; i++) {
    md_tracks[i].reset_params();
  }
}

void MCLSeq::seq() {

  for (uint8_t i = 0; i < num_md_tracks; i++) {
    if (md_tracks[i].mute_until_start) {
      if (clock_diff(MidiClock.div16th_counter, md_tracks[i].start_step) == 0) {
        md_tracks[i].step_count = 0;
        md_tracks[i].iterations = 1;
        md_tracks[i].mute_until_start = false;
      }
    }
    if ((MidiUart.uart_block == 0) &&
        (md_tracks[i].mute_until_start == false) &&
        (md_tracks[i].mute_state == SEQ_MUTE_OFF)) {

      uint8_t next_step = 0;
      if (md_tracks[i].step_count == (md_tracks[i].length - 1)) {
        next_step = 0;
      } else {
        next_step = md_tracks[i].step_count + 1;
      }

      if ((md_tracks[i].timing[md_tracks[i].step_count] >= 12) &&
          ((int8_t)md_tracks[i].timing[md_tracks[i].step_count] - 12 ==
           (int8_t)MidiClock.mod12_counter)) {

        // Dont transmit locks if MDExploit is on.
        if ((i != 15) || (!md_exploit.state)) {
          md_tracks[i].send_parameter_locks(md_tracks[i].step_count);
        }

        if (IS_BIT_SET64(md_tracks[i].pattern_mask, md_tracks[i].step_count)) {
          md_tracks[i].trig_conditional(
              md_tracks[i].conditional[md_tracks[i].step_count]);
        }
      }
      if ((md_tracks[i].timing[next_step] < 12) &&
          ((md_tracks[i].timing[next_step]) == MidiClock.mod12_counter)) {

        if ((i != 15) || (!md_exploit.state)) {
          md_tracks[i].send_parameter_locks(next_step);
        }

        if (IS_BIT_SET64(md_tracks[i].pattern_mask, next_step)) {
          md_tracks[i].trig_conditional(md_tracks[i].conditional[next_step]);
        }
      }
    }
    if (MidiClock.mod12_counter == 11) {
      if (md_tracks[i].step_count == md_tracks[i].length - 1) {
        md_tracks[i].step_count = 0;
        md_tracks[i].iterations++;
        if (md_tracks[i].iterations > 8) {
          md_tracks[i].iterations = 1;
        }
      } else {
        md_tracks[i].step_count++;
      }
    }
  }

  for (uint8_t i = 0; i < num_ext_tracks; i++) {

    if (ext_tracks[i].mute_until_start) {

      if (clock_diff(MidiClock.div16th_counter, ext_tracks[i].start_step) ==
          0) {
        ext_tracks[i].step_count = 0;
        ext_tracks[i].mute_until_start = false;
      }
    }
    if ((MidiUart2.uart_block == 0) &&
        (ext_tracks[i].mute_until_start == false) &&
        (ext_tracks[i].mute_state == SEQ_MUTE_OFF)) {

      int8_t timing_counter = MidiClock.mod12_counter;

      if ((ext_tracks[i].resolution == 1)) {
        if (MidiClock.mod12_counter < 6) {
          timing_counter = MidiClock.mod12_counter;
        } else {
          timing_counter = MidiClock.mod12_counter - 6;
        }
      }

      uint8_t next_step = 0;
      if (ext_tracks[i].step_count == ext_tracks[i].length) {
        next_step = 0;
      } else {
        next_step = ext_tracks[i].step_count + 1;
      }

      int8_t timing_mid = 6 * ext_tracks[i].resolution;
      for (uint8_t c = 0; c < 4; c++) {
        if ((ext_tracks[i].timing[ext_tracks[i].step_count] >= timing_mid) &&
            (((int8_t)ext_tracks[i].timing[ext_tracks[i].step_count] - timing_mid) ==
             (int8_t)timing_counter)) {

          if (ext_tracks[i].notes[c][ext_tracks[i].step_count] < 0) {
            ext_tracks[i].note_off(
                abs(ext_tracks[i].notes[c][ext_tracks[i].step_count]) - 1);
          }

          else if (ext_tracks[i].notes[c][ext_tracks[i].step_count] > 0) {
            ext_tracks[i].noteon_conditional(
                ext_tracks[i].conditional[ext_tracks[i].step_count],
                abs(ext_tracks[i].notes[c][ext_tracks[i].step_count]) - 1);
          }
        }

        if ((ext_tracks[i].timing[next_step] < timing_mid) &&
            ((ext_tracks[i].timing[next_step]) == (int8_t)timing_counter)) {

          if (ext_tracks[i].notes[c][ext_tracks[i].step_count + 1] < 0) {
            ext_tracks[i].note_off(abs(ext_tracks[i].notes[c][next_step]) - 1);
          } else if (ext_tracks[i].notes[c][ext_tracks[i].step_count + 1] > 0) {
            ext_tracks[i].noteon_conditional(
                ext_tracks[i].conditional[next_step],
                abs(ext_tracks[i].notes[c][next_step]) - 1);
          }
        }
      }
    }
    if (((MidiClock.mod12_counter == 11) || (MidiClock.mod12_counter == 5)) &&
        (ext_tracks[i].resolution == 1)) {
      ext_tracks[i].step_count++;
    }
    if ((MidiClock.mod12_counter == 11) && (ext_tracks[i].resolution == 2)) {
      ext_tracks[i].step_count++;
    }
    if (ext_tracks[i].step_count == ext_tracks[i].length) {
      ext_tracks[i].step_count = 0;
      ext_tracks[i].iterations++;
      if (ext_tracks[i].iterations > 8) {
        ext_tracks[i].iterations = 1;
      }
    }
  }

  /*
    for (uint8_t i = 0; i < num_md_tracks; i++) {
      md_tracks[i].seq();
    }

    for (uint8_t i = 0; i < num_ext_tracks; i++) {
      ext_tracks[i].seq();
    }
  */
}

void MCLSeqMidiEvents::onNoteOnCallback_Midi(uint8_t *msg) {}

void MCLSeqMidiEvents::onNoteOffCallback_Midi(uint8_t *msg) {}

void MCLSeqMidiEvents::onControlChangeCallback_Midi(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t param = msg[1];
  uint8_t value = msg[2];
  uint8_t track;
  uint8_t track_param;

  if (param >= 16) {
    MD.parseCC(channel, param, &track, &track_param);
    mcl_seq.md_tracks[track].update_param(track_param, value);
  }
}

void MCLSeqMidiEvents::onControlChangeCallback_Midi2(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t param = msg[1];
  uint8_t value = msg[2];
  if (channel < mcl_seq.num_ext_tracks) {
    if (param == 0x5E) {
      mcl_seq.ext_tracks[channel].mute_state = value;
    }
  }
}

void MCLSeqMidiEvents::setup_callbacks() {
  if (state) {
    return;
  }
  /*
  Midi.addOnNoteOnCallback(
      this, (midi_callback_ptr_t)&MCLSeqMidiEvents::onNoteOnCallback_Midi);
  Midi.addOnNoteOffCallback(
      this, (midi_callback_ptr_t)&MCLSeqMidiEvents::onNoteOffCallback_Midi);
  */
  Midi.addOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&MCLSeqMidiEvents::onControlChangeCallback_Midi);

  Midi2.addOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&MCLSeqMidiEvents::onControlChangeCallback_Midi2);

  state = true;
}

void MCLSeqMidiEvents::remove_callbacks() {

  if (!state) {
    return;
  }
  /*
  Midi.removeOnNoteOnCallback(
      this, (midi_callback_ptr_t)&MCLSeqMidiEvents::onNoteOnCallback_Midi);
  Midi.removeOnNoteOffCallback(
      this, (midi_callback_ptr_t)&MCLSeqMidiEvents::onNoteOffCallback_Midi);
  Midi.removeOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&MCLSeqMidiEvents::onControlChangeCallback_Midi);
  */
  Midi2.removeOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&MCLSeqMidiEvents::onControlChangeCallback_Midi2);

  state = false;
}

MCLSeq mcl_seq;
