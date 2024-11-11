/* Copyright 2018, Justin Mammarella jmamma@gmail.com */

#ifndef MIDIACTIVEPEERING_H__
#define MIDIACTIVEPEERING_H__

#include "Elektron.h"
#include "MidiID.h"
#include "Task.h"
#include "MCLSysConfig.h"
#include "MCLSeq.h"

#define UART1_PORT 1
#define UART2_PORT 2

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
   * Gets the device connected to the port.
   * Always return a non-null pointer (could be a NullMidiDevice*).
   **/
  MidiDevice *get_device(uint8_t port);
};

class GenericMidiDevice : public MidiDevice {
public:
  GenericMidiDevice();

  virtual uint8_t* icon();

  virtual bool probe();

  void init_grid_devices(uint8_t device_idx);
  virtual uint8_t get_mute_cc();
  virtual void muteTrack(uint8_t track, bool mute = true, MidiUartClass *uart_ = nullptr);
  virtual void setLevel(uint8_t track, uint8_t value, MidiUartClass *uart_ = nullptr);
};

class NullMidiDevice : public MidiDevice {
public:
  NullMidiDevice();
  virtual bool probe() { return false; }
};

extern GenericMidiDevice generic_midi_device;
extern NullMidiDevice null_midi_device;
extern MidiActivePeering midi_active_peering;

#endif /* MIDIACTIVEPEERING_H__ */
