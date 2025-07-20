/* Copyright 2018, Justin Mammarella jmamma@gmail.com */
#include "MCLActionsEvents.h"
#include "MCLSysConfig.h"
#include "MCLMemory.h"
#include "GridPages.h"
#include "GridTask.h"
#include "StackMonitor.h"
#include "MCLActions.h"

MCLActionsCallbacks mcl_actions_callbacks;
MCLActionsMidiEvents mcl_actions_midievents;

void MCLActionsMidiEvents::onProgramChangeCallback_Midi2(uint8_t *msg) {

  if (mcl_cfg.uart2_prg_in - 1 == MIDI_VOICE_CHANNEL(msg[0]) ||
      (mcl_cfg.uart2_prg_in == MIDI_OMNI_MODE)) {

    if (mcl_cfg.uart2_prg_mode == 0) {
      uint8_t track_select_array[NUM_SLOTS] = {};
      grid_load_page.track_select_array_from_type_select(track_select_array);
      grid_task.load_queue.put(mcl_cfg.load_mode,msg[1],track_select_array);
    } else {
      DEBUG_PRINTLN("set row");
      grid_task.midi_row_select = msg[1];
    }
  }
}

uint8_t MCLActionsMidiEvents::note_to_slot(uint8_t note) {
   /* disable black keys
  const uint16_t chromatic = 0b0000010101001010;
  uint8_t o = note / 12;
  uint8_t p = note - o * 12;
  uint16_t mask = 0;
  SET_BIT16(mask, p + 1);
  mask = mask - 1;
  return p - popcount16(chromatic & mask) + 7 * o;
  */
  return note;
}

void MCLActionsMidiEvents::onNoteOnCallback_Midi2(uint8_t *msg) {
  if (mcl_cfg.uart2_prg_in - 1 == MIDI_VOICE_CHANNEL(msg[0]) ||
      (mcl_cfg.uart2_prg_in == MIDI_OMNI_MODE)) {

    if (msg[1] < MIDI_NOTE_C4) {
      return;
    }
    uint8_t slot = note_to_slot(msg[1] - MIDI_NOTE_C4);
    if (slot > NUM_SLOTS - 1) {
      return;
    }

    SET_BIT32(slot_mask, slot);

    if (grid_task.midi_row_select == 255) {
      grid_task.midi_row_select = grid_page.getRow();
    }

    DEBUG_PRINT("Selecting slot: ");
    DEBUG_PRINT(slot);
    DEBUG_PRINT(" ");
    DEBUG_PRINTLN(grid_task.midi_row_select);
    grid_task.load_track_select[slot] = grid_task.midi_row_select;
  }
}

void MCLActionsMidiEvents::onNoteOffCallback_Midi2(uint8_t *msg) {
  if (mcl_cfg.uart2_prg_in - 1 == MIDI_VOICE_CHANNEL(msg[0]) ||
      (mcl_cfg.uart2_prg_in == MIDI_OMNI_MODE)) {

    if (msg[1] < MIDI_NOTE_C4) {
      return;
    }
    uint8_t slot = note_to_slot(msg[1] - MIDI_NOTE_C4);
    if (slot > NUM_SLOTS - 1) {
      return;
    }

    CLEAR_BIT32(slot_mask, slot);

    if (slot_mask == 0) {
      grid_task.load_queue.put(mcl_cfg.load_mode,grid_task.load_track_select);
      memset(grid_task.load_track_select, 255, sizeof(grid_task.load_track_select));
    }
  }
}

void MCLActionsMidiEvents::onControlChangeCallback_Midi(uint8_t *msg) {}

void MCLActionsCallbacks::StopHardCallback() {
  DEBUG_PRINTLN("BEGIN stop hard");
  uint8_t row_array[NUM_SLOTS];
  uint8_t slot_select_array[NUM_SLOTS] = {};
  bool proceed = false;

  for (uint8_t n = 0; n < NUM_SLOTS; n++) {
    GridDeviceTrack *gdt =
        mcl_actions.get_grid_dev_track(n);

    if (gdt == nullptr)
      continue;

    if (mcl_actions.chains[n].is_mode_queue()) {
      slot_select_array[n] = 1;
      row_array[n] = mcl_actions.chains[n].rows[0];
      mcl_actions.chains[n].set_pos(0);
      proceed = true;
      grid_task.last_active_row = mcl_actions.chains[n].rows[0];
    }
  }

  uint8_t _midi_lock_tmp = MidiUartParent::handle_midi_lock;
  if (!proceed) {
    goto end;
  }

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
  StackMonitor::print_stack_info();
  mcl_actions.send_tracks_to_devices(slot_select_array, row_array);
  MidiUartParent::handle_midi_lock = _midi_lock_tmp;
end:
  DEBUG_PRINTLN("END stop hard");
  grid_task.reset_midi_states();
  mcl_actions_midievents.slot_mask = 0;
  return;
}

