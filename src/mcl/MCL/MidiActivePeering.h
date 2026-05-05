/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef MIDIACTIVEPEERING_H__
#define MIDIACTIVEPEERING_H__

#include "Elektron.h"
#include "MidiID.h"
#include "Task.h"
#include "MCLSysConfig.h"
#include "MCLSeq.h"
#include "../Drivers/Generic/GenericMidiDevice.h"

#define UART1_PORT 1
#define UART2_PORT 2
#define UARTUSB_PORT 3

class MidiActivePeering : public Task {
public:
  MidiActivePeering(uint16_t _interval = 250) : Task(_interval) {
    setup(_interval);
  }
  virtual void setup(uint16_t _interval = 250) { interval = _interval; }
  virtual void disconnect(uint8_t port);
  virtual void force_connect(uint8_t port, MidiDevice *driver);
  virtual void run();
  virtual void destroy(){};

  /**
   * Gets the device connected to the physical port.
   * Always return a non-null pointer (could be a NullMidiDevice*).
   **/
  MidiDevice *get_device(uint8_t port);

  /** Cached device pointers for logical driver slots, USB-resolved. */
  MidiDevice *dev1;
  MidiDevice *dev2;

  /** Refresh dev1/dev2 from connected_midi_devices + USB routing. */
  void update_dev_cache();
};

class NullMidiDevice : public MidiDevice {
public:
  NullMidiDevice();
  virtual bool probe() { return false; }
};

extern NullMidiDevice null_midi_device;
extern MidiActivePeering midi_active_peering;

#endif /* MIDIACTIVEPEERING_H__ */
