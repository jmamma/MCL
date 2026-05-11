#pragma once

#include "platform.h"
#include <inttypes.h>

class MidiDevice;
class MidiUartClass;

struct DeviceParamTargetRef {
  MidiDevice *device = nullptr;
  uint8_t device_idx = 0;
  uint8_t target = 0;

  bool valid() const { return device != nullptr; }
};

namespace DeviceParamTargets {

MidiDevice *slot_device(uint8_t device_slot);
uint8_t slot_device_idx(uint8_t device_slot);
uint8_t slot_target_count(uint8_t device_slot);
uint8_t slot_param_count(uint8_t device_slot, uint8_t dest);
bool slot_target_label(uint8_t device_slot, uint8_t dest, char *out,
                       uint8_t len);
bool slot_param_label(uint8_t device_slot, uint8_t dest, uint8_t param,
                      char *out, uint8_t len);
bool slot_get_param(uint8_t device_slot, uint8_t dest, uint8_t param,
                    uint8_t *value);
bool slot_set_param(uint8_t device_slot, uint8_t dest, uint8_t param,
                    uint8_t value, MidiUartClass *uart_ = nullptr);

uint8_t perf_target_count();
uint8_t perf_param_count(uint8_t dest);
bool perf_target_label(uint8_t dest, char *out, uint8_t len);
bool perf_param_label(uint8_t dest, uint8_t param, char *out, uint8_t len);
bool perf_get_param(uint8_t dest, uint8_t param, uint8_t *value);
bool perf_set_param(uint8_t dest, uint8_t param, uint8_t value,
                    MidiUartClass *uart_ = nullptr,
                    MidiUartClass *uart2_ = nullptr);

} // namespace DeviceParamTargets
