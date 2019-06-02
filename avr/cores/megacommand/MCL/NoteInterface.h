/* Copyright 2018, Justin Mammarella jmamma@gmail.com */
// Takes MIDI Note input and turns it into a GUI button press.

#ifndef NOTEINTERFACE_H__
#define NOTEINTERFACE_H__

#include "GUI.h"

#define TRIG_HOLD_TIME 200

#define NI_MAX_NOTES 20

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
  uint8_t uart1_device = DEVICE_MD;
  uint8_t uart2_device = DEVICE_A4;
  uint8_t notes[NI_MAX_NOTES];
  uint8_t notecount = 0;
  uint8_t last_note;
  uint16_t note_hold = 0;
  bool note_proceed = false;
  bool state = true;
  void init_notes();
  void setup();
  void draw_notes(uint8_t line_number);
  uint8_t note_to_track_map(uint8_t note, uint8_t device);
  void note_on_event(uint8_t note_num, uint8_t port);
  void note_off_event(uint8_t note_num, uint8_t port);
  bool is_event(gui_event_t *event);
  bool notes_all_off();
  bool notes_all_off_md();
  uint8_t notes_count_off();
  uint8_t notes_count();

  NoteInterfaceMidiEvents ni_midi_events;
};

extern NoteInterface note_interface;

#endif /* NOTEINTERFACE_H__ */
