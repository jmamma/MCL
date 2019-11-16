#include "LFO.h"
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
    md_tracks[i].resolution = 1;
  }

  for (uint8_t i = 0; i < num_lfo_tracks; i++) {
    lfo_tracks[i].track_number = i;
    if (i == 0) {
      lfo_tracks[i].params[0].dest = 17;
      lfo_tracks[i].params[1].dest = 18;
      lfo_tracks[i].params[0].param = 8;
      lfo_tracks[i].params[1].param = 8;
    }
  }
#ifdef EXT_TRACKS
  for (uint8_t i = 0; i < num_ext_tracks; i++) {
    ext_tracks[i].channel = i;
    ext_tracks[i].set_length(16);
    ext_tracks[i].resolution = 1;
  }
#endif
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
  for (uint8_t i = 0; i < num_lfo_tracks; i++) {
    lfo_tracks[i].update_params_offset();
  }
}

void MCLSeq::onMidiStartImmediateCallback() {
#ifdef EXT_TRACKS
  for (uint8_t i = 0; i < num_ext_tracks; i++) {
    // ext_tracks[i].start_clock32th = 0;
    ext_tracks[i].step_count = 0;
    ext_tracks[i].iterations = 1;
  }
#endif
  for (uint8_t i = 0; i < num_md_tracks; i++) {

    // md_tracks[i].start_clock32th = 0;
    md_tracks[i].step_count = 0;
    md_tracks[i].iterations = 1;
    md_tracks[i].oneshot_mask = 0;
  }

  for (uint8_t i = 0; i < num_lfo_tracks; i++) {
    lfo_tracks[i].sample_hold = 0;
    lfo_tracks[i].step_count = 0;
  }
}

void MCLSeq::onMidiStartCallback() {
  for (uint8_t i = 0; i < num_md_tracks; i++) {
    md_tracks[i].update_params();
    md_tracks[i].mute_state = SEQ_MUTE_OFF;
  }
  for (uint8_t i = 0; i < num_lfo_tracks; i++) {
    lfo_tracks[i].update_params_offset();
  }
}

void MCLSeq::onMidiStopCallback() {
#ifdef EXT_TRACKS
  for (uint8_t i = 0; i < num_ext_tracks; i++) {
    ext_tracks[i].buffer_notesoff();
  }
#endif
  for (uint8_t i = 0; i < num_md_tracks; i++) {
    md_tracks[i].reset_params();
  }
  for (uint8_t i = 0; i < num_lfo_tracks; i++) {
    lfo_tracks[i].reset_params_offset();
  }
}

#ifdef MEGACOMMAND
#pragma GCC push_options
#pragma GCC optimize("unroll-loops")
#endif
void MCLSeq::seq() {

  for (uint8_t i = 0; i < num_md_tracks; i++) {
    md_tracks[i].seq();
  }

  for (uint8_t i = 0; i < num_lfo_tracks; i++) {
    lfo_tracks[i].seq();
  }

#ifdef EXT_TRACKS
  for (uint8_t i = 0; i < num_ext_tracks; i++) {
    ext_tracks[i].seq();
  }
#endif
}
#ifdef MEGACOMMAND
#pragma GCC pop_options
#endif
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
    for (uint8_t n = 0; n < mcl_seq.num_lfo_tracks; n++) {
      mcl_seq.lfo_tracks[n].check_and_update_params_offset(track_param, value);
    }
  }
}

void MCLSeqMidiEvents::onControlChangeCallback_Midi2(uint8_t *msg) {
  uint8_t channel = MIDI_VOICE_CHANNEL(msg[0]);
  uint8_t param = msg[1];
  uint8_t value = msg[2];
#ifdef EXT_TRACKS
  if (channel < mcl_seq.num_ext_tracks) {
    if (param == 0x5E) {
      mcl_seq.ext_tracks[channel].mute_state = value;
    }
  }
#endif
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
#ifdef EXT_TRACKS
  Midi2.addOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&MCLSeqMidiEvents::onControlChangeCallback_Midi2);
#endif
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
  */
  Midi.removeOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&MCLSeqMidiEvents::onControlChangeCallback_Midi);

  Midi2.removeOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&MCLSeqMidiEvents::onControlChangeCallback_Midi2);

  state = false;
}

MCLSeq mcl_seq;
