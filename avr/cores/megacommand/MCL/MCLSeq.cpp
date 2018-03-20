#include "MCLSeq.h"

void MCLSeq::setup() {
  for (uint8_t i = 0; i < NUM_PARAM_PAGES; i++ 2) {
    seq_param_page[i].setEncoders(&seq_param1, &seq_param2, &seq_param3,
                                  &seq_param4);
    seq_param_page[i].init(i, i + 1);
  }
    for (uint8_t i = 0; i < num_md_tracks; i++) {
    md_tracks[i].track_number = i;
    }

    for (uint8_t i = 0; i < num_ext_tracks; i++) {
    ext_tracks[i].track_number = i;
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
    ext_tracks.seq_buffer_notesoff();
  }
}


void MCLSeq::sequencer() {


  //   MD.setTrackParam(1,0,random(127));
  if (in_sysex == 0) {

    for (uint8_t i = 0; i < num_md_tracks; i++) {
    md_tracks[i].seq();
  }

    for (uint8_t i = 0; i < num_ext_tracks; i++) {
    ext_tracks[i].seq();
    }
 
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
  if (channel < num_ext_tracks) {
  if (param == 0x5E) {
    ext_tracks[channel].mute = value;
  }
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
