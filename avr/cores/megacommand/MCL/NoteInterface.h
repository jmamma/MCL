/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef NOTEINTERFACE_H__
#define NOTEINTERFACE_H__

#include "MCL.h"
#define NI_MAX_NOTES 20

typedef struct gui_event_mcl_s {
  uint8_t mask;
  uint8_t source;
  uint8_t port;
} gui_event_mcl_t;

class NoteInterface {
public:
  uint8_t notes[NI_MAX_NOTES];
  uint8_t notecount = 0;
  uint8_t last_note;
  bool note_proceed = false;
  bool state = enable;
  init_notes();
  void setup();
  void draw_notes(uint8_t line_number);
  void note_on_event(uint8_t note_num, uint8_t port);
  void note_off_event(uint8_t note_num, uint8_t port);
  bool is_event(event_t *event);
  bool notes_all_off();
  uint8_t notes_count_off();
  uint8_t notes_count();

  NoteInterfaceMidiEvents ni_midi_events;
};

class NoteInterfaceMidiEvents : public MidiCallBack {
  bool state;
  void setupCallbacks();
  void removeCallbacks();

  void onNoteOnCallback_Midi(uint8_t *msg);
  void onNoteOffCallback_Midi(uint8_t *msg);
  void onNoteOnCallback_Midi2(uint8_t *msg);
  void onNoteOffCallback_Midi2(uint8_t *msg);

}

extern NoteInterface note_interface;

#endif /* NOTEINTERFACE_H__ */
