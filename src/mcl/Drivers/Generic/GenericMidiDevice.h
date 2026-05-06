#pragma once

#include "../MidiDevice.h"

class GenericMidiDevice : public MidiDevice {
public:
  GenericMidiDevice();

  virtual uint8_t *icon() override;

  virtual bool probe() override;

  void init_grid_devices(uint8_t device_idx) override;
  virtual uint8_t get_mute_cc() override;
  virtual void muteTrack(uint8_t track, bool mute = true,
                         MidiUartClass *uart_ = nullptr) override;
  virtual void mixer_set_record_mutes(uint8_t device_idx, uint8_t track,
                                      bool state,
                                      bool clear = false) override;
  virtual void setLevel(uint8_t track, uint8_t value,
                        MidiUartClass *uart_ = nullptr);
};

extern GenericMidiDevice generic_midi_device;
