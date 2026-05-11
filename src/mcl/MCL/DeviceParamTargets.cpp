#include "DeviceParamTargets.h"

#include "../Drivers/MidiDevice.h"
#include "DeviceManager.h"

namespace {

DeviceParamTargetRef resolve_slot(uint8_t device_slot, uint8_t dest) {
  DeviceParamTargetRef ref;
  if (dest == 0) {
    return ref;
  }
  ref.device = DeviceParamTargets::slot_device(device_slot);
  ref.device_idx = DeviceParamTargets::slot_device_idx(device_slot);
  ref.target = dest - 1;
  if (ref.device == nullptr ||
      ref.target >= ref.device->param_target_count(ref.device_idx)) {
    ref.device = nullptr;
  }
  return ref;
}

DeviceParamTargetRef resolve_perf(uint8_t dest) {
  DeviceParamTargetRef ref;
  if (dest == 0) {
    return ref;
  }

  MidiDevice *primary = device_manager.primary_device();
  uint8_t primary_count = primary->param_target_count(0);
  uint8_t target = dest - 1;
  if (target < primary_count) {
    ref.device = primary;
    ref.device_idx = 0;
    ref.target = target;
    return ref;
  }

  MidiDevice *secondary = device_manager.secondary_device();
  if (secondary == primary) {
    return ref;
  }
  target -= primary_count;
  if (target < secondary->param_target_count(1)) {
    ref.device = secondary;
    ref.device_idx = 1;
    ref.target = target;
  }
  return ref;
}

} // namespace

namespace DeviceParamTargets {

MidiDevice *slot_device(uint8_t device_slot) {
  return device_slot == 2 ? device_manager.secondary_device()
                          : device_manager.primary_device();
}

uint8_t slot_device_idx(uint8_t device_slot) {
  return device_slot == 2 ? 1 : 0;
}

uint8_t slot_target_count(uint8_t device_slot) {
  MidiDevice *device = slot_device(device_slot);
  return device->param_target_count(slot_device_idx(device_slot));
}

uint8_t slot_param_count(uint8_t device_slot, uint8_t dest) {
  DeviceParamTargetRef ref = resolve_slot(device_slot, dest);
  if (!ref.valid()) {
    return 0;
  }
  return ref.device->param_count(ref.device_idx, ref.target);
}

bool slot_target_label(uint8_t device_slot, uint8_t dest, char *out,
                       uint8_t len) {
  DeviceParamTargetRef ref = resolve_slot(device_slot, dest);
  return ref.valid() &&
         ref.device->param_target_label(ref.device_idx, ref.target, out, len);
}

bool slot_param_label(uint8_t device_slot, uint8_t dest, uint8_t param,
                      char *out, uint8_t len) {
  DeviceParamTargetRef ref = resolve_slot(device_slot, dest);
  return ref.valid() &&
         ref.device->param_label(ref.device_idx, ref.target, param, out, len);
}

bool slot_get_param(uint8_t device_slot, uint8_t dest, uint8_t param,
                    uint8_t *value) {
  DeviceParamTargetRef ref = resolve_slot(device_slot, dest);
  return ref.valid() &&
         ref.device->get_param(ref.device_idx, ref.target, param, value);
}

bool slot_set_param(uint8_t device_slot, uint8_t dest, uint8_t param,
                    uint8_t value, MidiUartClass *uart_) {
  DeviceParamTargetRef ref = resolve_slot(device_slot, dest);
  return ref.valid() &&
         ref.device->set_param(ref.device_idx, ref.target, param, value,
                               uart_);
}

uint8_t perf_target_count() {
  MidiDevice *primary = device_manager.primary_device();
  uint8_t count = primary->param_target_count(0);
  MidiDevice *secondary = device_manager.secondary_device();
  if (secondary != primary) {
    count += secondary->param_target_count(1);
  }
  return count;
}

uint8_t perf_param_count(uint8_t dest) {
  DeviceParamTargetRef ref = resolve_perf(dest);
  if (!ref.valid()) {
    return 0;
  }
  return ref.device->param_count(ref.device_idx, ref.target);
}

bool perf_target_label(uint8_t dest, char *out, uint8_t len) {
  DeviceParamTargetRef ref = resolve_perf(dest);
  return ref.valid() &&
         ref.device->param_target_label(ref.device_idx, ref.target, out, len);
}

bool perf_param_label(uint8_t dest, uint8_t param, char *out, uint8_t len) {
  DeviceParamTargetRef ref = resolve_perf(dest);
  return ref.valid() &&
         ref.device->param_label(ref.device_idx, ref.target, param, out, len);
}

bool perf_get_param(uint8_t dest, uint8_t param, uint8_t *value) {
  DeviceParamTargetRef ref = resolve_perf(dest);
  return ref.valid() &&
         ref.device->get_param(ref.device_idx, ref.target, param, value);
}

bool perf_set_param(uint8_t dest, uint8_t param, uint8_t value,
                    MidiUartClass *uart_, MidiUartClass *uart2_) {
  DeviceParamTargetRef ref = resolve_perf(dest);
  if (!ref.valid()) {
    return false;
  }
  MidiUartClass *uart = ref.device_idx == 1 ? uart2_ : uart_;
  return ref.device->set_param(ref.device_idx, ref.target, param, value, uart);
}

} // namespace DeviceParamTargets
