/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef MIDIACTIVEPEERING_H__
#define MIDIACTIVEPEERING_H__

#include "MidiID.h"
#include "Elektron.h"
#include "Task.h"

#define UART1_PORT 1
#define UART2_PORT 2

class MidiActivePeering : public Task {
public:
  MidiActivePeering(uint16_t _interval = 0) : Task(_interval) { setup(_interval); }

  virtual void setup(uint16_t _interval = 0) { interval = _interval; }

  virtual void run();
  virtual void destroy() {};

  MidiDevice* get_device(uint8_t port);
};

extern MidiActivePeering midi_active_peering;

#endif /* MIDIACTIVEPEERING_H__ */
