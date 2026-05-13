#pragma once

#include <inttypes.h>

class MidiDevice;

class DeviceContext {
public:
  DeviceContext() = default;
  DeviceContext(MidiDevice *device, uint8_t slot)
      : device_(device), slot_(slot) {}

  MidiDevice *device() const { return device_; }
  uint8_t slot() const { return slot_; }
  uint8_t grid_idx() const { return slot_ == 0 ? 0 : slot_ - 1; }
  bool valid() const { return device_ != nullptr; }

private:
  MidiDevice *device_ = nullptr;
  uint8_t slot_ = 0;
};
