/* Copyright 2018, Justin Mammarella jmamma@gmail.com */
#include "MCL_impl.h"

MCLActionsCallbacks mcl_actions_callbacks;
MCLActionsMidiEvents mcl_actions_midievents;

void MCLActionsMidiEvents::onProgramChangeCallback_Midi(uint8_t *msg) {
/*
  mcl_actions.kit_reload(msg[1]);
  mcl_actions.start_clock32th = MidiClock.div32th_counter;

  mcl_actions.start_clock96th = MidiClock.div96th_counter;
  if (MidiClock.state != 2) {
    MD.getBlockingKit(0x7F);
  }
*/
}

void MCLActionsMidiEvents::onNoteOnCallback_Midi(uint8_t *msg) {}
void MCLActionsMidiEvents::onNoteOffCallback_Midi(uint8_t *msg) {}
void MCLActionsMidiEvents::onControlChangeCallback_Midi(uint8_t *msg) {}

void MCLActionsCallbacks::StopHardCallback() {
  DEBUG_PRINTLN("BEGIN stop hard");
  uint8_t row_array[NUM_SLOTS];
  uint8_t slot_select_array[NUM_SLOTS] = {};
  bool proceed = false;


  for (uint8_t n = 0; n < NUM_SLOTS; n++) {
    if (mcl_actions.chains[n].is_mode_queue()) {
      slot_select_array[n] = 1;
      row_array[n] = mcl_actions.chains[n].rows[0];
      mcl_actions.chains[n].set_pos(0);
      proceed = true;
      grid_page.last_active_row = mcl_actions.chains[n].rows[0];
    }
  }

  if (!proceed) { goto end; }

  uint8_t _midi_lock_tmp = MidiUartParent::handle_midi_lock;
  MidiUartParent::handle_midi_lock = 1;
 /*
  ElektronDevice *elektron_devs[2] = {
      midi_active_peering.get_device(UART1_PORT)->asElektronDevice(),
      midi_active_peering.get_device(UART2_PORT)->asElektronDevice(),
  };

    for (uint8_t i = 0; i < NUM_DEVS; ++i) {
    if (elektron_devs[i] != nullptr &&
        elektron_devs[i]->canReadWorkspaceKit()) {
      elektron_devs[i]->getBlockingKit(0x7F);
    }
  }
  */
  DEBUG_PRINTLN("StopHard");
  DEBUG_PRINTLN((int)SP);
  mcl_actions.send_tracks_to_devices(slot_select_array, row_array);
  MidiUartParent::handle_midi_lock = _midi_lock_tmp;
end:
  DEBUG_PRINTLN("END stop hard");
  return;
}

void MCLActionsCallbacks::onMidiStartCallback() {
  DEBUG_PRINTLN("BEGIN on midi start");
  DEBUG_PRINTLN(SP);
  mcl_actions.start_clock32th = 0;
  mcl_actions.start_clock16th = 0;
  for (uint8_t n = 0; n < NUM_SLOTS; n++) {
    if (grid_page.active_slots[n] != SLOT_DISABLED) {
      mcl_actions.next_transitions[n] = 0;
      mcl_actions.transition_offsets[n] = 0;
      mcl_actions.chains[n].reset();
      mcl_actions.calc_next_slot_transition(n);
    }
  }
  mcl_actions.calc_next_transition();
  DEBUG_PRINTLN("END midi start");
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
  //  MidiClock.removeOnMidiContinueCallback(
  //    this, (midi_clock_callback_ptr_t)&MCLActionsCallbacks::onMidiStartCallback);

  state = false;
};

