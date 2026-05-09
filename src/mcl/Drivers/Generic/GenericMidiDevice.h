#pragma once

#include "../MidiDevice.h"

class GenericMidiDevice : public MidiDevice {
public:
  GenericMidiDevice();

  virtual bool probe() override;

  void init_grid_devices(uint8_t device_idx) override;
  virtual uint8_t get_mute_cc() override;
  virtual void muteTrack(uint8_t track, bool mute = true,
                         MidiUartClass *uart_ = nullptr) override;
  virtual bool mixer_param(uint8_t device_idx, uint8_t track,
                           uint8_t param_idx,
                           MidiDeviceMixerParam *param) override;
  virtual bool set_mixer_param(uint8_t device_idx, uint8_t track,
                               uint8_t param_idx, int16_t value,
                               bool send = true) override;
  virtual void mixer_set_record_mutes(uint8_t device_idx, uint8_t track,
                                      bool state,
                                      bool clear = false) override;
  virtual void setLevel(uint8_t track, uint8_t value,
                        MidiUartClass *uart_ = nullptr);

private:
  uint8_t mixer_levels[16];
};

extern GenericMidiDevice generic_midi_device;
