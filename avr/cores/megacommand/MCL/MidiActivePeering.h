/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef MIDIACTIVEPEERING_H__
#define MIDIACTIVEPEERING_H__

#include "MidiID.hh"

#define UART1_PORT 1
#define UART2_PORT 2

class MidiActivePeering {
public:
  void md_setup();
  void a4_setup();
  void check();
  uint8_t get_device(uint8_t port);
};

extern MidiActivePeering midi_active_peering;

#endif /* MIDIACTIVEPEERING_H__ */
