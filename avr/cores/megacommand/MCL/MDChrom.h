/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef MDCHROM_H__
#define MDCHROM_H__

#include "MCL.h"
#define EXPLOIT_MAX_NOTES 20

class MCLChrom {
public:
  // Two global objects are stored
  // Switching between these globals enabled/disables epxloit
  MDGlobal global_one;
  MDGlobal global_two;

  // Flag to indicate global initalisation
  bool globals_initialized;
  // State to indicate whether exploit is on or off
  bool state;
  // Clock when export started for timeout
  uint16_t start_clock = 0;

  MCLChromCallbacks md_exploit_callbacks;
  MCLChromMidiEvents md_exploit_midievents;
 
  MCLChrom() {}
  void setup();
  void send_globals();
  void switch_global(uint8_t global_page);
  bool on();
  bool off();
};

extern MCLChrom md_exploit;

class MCLChromMidiEvents : public MidiCallBack {
  bool state;

  void setup_callbacks();
  void remove_callbacks();

  uint8_t note_to_trig(uint8_t);
  // Callbacks for intercepting MD triggers as GUI input
  void onNoteOnCallback_Midi(uint8_t *msg);
  void onNoteOffCallback_Midi(uint8_t *msg);
  // Control change callback required to disable exploit if CC received
  // Any updating of MD display causes exploit to fail.
  void onControlChangeCallback_Midi(uint8_t *msg);

}

class MCLChromCallbacks : public ClockCallback {
public:
  bool state;

  void onMidiStartCallback();

  void setup_callbacks();
  void remove_callbacks();
};

#endif /* MDCHROM_H__ */
