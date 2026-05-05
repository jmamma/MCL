/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef MIDIACTIVEPEERING_H__
#define MIDIACTIVEPEERING_H__

#include "Elektron.h"
#include "MidiID.h"
#include "Task.h"
#include "MCLSysConfig.h"
#include "MCLSeq.h"
#include "MidiSetup.h"
#include "../Drivers/MidiDevice.h"
#include "../Drivers/Generic/GenericMidiDevice.h"

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

  /** Compatibility aliases for logical driver slots, USB-resolved. */
  MidiDevice *dev1;
  MidiDevice *dev2;

  /** Refresh compatibility aliases from DeviceManager. */
  void update_dev_cache();
};

extern MidiActivePeering midi_active_peering;

#endif /* MIDIACTIVEPEERING_H__ */
