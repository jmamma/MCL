/* Copyright 2018, Justin Mammarella jmamma@gmail.com */
#include "MCL_impl.h"

void NoteInterface::setup() { ni_midi_events.setup_callbacks(); }

void NoteInterface::init_notes() {
   notes_on = 0;
   notes_off = 0;
   //notes_ignore = 0;
   memset(note_hold, 0, sizeof(note_hold));
}

bool NoteInterface::is_note(uint8_t note_num) {
   uint32_t mask = notes_on | notes_off;
   return IS_BIT_SET32(mask, note_num);
}

void NoteInterface::clear_note(uint8_t note_num) {
   if (note_num < NI_MAX_NOTES) {
     CLEAR_BIT32(notes_on, note_num);
     CLEAR_BIT32(notes_off, note_num);
   }
}

void NoteInterface::add_note_event(uint8_t note_num, uint8_t event_mask, uint8_t port) {
  gui_event_t event;
  event.source = note_num + 128;
  event.mask = event_mask;
  event.port = port;
  EventRB.putp(&event);
}

void NoteInterface::note_on_event(uint8_t note_num, uint8_t port) {
  if (!state) {
    return;
  }
  if (note_num > NI_MAX_NOTES) {
    return;
  }
  if (IS_BIT_SET32(notes_ignore, note_num)) {
    CLEAR_BIT32(notes_ignore, note_num);
    return;
  }
  SET_BIT32(notes_on, note_num);
  CLEAR_BIT32(notes_off, note_num);

  if (note_num < GRID_WIDTH) {
    note_hold[port] = slowclock;
  }
  add_note_event(note_num, EVENT_BUTTON_PRESSED, port);
}
void NoteInterface::note_off_event(uint8_t note_num, uint8_t port) {
  if (!state) {
    return;
  }
  if (IS_BIT_SET32(notes_ignore, note_num)) {
    CLEAR_BIT32(notes_ignore, note_num);
    return;
  }
  CLEAR_BIT32(notes_on, note_num);
  SET_BIT32(notes_off, note_num);
  add_note_event(note_num, EVENT_BUTTON_RELEASED, port);
}

uint8_t NoteInterface::note_to_track_map(uint8_t note, uint8_t device) {
  uint8_t note_to_track_map[7] = {0, 2, 4, 5, 7, 9, 11};
  for (uint8_t i = 0; i < 7; i++) {
    if (note_to_track_map[i] == (note - (note / 12) * 12)) {
      if (device == DEVICE_A4) {
        return i + NUM_MD_TRACKS;
      }

      return i;
    }
  }
  return 255;
}

uint8_t NoteInterface::get_first_md_note() {
  uint32_t on = notes_on;

  uint8_t n = 0;
  while (on) { 
    if (on & 1) {
      return n;
    }
    n++;
    on >>= 1;
  }
  return 255;
}

uint8_t NoteInterface::notes_count_on() {
  return popcount32(notes_on);
}
uint8_t NoteInterface::notes_count_off() {
  return popcount32(notes_off);
}
uint8_t NoteInterface::notes_count() {
  return popcount32(notes_off | notes_on);
}

void NoteInterface::draw_notes(uint8_t line_number) {
}

void NoteInterfaceMidiEvents::onNoteOnCallback_Midi(uint8_t *msg) {
  if (midi_active_peering.get_device(UART1_PORT) == &MD) {
    return;
  }
  uint8_t note_num = note_interface.note_to_track_map(
      msg[1], midi_active_peering.get_device(UART1_PORT)->id);
  note_interface.note_on_event(note_num, UART1_PORT);
}
void NoteInterfaceMidiEvents::onNoteOnCallback_Midi2(uint8_t *msg) {

  if (midi_active_peering.get_device(UART2_PORT)->id !=
      note_interface.uart2_device) {
    return;
  }
  uint8_t note_num = note_interface.note_to_track_map(
      msg[1], midi_active_peering.get_device(UART2_PORT)->id);
  DEBUG_PRINTLN(note_num);
  note_interface.note_on_event(note_num, UART2_PORT);
}
void NoteInterfaceMidiEvents::onNoteOffCallback_Midi(uint8_t *msg) {
  // only accept input if device is not a MD
  // MD input is handled by the TrigInterface object
  if (midi_active_peering.get_device(UART1_PORT) == &MD) {
    return;
  }
  uint8_t note_num = note_interface.note_to_track_map(
      msg[1], midi_active_peering.get_device(UART1_PORT)->id);
  note_interface.note_off_event(note_num, UART1_PORT);
}
void NoteInterfaceMidiEvents::onNoteOffCallback_Midi2(uint8_t *msg) {

  if (midi_active_peering.get_device(UART2_PORT)->id !=
      note_interface.uart2_device) {
    return;
  }

  uint8_t note_num = note_interface.note_to_track_map(
      msg[1], midi_active_peering.get_device(UART2_PORT)->id);
  DEBUG_PRINTLN(F("note to track"));
  DEBUG_PRINTLN(note_num);
  note_interface.note_off_event(note_num, UART2_PORT);
}

void NoteInterfaceMidiEvents::setup_callbacks() {
  if (state) {
    return;
  }
/*
  Midi.addOnNoteOnCallback(
      this,
      (midi_callback_ptr_t)&NoteInterfaceMidiEvents::onNoteOnCallback_Midi);
  Midi.addOnNoteOffCallback(
      this,
      (midi_callback_ptr_t)&NoteInterfaceMidiEvents::onNoteOffCallback_Midi);
*/
  Midi2.addOnNoteOnCallback(
      this,
      (midi_callback_ptr_t)&NoteInterfaceMidiEvents::onNoteOnCallback_Midi2);
  Midi2.addOnNoteOffCallback(
      this,
      (midi_callback_ptr_t)&NoteInterfaceMidiEvents::onNoteOffCallback_Midi2);
  state = true;
}

void NoteInterfaceMidiEvents::remove_callbacks() {

  if (!state) {
    return;
  }
/*
  Midi.removeOnNoteOnCallback(
      this,
      (midi_callback_ptr_t)&NoteInterfaceMidiEvents::onNoteOnCallback_Midi);
  Midi.removeOnNoteOffCallback(
      this,
      (midi_callback_ptr_t)&NoteInterfaceMidiEvents::onNoteOffCallback_Midi);
*/
  Midi2.removeOnNoteOnCallback(
      this,
      (midi_callback_ptr_t)&NoteInterfaceMidiEvents::onNoteOnCallback_Midi2);
  Midi2.removeOnNoteOffCallback(
      this,
      (midi_callback_ptr_t)&NoteInterfaceMidiEvents::onNoteOffCallback_Midi2);

  state = false;
}

NoteInterface note_interface;
