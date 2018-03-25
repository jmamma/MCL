/* Copyright 2018, Justin Mammarella jmamma@gmail.com */
#include "NoteInterface.h"

void NoteInterface::setup() { ni_midi_events.setup_callbacks(); }

void NoteInterface::init_notes() {
  for (uint8_t i = 0; i < 20; i++) {
    notes[i] = 0;
    // notes_off[i] = 0;
  }
}

bool NoteInterface::is_event(event_t *event) {
  if (event->source >= 128) {
    return true;
  }
  return false;
}
void NoteInterface::note_on_event(uint8_t note_num, uint8_t port) {

  if (!state) {
    return;
  }
  if (note_num > 20) {
    return;
  }
  if (notes[note_num] != 1) {
    notes[note_num] = 1;
  }
  if (note_num < 16) {
    note_hold = slow_clock;
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

  notes[note_num] = 3;

  gui_event_t event;
  event.source = i;
  event.mask = EVENT_BUTTON_RELEASED;
  event.port = port;
  EventRB.putp(&event);
}

uint8_t NoteInterface::note_to_track_map(uint8_t note, uint8_t device) {
  uint8_t note_to_track_map[7] = {0, 2, 4, 5, 7, 9, 11};
  for (uint8_t i = 0; i < 7; i++) {
    if (note_to_track_map[i] == (note - (note / 12) * 12)) {
      return i + 16;
    }
  }
}
bool NoteInterface::notes_all_off() {
  bool all_notes_off = false;
  uint8_t a = 0;
  uint8_t b = 0;
  for (uint8_t i = 0; i < 20; i++) {
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

uint8_t NoteInterface::notes_count_off() {
  uint8_t a = 0;
  for (uint8_t i = 0; i < 20; i++) {
    if (notes[i] == 3) {
      a++;
    }
  }
  return a;
}
uint8_t NoteInterface::notes_count() {
  uint8_t a = 0;
  for (uint8_t i = 0; i < 20; i++) {
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

  /*Display 16 track cues on screen,
   For 16 tracks check to see if there is a cue*/
  for (int i = 0; i < 16; i++) {
    if (curpage == CUE_PAGE) {

      if (IS_BIT_SET32(cfg.cues, i)) {
        str[i] = 'X';
      }
    }
    if (notes[i] > 0) {
      /*If the bit is set, there is a cue at this position. We'd like to display
       * it as [] on screen*/
      /*Char 219 on the minicommand LCD is a []*/

      str[i] = (char)219;
    }
  }

  /*Display the cues*/
  GUI.put_string_at(0, str);
}

void NoteIntefaceMidiEvents::onNoteOnCallback_Midi(uint8_t *msg) {
  if (midi_active_perring.uart1_device == DEVICE_MD) {
    return;
  }
  uint8_t note_num =
      note_to_track_map(msg[1], midi_active_perring.uart1_device);
  note_on_event(note_num, UART1_PORT);
}
void NoteIntefaceMidiEvents::onNoteOnCallback_Midi2(uint8_t *msg) {
  uint8_t note_num =
      note_to_track_map(msg[1], midi_active_perring.uart2_device);

  if (midi_active_peering.uart2_device == DEVICE_A4) {
    note_num += 16;
  }
  note_on_event(note_num, UART2_PORT);
}
void NoteIntefaceMidiEvents::onNoteOffCallback_Midi(uint8_t *msg) {
  // only accept input if device is not a MD
  // MD input is handled by the NoteInterface object
  if (midi_active_peering.uart1_device == DEVICE_MD) {
    return;
  }
  uint8_t note_num =
      note_to_track_map(msg[1], midi_active_perring.uart1_device);
  note_off_event(note_num, UART1_PORT);
}
void NoteIntefaceMidiEvents::onNoteOffCallback_Midi2(uint8_t *msg) {
  uint8_t note_num =
      note_to_track_map(msg[1], midi_active_perring.uart2_device);
  if (midi_active_peering.uart2_device == DEVICE_A4) {
    note_num += 16;
  }
  note_off_event(note_num, UART2_PORT);
}

void NoteInterfaceMidiEvents::setup_callbacks() {
  if (state) {
    return;
  }
  Midi.addOnNoteOnCallback(
      this,
      (midi_callback_ptr_t)&NoteInterfaceMidiEvents::onNoteOnCallback_Midi);
  Midi.addOnNoteOffCallback(
      this, (midi_callback_ptr_t)&NoteInterfaceMidiEvents::onNoteOffCallback_Midi;
  Midi2.addOnNoteOnCallback(
      this, (midi_callback_ptr_t)&NoteInterfaceMidiEvents::onNoteOnCallback_Midi2);
  Midi2.addOnNoteOffCallback(
      this, (midi_callback_ptr_t)&NoteInterfaceMidiEvents::onNoteOffCallback_Midi2);
 
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
      this, (midi_callback_ptr_t)&NoteInterfaceMidiEvents::onNoteOffCallback_Midi;
  Midi2.removeOnNoteOnCallback(
      this, (midi_callback_ptr_t)&NoteInterfaceMidiEvents::onNoteOnCallback_Midi2);
  Midi2.removeOnNoteOffCallback(
      this, (midi_callback_ptr_t)&NoteInterfaceMidiEvents::onNoteOffCallback_Midi2); 

  state = false;
}

NoteInterface note_interface;
