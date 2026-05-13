#pragma once

#include <inttypes.h>

class MidiDevice;

enum class DeviceIdx : uint8_t {
  Primary = 0,
  Secondary = 1,
  None = 255,
};

class DeviceContext {
public:
  DeviceContext() = default;

  static DeviceContext primary(MidiDevice *d) {
    return DeviceContext(d, DeviceIdx::Primary);
  }
  static DeviceContext secondary(MidiDevice *d) {
    return DeviceContext(d, DeviceIdx::Secondary);
  }
  static DeviceContext for_device(MidiDevice *d, DeviceIdx device_idx) {
    return DeviceContext(d, device_idx);
  }

  MidiDevice *device() const { return device_; }
  DeviceIdx device_idx() const { return device_idx_; }
  bool valid() const { return device_ != nullptr; }

private:
  DeviceContext(MidiDevice *d, DeviceIdx idx)
      : device_(d), device_idx_(idx) {}

  MidiDevice *device_ = nullptr;
  DeviceIdx device_idx_ = DeviceIdx::None;
};
