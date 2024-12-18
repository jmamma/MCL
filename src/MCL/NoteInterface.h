/* Copyright 2018, Justin Mammarella jmamma@gmail.com */
// Takes MIDI Note input and turns it into a GUI button press.

#ifndef NOTEINTERFACE_H__
#define NOTEINTERFACE_H__

#include "MidiID.h"
#include "GUI.h"
#include "MCLMemory.h"
#include "midi-common.h"

#define TRIG_HOLD_TIME 450

#define NI_MAX_NOTES NUM_SLOTS

class NoteInterfaceMidiEvents : public MidiCallback {
public:
  bool state;
  void setup_callbacks();
  void remove_callbacks();

  void onNoteOnCallback_Midi(uint8_t *msg);
  void onNoteOffCallback_Midi(uint8_t *msg);
  void onNoteOnCallback_Midi2(uint8_t *msg);
  void onNoteOffCallback_Midi2(uint8_t *msg);
};

class NoteInterface {
public:
  uint8_t uart2_device = DEVICE_A4;

  uint32_t notes_on;
  uint32_t notes_off;
  uint32_t notes_ignore;
  uint8_t last_note;
  uint16_t note_hold[NUM_DEVS];
  bool note_proceed = false;
  bool state = true;
  void init_notes();
  void setup();
  void draw_notes(uint8_t line_number);
  uint8_t note_to_track_map(uint8_t note, uint8_t device);

  void add_note_event(uint8_t note_num, uint8_t event_mask, uint8_t port);
  void note_on_event(uint8_t note_num, uint8_t port);
  void note_off_event(uint8_t note_num, uint8_t port);
  bool is_event(gui_event_t *event) { return event->source >= 128; }
  bool notes_all_off() { return ((notes_on == 0) && (notes_off > 0)); }
  bool notes_all_off_md() { return (((uint16_t) notes_on == 0) && ((uint16_t)notes_off > 0)); }
  uint8_t notes_count_on();
  uint8_t notes_count_off();
  uint8_t notes_count();
  uint8_t get_first_md_note();

  bool is_note(uint8_t note_num);
  bool is_note_on(uint8_t note_num) { return IS_BIT_SET32(notes_on, note_num); }
  bool is_note_off(uint8_t note_num) { return IS_BIT_SET32(notes_off, note_num); }
  void clear_note(uint8_t note_num);

  void ignoreNextEvent(uint8_t i) {
    SET_BIT32(notes_ignore, i);
  }
  NoteInterfaceMidiEvents ni_midi_events;
};

extern NoteInterface note_interface;

#endif /* NOTEINTERFACE_H__ */
