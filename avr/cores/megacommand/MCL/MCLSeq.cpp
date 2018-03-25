#include "MCLSeq.h"
#include "MCL.h"

void MCLSeq::setup() {
  for (uint8_t i = 0; i < NUM_PARAM_PAGES; i = i + 2) {
    seq_param_page[i].setEncoders(&seq_param1, &seq_param2, &seq_param3,
                                  &seq_param4);
    seq_param_page[i].construct(i, i + 1);
    seq_param_page[i].page_id = i;

  }
  for (uint8_t i = 0; i < num_md_tracks; i++) {
    md_tracks[i].track_number = i;
  }

  for (uint8_t i = 0; i < num_ext_tracks; i++) {
    ext_tracks[i].channel = i;
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
      this, (midi_callback_ptr_t)&MCLSeqMidiEvents::onControlChangeCallback_Midi);
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
      this, (midi_callback_ptr_t)&MCLSeqMidiEvents::onControlChangeCallback_Midi);
  Midi2.removeOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&MCLSeqMidiEvents::onControlChangeCallback_Midi2);

  state = false;
}

MCLSeq mcl_seq;