void MCLActionsCallbacks::onMidiStartCallback() {
  DEBUG_PRINTLN("BEGIN on midi start");
  StackMonitor::print_stack_info();
  mcl_actions.start_clock32th = 0;
  mcl_actions.start_clock16th = 0;
  for (uint8_t n = 0; n < NUM_SLOTS; n++) {
    GridDeviceTrack *gdt =
        mcl_actions.get_grid_dev_track(n);

    if (gdt == nullptr)
      continue;

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
  slot_mask = 0;
  MidiUSB.addOnProgramChangeCallback(this,
                                   (midi_callback_ptr_t)&MCLActionsMidiEvents::
                                       onProgramChangeCallback_Midi2);

  Midi2.addOnProgramChangeCallback(this,
                                   (midi_callback_ptr_t)&MCLActionsMidiEvents::
                                       onProgramChangeCallback_Midi2);

  MidiUSB.addOnNoteOnCallback(
      this, (midi_callback_ptr_t)&MCLActionsMidiEvents::onNoteOnCallback_Midi2);
  MidiUSB.addOnNoteOffCallback(
      this,
      (midi_callback_ptr_t)&MCLActionsMidiEvents::onNoteOffCallback_Midi2);
  Midi2.addOnNoteOnCallback(
      this, (midi_callback_ptr_t)&MCLActionsMidiEvents::onNoteOnCallback_Midi2);
  Midi2.addOnNoteOffCallback(
      this,
      (midi_callback_ptr_t)&MCLActionsMidiEvents::onNoteOffCallback_Midi2);
  Midi.addOnControlChangeCallback(
      this,
      (midi_callback_ptr_t)&MCLActionsMidiEvents::onControlChangeCallback_Midi);

  state = true;
}

void MCLActionsMidiEvents::remove_callbacks() {

  if (!state) {
    return;
  }
  MidiUSB.removeOnProgramChangeCallback(
      this, (midi_callback_ptr_t)&MCLActionsMidiEvents::
                onProgramChangeCallback_Midi2);


  Midi2.removeOnProgramChangeCallback(
      this, (midi_callback_ptr_t)&MCLActionsMidiEvents::
                onProgramChangeCallback_Midi2);
  MidiUSB.removeOnNoteOnCallback(
      this, (midi_callback_ptr_t)&MCLActionsMidiEvents::onNoteOnCallback_Midi2);
  MidiUSB.removeOnNoteOffCallback(
      this,
      (midi_callback_ptr_t)&MCLActionsMidiEvents::onNoteOffCallback_Midi2);
  Midi2.removeOnNoteOnCallback(
      this, (midi_callback_ptr_t)&MCLActionsMidiEvents::onNoteOnCallback_Midi2);
  Midi2.removeOnNoteOffCallback(
      this,
      (midi_callback_ptr_t)&MCLActionsMidiEvents::onNoteOffCallback_Midi2);
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
      this,
      (midi_clock_callback_ptr_t)&MCLActionsCallbacks::onMidiStartCallback);
  //  MidiClock.addOnMidiContinueCallback(
  //    this,
  //    (midi_clock_callback_ptr_t)&MCLActionsCallbacks::onMidiStartCallback);

  state = true;
};
void MCLActionsCallbacks::remove_callbacks() {
  if (!state) {
    return;
  }
  MidiClock.removeOnMidiStartCallback(
      this,
      (midi_clock_callback_ptr_t)&MCLActionsCallbacks::onMidiStartCallback);
  //  MidiClock.removeOnMidiContinueCallback(
  //    this,
  //    (midi_clock_callback_ptr_t)&MCLActionsCallbacks::onMidiStartCallback);

  state = false;
};
