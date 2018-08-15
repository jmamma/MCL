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
      }
    } */

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
  MidiClock.addOn192Callback(this, (midi_clock_callback_ptr_t)&MCLSeq::seq);

  MidiClock.addOnMidiStopCallback(
      this, (midi_clock_callback_ptr_t)&MCLSeq::onMidiStopCallback);
};

void MCLSeq::onMidiStopCallback() {
  for (uint8_t i = 0; i < 4; i++) {
    ext_tracks[i].buffer_notesoff();
  }
}

void MCLSeq::seq() {

  if (in_sysex == 0) {

  //  for (uint8_t i = 0; i < 1; i++) {
  //    lfos[i].seq();
  //  }

    for (uint8_t i = 0; i < num_md_tracks; i++) {
      md_tracks[i].seq();
    }
  }
  if (in_sysex2 == 0) {
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
