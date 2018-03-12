/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef MIDIACTIVEPEERING_H__
#define MIDIACTIVEPEERING_H__
#include "MCL.hh"

class MidiActivePeering {
  public:
  void md_setup();
  void a4_setup();
  void check();
};

extern MidiActivePeering midi_active_peering;

#endif /* MIDIACTIVEPEERING_H__ */
