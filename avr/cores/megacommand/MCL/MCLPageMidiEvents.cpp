#include "MCLPageMidiEvents.h"

void MCLPageMidiEvents::setup_callbacks() {
  Midi.addOnNoteOnCallback(
      this, (midi_callback_ptr_t)&MCLPageMidiEvents::onNoteOnCallback_Midi);
  Midi.addOnNoteOffCallback(
      this, (midi_callback_ptr_t)&MCLPageMidiEvents::onNoteOffCallback_Midi);

  Midi2.addOnNoteOnCallback(
      this, (midi_callback_ptr_t)&MCLPageMidiEvents::onNoteOnCallback_Midi2);
  Midi2.addOnNoteOffCallback(
      this, (midi_callback_ptr_t)&MCLPageMidiEvents::onNoteOffCallback_Midi2);

  Midi.addOnControlChangeCallback(
      this, (midi_callback_ptr_t)&MCLPageMidiEvents::onControlChangeCallback_Midi);
  Midi2.addOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&MCLPageMidiEvents::onControlChangeCallback_Midi2);
}

void MCLPageMidiEvents::remove_callbacks() {
  Midi.removeOnNoteOnCallback(
      this, (midi_callback_ptr_t)&MCLPageMidiEvents::onNoteOnCallback_Midi);
  Midi2.removeOnNoteOffCallback(
      this, (midi_callback_ptr_t)&MCLPageMidiEvents::onNoteOffCallback_Midi2);

  Midi.removeOnNoteOffCallback(
      this, (midi_callback_ptr_t)&MCLPageMidiEvents::onNoteOffCallback_Midi);
  Midi2.removeOnNoteOnCallback(
      this, (midi_callback_ptr_t)&MCLPageMidiEvents::onNoteOnCallback_Midi2);

  Midi.removeOnControlChangeCallback(
      this, (midi_callback_ptr_t)&MCLPageMidiEvents::onControlChangeCallback_Midi);
  Midi2.removeOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&MCLPageMidiEvents::onControlChangeCallback_Midi2);
}
/*
void MCLPageMidiEvents::onNoteOnCallback_Midi(uint8_t *msg);
void MCLPageMidiEvents::onNoteOffCallback_Midi(uint8_t *msg);
void MCLPageMidiEvents::onNoteOnCallback_Midi2(uint8_t *msg);
void MCLPageMidiEvents::onNoteOffCallback_Midi2(uint8_t *msg);
void MCLPageMidiEvents::onControlChangeCallback_Midi(uint8_t *msg);
void MCLPageMidiEvents::onControlChangeCallback_Midi2(uint8_t *msg);
*/
