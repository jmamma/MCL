/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef MCLACTIONSEVENTS_H__
#define MCLACTIONSEVENTS_H__

#include "MidiClock.h"
#include "midi-common.h"

class MCLActionsMidiEvents : public MidiCallback {
public:
  bool state;
  uint32_t slot_mask;
  uint8_t note_to_slot(uint8_t note);
  void setup_callbacks();
  void remove_callbacks();
  void onProgramChangeCallback_Midi2(uint8_t *msg);
  // Callbacks for intercepting MIDI notes for slot loading
  void onNoteOnCallback_Midi2(uint8_t *msg);
  void onNoteOffCallback_Midi2(uint8_t *msg);
  // Control change callback required to disable exploit if CC received
  // Any updating of MD display causes exploit to fail.
  void onControlChangeCallback_Midi(uint8_t *msg);
};

class MCLActionsCallbacks : public ClockCallback {
public:
  bool state;
  void onMidiStartCallback();
  void onMidiStopCallback();
  void StopHardCallback();
  void setup_callbacks();
  void remove_callbacks();
};

#endif /* MCLACTIONSEVENTS_H__ */
