/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef MIDIACTIVEPEERING_H__
#define MIDIACTIVEPEERING_H__
#include "MCL.h"

#define UART1_PORT 1
#define UART2_PORT 2

#define DEVICE_NULL 0
#define DEVICE_MIDI 0x0F
#define DEVICE_MD 0x02
#define DEVICE_A4 0x06

class MidiActivePeering {
  public:
  uint8_t uart1_device = DEVICE_NULL;
  uint8_t uart2_device = DEVICE_NULL;
  void md_setup();
  void a4_setup();
  void check();
  uint8_t get_device(uint8_t port);
};

extern MidiActivePeering midi_active_peering;

#endif /* MIDIACTIVEPEERING_H__ */
