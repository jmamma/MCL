#include "MCL.h"
#include "MCLSeq.h"

void MCLSeq::setup() {

  for (uint8_t i = 0; i < NUM_PARAM_PAGES; i++) {

    seq_param_page[i].setEncoders(&seq_param1, &seq_param2, &seq_param3,
                                  &seq_param4);
    seq_param_page[i].construct(i * 2, 1 + i * 2);
    seq_param_page[i].page_id = i;
  }
  for (uint8_t i = 0; i < num_md_tracks; i++) {
    md_tracks[i].track_number = i;
    md_tracks[i].length = 16;
  }

  for (uint8_t i = 0; i < num_ext_tracks; i++) {
    ext_tracks[i].channel = i;
    ext_tracks[i].length = 16;
  }

  //   MidiClock.addOnClockCallback(this,
  //   (midi_clock_callback_ptr_t)&MDSequencer::MDSetup);
  MidiClock.addOn192Callback(this,
                             (midi_clock_callback_ptr_t)&MCLSeq::sequencer);
  MidiClock.addOnMidiStopCallback(
      this, (midi_clock_callback_ptr_t)&MCLSeq::onMidiStopCallback);
};

void MCLSeq::onMidiStopCallback() {
  for (uint8_t i = 0; i < 4; i++) {
    ext_tracks[i].buffer_notesoff();
  }
}

void MCLSeq::sequencer() {

  //   MD.setTrackParam(1,0,random(127));
  if (in_sysex == 0) {
/*
          for (uint8_t i = 0; i < num_md_tracks; i++) { 
   uint8_t step_count =
      (MidiClock.div16th_counter - mcl_actions_callbacks.start_clock32th / 2) -
      (mcl_seq.md_tracks[i].length * ((MidiClock.div16th_counter -
                  mcl_actions_callbacks.start_clock32th / 2) /
                 md_tracks[i].length));

  int8_t utiming = md_tracks[i].timing[step_count];         // upper
  uint8_t condition = md_tracks[i].conditional[step_count]; // lower
  uint8_t next_step = 0;
  if (step_count == (md_tracks[i].length - 1)) {
    next_step = 0;
  } else {
    next_step = step_count + 1;
  }

  int8_t utiming_next = md_tracks[i].timing[next_step];         // upper
  uint8_t condition_next = md_tracks[i].conditional[next_step]; // lower

  //-5 -4 -3 -2 -1  0  1 2 3 4 5
  //   0 1  2  3  4  5  6  7 8 9 10 11
  ///  0  1  2  3  4  5  0  1 2 3 4 5

  if ((utiming >= 12) && (utiming - 12 == (int8_t)MidiClock.mod12_counter)) {

  //  md_tracks[i].send_parameter_locks(step_count);

    if (IS_BIT_SET64(md_tracks[i].pattern_mask, step_count)) 
    {
    //  md_tracks[i].trig_conditional(condition);
    MD.triggerTrack(i, 127);
    }
  }

  if ((utiming_next < 12) &&
      ((utiming_next) == (int8_t)MidiClock.mod12_counter)) {

   // md_tracks[i].send_parameter_locks(next_step);

    if (IS_BIT_SET64(md_tracks[i].pattern_mask, next_step)) {
   //   md_tracks[i].trig_conditional(condition_next);
    MD.triggerTrack(i, 127);
    }
  }
   } 
          
*/          
    for (uint8_t i = 0; i < num_md_tracks; i++) {
      md_tracks[i].seq();
    }

    for (uint8_t i = 0; i < num_ext_tracks; i++) {
      ext_tracks[i].seq();
    }

  }
}

void MCLSeqMidiEvents::onNoteOnCallback_Midi(uint8_t *msg) {}

void MCLSeqMidiEvents::onNoteOffCallback_Midi(uint8_t *msg) {}

void MCLSeqMidiEvents::onControlChangeCallback_Midi(uint8_t *msg) {}

void MCLSeqMidiEvents::onControlChangeCallback_Midi2(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t param = msg[1];
  uint8_t value = msg[2];
  if (channel < mcl_seq.num_ext_tracks) {
    if (param == 0x5E) {
      mcl_seq.ext_tracks[channel].mute = value;
    }
  }
}

void MCLSeqMidiEvents::setup_callbacks() {
  if (state) {
    return;
  }
  Midi.addOnNoteOnCallback(
      this, (midi_callback_ptr_t)&MCLSeqMidiEvents::onNoteOnCallback_Midi);
  Midi.addOnNoteOffCallback(
      this, (midi_callback_ptr_t)&MCLSeqMidiEvents::onNoteOffCallback_Midi);
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
  Midi.removeOnNoteOnCallback(
      this, (midi_callback_ptr_t)&MCLSeqMidiEvents::onNoteOnCallback_Midi);
  Midi.removeOnNoteOffCallback(
      this, (midi_callback_ptr_t)&MCLSeqMidiEvents::onNoteOffCallback_Midi);
  Midi.removeOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&MCLSeqMidiEvents::onControlChangeCallback_Midi);
  Midi2.removeOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&MCLSeqMidiEvents::onControlChangeCallback_Midi2);

  state = false;
}

MCLSeq mcl_seq;
