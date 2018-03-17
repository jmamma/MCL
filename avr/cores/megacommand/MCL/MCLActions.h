/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef MCLACTIONS_H__
#define MCLACTIONS_H__

#include "MCL.h"

class MCLActions {
public:

  MCLActionsCallbacks mcl_actions_callbacks;
  MCLActionsMidiEvents mcl_actions_midievents;
 
  MCLActions() {}
  void setup();
  void send_globals();
  void switch_global(uint8_t global_page);
  bool on();
  bool off();
};

extern MCLActions mcl_actions;

class MCLActionsMidiEvents : public MidiCallBack {
  bool state;

  void setup_callbacks();
  void remove_callbacks();

  void OnProgramChangeCallback(uint8_t *msg);
  // Callbacks for intercepting MD triggers as GUI input
  void onNoteOnCallback_Midi(uint8_t *msg);
  void onNoteOffCallback_Midi(uint8_t *msg);
  // Control change callback required to disable exploit if CC received
  // Any updating of MD display causes exploit to fail.
  void onControlChangeCallback_Midi(uint8_t *msg);

}

class MCLActionsCallbacks : public ClockCallback {
public:
  bool state;

  void onMidiStartCallback();

  void setup_callbacks();
  void remove_callbacks();
};

#endif /* MCLACTIONS_H__ */
