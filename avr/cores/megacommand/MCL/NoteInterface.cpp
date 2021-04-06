/* Copyright 2018, Justin Mammarella jmamma@gmail.com */
#include "MCL_impl.h"

void NoteInterface::setup() { ni_midi_events.setup_callbacks(); }

void NoteInterface::init_notes() {
  memset(notes, 0, NI_MAX_NOTES);
  memset(note_hold, 0, NUM_DEVS);
}

bool NoteInterface::is_event(gui_event_t *event) {
  DEBUG_PRINTLN(event->source);
  if (event->source >= 128) {
    return true;
  }
  return false;
}
void NoteInterface::note_on_event(uint8_t note_num, uint8_t port) {
  if (!state) {
    DEBUG_PRINTLN("note interface disabled");
    return;
  }
  if (note_num > NI_MAX_NOTES) {
    return;
  }
  if (notes[note_num] != 1) {
    notes[note_num] = 1;
  }
  if (note_num < GRID_WIDTH) {
    note_hold[port] = slowclock;
  }
  if (IS_BIT_SET64(ignore_next_mask, note_num)) {
    CLEAR_BIT64(ignore_next_mask, note_num);
    return;
  }
  gui_event_t event;
  event.source = note_num + 128;
  event.mask = EVENT_BUTTON_PRESSED;
  event.port = port;
  EventRB.putp(&event);
}
void NoteInterface::note_off_event(uint8_t note_num, uint8_t port) {
  if (!state) {
    return;
  }
  DEBUG_PRINTLN(note_num);
  notes[note_num] = 3;
  if (IS_BIT_SET64(ignore_next_mask, note_num)) {
    CLEAR_BIT64(ignore_next_mask, note_num);
    return;
  }

  DEBUG_PRINTLN(F("note off"));
  gui_event_t event;
  event.source = note_num + 128;
  event.mask = EVENT_BUTTON_RELEASED;
  event.port = port;
  EventRB.putp(&event);
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
  uint8_t note = 255;
  for (uint8_t n = 0; n < NUM_MD_TRACKS; n++) {
    if (notes[n] == 1) {
      note = n;
      break;
    }
  }
  return note;
}

bool NoteInterface::notes_all_off_md() {
  bool all_notes_off = false;
  uint8_t a = 0;
  uint8_t b = 0;
  for (uint8_t i = 0; i < NUM_MD_TRACKS; i++) {
    if (notes[i] == 1) {
      a++;
    }
    if (notes[i] == 3) {
      b++;
    }
  }
  if ((a == 0) && (b > 0)) {
    all_notes_off = true;
  }
  return all_notes_off;
}

bool NoteInterface::notes_all_off() {
  bool all_notes_off = false;
  uint8_t a = 0;
  uint8_t b = 0;
  for (uint8_t i = 0; i < NI_MAX_NOTES; i++) {
    if (notes[i] == 1) {
      a++;
    }
    if (notes[i] == 3) {
      b++;
    }
  }
  if ((a == 0) && (b > 0)) {
    all_notes_off = true;
  }
  return all_notes_off;
}

uint8_t NoteInterface::notes_count_on() {
  uint8_t a = 0;
  for (uint8_t i = 0; i < NI_MAX_NOTES; i++) {
    if (notes[i] == 1) {
      a++;
    }
  }
  return a;
}

uint8_t NoteInterface::notes_count_off() {
  uint8_t a = 0;
  for (uint8_t i = 0; i < NI_MAX_NOTES; i++) {
    if (notes[i] == 3) {
      a++;
    }
  }
  return a;
}
uint8_t NoteInterface::notes_count() {
  uint8_t a = 0;
  for (uint8_t i = 0; i < NI_MAX_NOTES; i++) {
    if (notes[i] > 0) {
      a++;
    }
  }
  return a;
}

void NoteInterface::draw_notes(uint8_t line_number) {
  if (line_number == 0) {
    GUI.setLine(GUI.LINE1);
  } else {
    GUI.setLine(GUI.LINE2);
  }
  /*Initialise the string with blank steps*/
  char str[17] = "----------------";

  for (int i = 0; i < 16; i++) {
    if (notes[i] > 0 && notes[i] != 3) {

#ifdef OLED_DISPLAY
      str[i] = (char)2;
#else
      str[i] = (char)219;
#endif
    }
  }

  GUI.put_string_at(0, str);
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
  // MD input is handled by the NoteInterface object
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
  Midi.addOnNoteOnCallback(
      this,
      (midi_callback_ptr_t)&NoteInterfaceMidiEvents::onNoteOnCallback_Midi);
  Midi.addOnNoteOffCallback(
      this,
      (midi_callback_ptr_t)&NoteInterfaceMidiEvents::onNoteOffCallback_Midi);
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
  Midi.removeOnNoteOnCallback(
      this,
      (midi_callback_ptr_t)&NoteInterfaceMidiEvents::onNoteOnCallback_Midi);
  Midi.removeOnNoteOffCallback(
      this,
      (midi_callback_ptr_t)&NoteInterfaceMidiEvents::onNoteOffCallback_Midi);
  Midi2.removeOnNoteOnCallback(
      this,
      (midi_callback_ptr_t)&NoteInterfaceMidiEvents::onNoteOnCallback_Midi2);
  Midi2.removeOnNoteOffCallback(
      this,
      (midi_callback_ptr_t)&NoteInterfaceMidiEvents::onNoteOffCallback_Midi2);

  state = false;
}

NoteInterface note_interface;
