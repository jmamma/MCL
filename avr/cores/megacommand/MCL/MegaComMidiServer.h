#pragma once

#include "MegaComTask.h"

class MCMidiServer : public MegaComServer {
public:
  MCMidiServer(): MegaComServer(true){}
  // send to virtual Midi port 0 or 1
  void send(uint8_t port, uint8_t data);
  void send_isr(uint8_t port, uint8_t data);
  // receive a byte from virtual Midi
  virtual int run();
  // not used
  virtual int resume(int){}
};

extern MCMidiServer megacom_midiserver;
