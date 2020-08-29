/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef MIDIACTIVEPEERING_H__
#define MIDIACTIVEPEERING_H__

#include "MidiID.h"
#include "Task.h"

#define UART1_PORT 1
#define UART2_PORT 2

struct midi_peer_driver_t {
  // mandatory
  uint8_t id;
  // mandatory
  const char* name;
  // mandatory
  bool (*probe)(uint8_t port);
  // optional. can be NULL
  void (*disconnect)();
  // optional. can be NULL
  const uint8_t* icon;
};

class MidiActivePeering : public Task {
public:
  MidiActivePeering(uint16_t _interval = 0) : Task(_interval) { setup(_interval); }

  virtual void setup(uint16_t _interval = 0) { interval = _interval; }

  virtual void run();
  virtual void destroy() {};

  uint8_t get_device(uint8_t port);
};

extern MidiActivePeering midi_active_peering;

#endif /* MIDIACTIVEPEERING_H__ */
