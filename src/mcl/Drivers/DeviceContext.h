#pragma once

#include <inttypes.h>

class MidiDevice;

class DeviceContext {
public:
  DeviceContext() = default;

  static DeviceContext primary(MidiDevice *d) {
    return DeviceContext(d, 0);
  }
  static DeviceContext secondary(MidiDevice *d) {
    return DeviceContext(d, 1);
  }
  static DeviceContext for_device(MidiDevice *d, uint8_t device_idx) {
    return DeviceContext(d, device_idx);
  }

  MidiDevice *device() const { return device_; }
  uint8_t device_idx() const { return device_idx_; }
  bool valid() const { return device_ != nullptr; }

private:
  DeviceContext(MidiDevice *d, uint8_t idx)
      : device_(d), device_idx_(idx) {}

  MidiDevice *device_ = nullptr;
  uint8_t device_idx_ = 0;
};
