/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef MIDIACTIVEPEERING_H__
#define MIDIACTIVEPEERING_H__

#include "MidiID.hh"
#include "Task.hh"

#define UART1_PORT 1
#define UART2_PORT 2

class MidiActivePeering : public Task {
public:
  MidiActivePeering(uint16_t _interval = 0) : Task(_interval) { setup(_interval); }

  virtual void setup(uint16_t _interval = 0) { interval = _interval; }

  virtual void run();
  virtual void destroy() {};
  void prepare_display();
  void delay_progress(uint16_t clock_);

  void md_setup();
  void a4_setup();
  uint8_t get_device(uint8_t port);
};

extern MidiActivePeering midi_active_peering;

#endif /* MIDIACTIVEPEERING_H__ */
