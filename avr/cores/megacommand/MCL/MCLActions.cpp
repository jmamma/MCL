/* Copyright 2018, Justin Mammarella jmamma@gmail.com */
#include "MCLActions.h"

void MCLActions::setup() {}
void MCLActionsMidiEvents::onProgramChangeCallback(uint8_t *msg) {
      load_the_damnkit(msg[1]);
      pattern_start_clock32th = MidiClock.div32th_counter;
}
 
void MCLActionsMidiEvents::setup_callbacks() {
  if (state) {
    return;
  }
  Midi.addOnProgramChangeCallback(
      this, (midi_callback_ptr_t)&MCLActionsMidiEvents::onProgramChangeCallback);

  Midi.addOnNoteOnCallback(
      this, (midi_callback_ptr_t)&MCLActionsMidiEvents::onNoteOnCallback);
  Midi.addOnNoteOffCallback(
      this, (midi_callback_ptr_t)&MCLActionsMidiEvents::onNoteOffCallback);
  Midi.addOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&MCLActionsMidiEvents::onControlChangeCallback);

  state = true;
}

void MCLActionsMidiEvents::remove_callbacks() {

  if (!state) {
    return;
  }
  Midi.removeOnProgramChangeCallback(
      this, (midi_callback_ptr_t)&MCLActionsMidiEvents::onProgramChangeCallback);

  Midi.removeOnNoteOnCallback(
      this, (midi_callback_ptr_t)&MCLActionsMidiEvents::onNoteOnCallback);
  Midi.removeOnNoteOffCallback(
      this, (midi_callback_ptr_t)&MCLActionsMidiEvents::onNoteOffCallback);
  Midi.removeOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&MCLActionsMidiEvents::onControlChangeCallback);

  state = false;
}
void MCLActionsCallbacks::setup_callbacks() {
  if (state) {
    return;
  }
  MidiClock.addOnMidiStartCallback(
      this, (midi_clock_callback_ptr_t)&TrigInterface::onMidiStartCallback);
  MidiClock.addOnMidiContinueCallback(
      this, (midi_clock_callback_ptr_t)&TrigInterface::onMidiStartCallback);

  state = true;
};
void MCLActionsCallbacks::remove_callbacks() {
  if (!state) {
    return;
  }
  MidiClock.removeOnMidiStartCallback(
      this, (midi_clock_callback_ptr_t)&TrigInterface::onMidiStartCallback);
  MidiClock.removeOnMidiContinueCallback(
      this, (midi_clock_callback_ptr_t)&TrigInterface::onMidiStartCallback);

  state = false;
};
void MCLActionsMidiEvents::onNoteOnCallback(uint8_t *msg) {
}
void MCLActionsMidiEvents::onNoteOffCallback(uint8_t *msg) {
}

void MCLActionsCallbacks::onNoteOffCallback(uint8_t *msg) {}

void MCLActionsCallbacks::onControlChangeCallback(uint8_t *msg) {
}

void MCLActionsCallbacks::onMidiStartCallback() {
}

MCLActions mcl_actions;
