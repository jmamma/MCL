#pragma once

#include "../MidiDevice.h"

class GenericMidiMixerCapability;

class GenericMidiDevice : public MidiDevice {
public:
  GenericMidiDevice();

  virtual bool probe() override;

  void init_grid_devices(uint8_t device_idx) override;
  virtual uint8_t get_mute_cc() override;
  virtual void muteTrack(uint8_t track, bool mute = true,
                         MidiUartClass *uart_ = nullptr) override;
  virtual DeviceMixerCapability *mixer() override;
#if !defined(__AVR__)
  virtual uint8_t param_target_count(uint8_t device_idx) const override;
  virtual uint8_t param_count(uint8_t device_idx, uint8_t target) const override;
  virtual bool set_param(uint8_t device_idx, uint8_t target, uint8_t param,
                         uint8_t value,
                         MidiUartClass *uart_ = nullptr) override;
#endif
  virtual void setLevel(uint8_t track, uint8_t value,
                        MidiUartClass *uart_ = nullptr);

private:
  friend class GenericMidiMixerCapability;
  uint8_t mixer_levels[16];
};

extern GenericMidiDevice generic_midi_device;
