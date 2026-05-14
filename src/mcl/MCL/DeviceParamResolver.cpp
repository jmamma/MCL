#include "DeviceParamResolver.h"

#include "DeviceManager.h"

/*
 * Platform-shared resolver entry points. Per-platform implementations
 * of every DeviceParamTarget / DevicePerfTarget method live in
 * DeviceParamResolver_avr.cpp and DeviceParamResolver_generic.cpp;
 * exactly one is compiled per build.
 */

namespace DeviceParamResolver {

MidiDevice *device_for_idx(DeviceIdx device_idx) {
  return device_manager.device_for_idx(device_idx);
}

uint8_t perf_dest_from_idx(DeviceIdx device_idx, uint8_t local_dest) {
  if (local_dest == 0 || local_dest > target_count_for_idx(device_idx)) {
    return 255;
  }
  uint8_t offset = device_idx == DeviceIdx::Secondary
                       ? target_count_for_idx(DeviceIdx::Primary)
                       : 0;
  return offset + local_dest - 1;
}

} // namespace DeviceParamResolver
