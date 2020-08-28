/* Copyright 2018, Justin Mammarella jmamma@gmail.com */
#include "MCL.h"
#include "MCLActions.h"
#include "MCLActionsEvents.h"

MCLActionsCallbacks mcl_actions_callbacks;
MCLActionsMidiEvents mcl_actions_midievents;

void MCLActionsMidiEvents::onProgramChangeCallback_Midi(uint8_t *msg) {
  mcl_actions.kit_reload(msg[1]);
  mcl_actions.start_clock32th = MidiClock.div32th_counter;

  mcl_actions.start_clock96th = MidiClock.div96th_counter;
  if (MidiClock.state != 2) {
    MD.getBlockingKit(0xF7);
  }
}

void MCLActionsMidiEvents::onNoteOnCallback_Midi(uint8_t *msg) {}
void MCLActionsMidiEvents::onNoteOffCallback_Midi(uint8_t *msg) {}
void MCLActionsMidiEvents::onControlChangeCallback_Midi(uint8_t *msg) {}

void MCLActionsCallbacks::onMidiStopCallback() {
 DEBUG_PRINTLN("initialising nearest steps");
//   memset(&mcl_actions.next_transitions[0], 0, 20);
/*
  for (uint8_t n = 0; n < NUM_TRACKS; n++) {
  mcl_actions.next_transitions[n] = 0;
  if (mcl_cfg.chain_mode != 2) { mcl_actions.calc_next_slot_transition(n); }
  }
  if (mcl_cfg.chain_mode != 2) { mcl_actions.calc_next_transition(); }
  else { mcl_actions.next_transition = (uint16_t) -1; }
*/
 }

void MCLActionsCallbacks::onMidiStartCallback() {
  mcl_actions.start_clock32th = 0;
  mcl_actions.start_clock16th = 0;
  for (uint8_t n = 0; n < NUM_TRACKS; n++) {
    if (grid_page.active_slots[n] >= 0) {
      mcl_actions.next_transitions[n] = 0;
      mcl_actions.transition_offsets[n] = 0;
      if (mcl_cfg.chain_mode != 2) { mcl_actions.calc_next_slot_transition(n); }
    }
  }
  if (mcl_cfg.chain_mode != 2) { mcl_actions.calc_next_transition(); }
  else { mcl_actions.next_transition = (uint16_t) -1; }
}

void MCLActionsMidiEvents::setup_callbacks() {
  if (state) {
    return;
  }
  Midi.addOnProgramChangeCallback(
      this,
      (midi_callback_ptr_t)&MCLActionsMidiEvents::onProgramChangeCallback_Midi);

  Midi.addOnNoteOnCallback(
      this, (midi_callback_ptr_t)&MCLActionsMidiEvents::onNoteOnCallback_Midi);
  Midi.addOnNoteOffCallback(
      this, (midi_callback_ptr_t)&MCLActionsMidiEvents::onNoteOffCallback_Midi);
  Midi.addOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&MCLActionsMidiEvents::onControlChangeCallback_Midi);

  state = true;
}

void MCLActionsMidiEvents::remove_callbacks() {

  if (!state) {
    return;
  }
  Midi.removeOnProgramChangeCallback(
      this,
      (midi_callback_ptr_t)&MCLActionsMidiEvents::onProgramChangeCallback_Midi);

  Midi.removeOnNoteOnCallback(
      this, (midi_callback_ptr_t)&MCLActionsMidiEvents::onNoteOnCallback_Midi);
  Midi.removeOnNoteOffCallback(
      this, (midi_callback_ptr_t)&MCLActionsMidiEvents::onNoteOffCallback_Midi);
  Midi.removeOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&MCLActionsMidiEvents::onControlChangeCallback_Midi);

  state = false;
}
void MCLActionsCallbacks::setup_callbacks() {
  if (state) {
    return;
  }
  MidiClock.addOnMidiStartCallback(
      this, (midi_clock_callback_ptr_t)&MCLActionsCallbacks::onMidiStartCallback);
  MidiClock.addOnMidiStopCallback(
      this, (midi_clock_callback_ptr_t)&MCLActionsCallbacks::onMidiStopCallback);
//  MidiClock.addOnMidiContinueCallback(
  //    this, (midi_clock_callback_ptr_t)&MCLActionsCallbacks::onMidiStartCallback);

  state = true;
};
void MCLActionsCallbacks::remove_callbacks() {
  if (!state) {
    return;
  }
  MidiClock.removeOnMidiStartCallback(
      this, (midi_clock_callback_ptr_t)&MCLActionsCallbacks::onMidiStartCallback);
  MidiClock.removeOnMidiStopCallback(
      this, (midi_clock_callback_ptr_t)&MCLActionsCallbacks::onMidiStopCallback);
  //  MidiClock.removeOnMidiContinueCallback(
  //    this, (midi_clock_callback_ptr_t)&MCLActionsCallbacks::onMidiStartCallback);

  state = false;
};

