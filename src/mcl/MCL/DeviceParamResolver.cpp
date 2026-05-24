#include "DeviceParamResolver.h"

#include "DeviceManager.h"

/*
 * Platform-shared resolver entry points. Per-platform implementations
 * of every DeviceParamTarget / DevicePerfTarget method live in
 * DeviceParamResolver_avr.cpp and DeviceParamResolver_generic.cpp.
 * Both files are compiled by PlatformIO, but file-scope guards ensure only
 * one contributes symbol definitions in each build.
 */

namespace DeviceParamResolver {

MidiDevice *device_for_idx(DeviceIdx device_idx) {
  return device_manager.device_for_idx(device_idx);
}

uint8_t target_slot_count_for_idx(DeviceIdx device_idx) {
  uint8_t count = target_count_for_idx(device_idx);
  if (device_idx == DeviceIdx::Secondary &&
      count < RESERVED_SECONDARY_TARGETS) {
    return RESERVED_SECONDARY_TARGETS;
  }
  return count;
}

uint8_t perf_data_dest_for_target(DeviceIdx device_idx, uint8_t target) {
  if (device_idx == DeviceIdx::None ||
      target >= target_count_for_idx(device_idx)) {
    return INVALID_PERF_DATA_DEST;
  }
  uint8_t offset = device_idx == DeviceIdx::Secondary
                       ? target_slot_count_for_idx(DeviceIdx::Primary)
                       : 0;
  return offset + target;
}

uint8_t perf_dest_for_target(DeviceIdx device_idx, uint8_t target) {
  uint8_t dest = perf_data_dest_for_target(device_idx, target);
  return dest == INVALID_PERF_DATA_DEST ? 0 : dest + 1;
}

bool perf_dest_to_target(uint8_t perf_dest, DeviceIdx *device_idx,
                         uint8_t *target) {
  if (perf_dest == 0 || device_idx == nullptr || target == nullptr) {
    return false;
  }

  uint8_t data_dest = perf_dest - 1;
  uint8_t primary_count = target_slot_count_for_idx(DeviceIdx::Primary);
  if (data_dest < primary_count) {
    *device_idx = DeviceIdx::Primary;
    *target = data_dest;
    return true;
  }

  data_dest -= primary_count;
  if (data_dest < target_slot_count_for_idx(DeviceIdx::Secondary)) {
    *device_idx = DeviceIdx::Secondary;
    *target = data_dest;
    return true;
  }
  return false;
}

uint8_t primary_perf_editor_dest(uint8_t target) {
  return perf_dest_for_target(DeviceIdx::Primary, target);
}

} // namespace DeviceParamResolver
