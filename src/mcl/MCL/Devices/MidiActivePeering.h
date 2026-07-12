/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef MIDIACTIVEPEERING_H__
#define MIDIACTIVEPEERING_H__

#include "Task.h"

class MidiDevice;

class MidiActivePeering : public Task {
public:
  MidiActivePeering(uint16_t _interval = 250) : Task(_interval) {
    setup(_interval);
  }
  void setup(uint16_t _interval = 250) { interval = _interval; }
  void disconnect(uint8_t port);
  void force_connect(uint8_t port, MidiDevice *driver);
  virtual void run();
};

extern MidiActivePeering midi_active_peering;

#endif /* MIDIACTIVEPEERING_H__ */
