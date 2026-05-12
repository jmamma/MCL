#pragma once

#include "platform.h"
#include "../Drivers/MidiDeviceParam.h"
#include <inttypes.h>

class MidiDevice;
class MidiUartClass;
class PerfData;

struct DeviceParamTarget {
#if defined(__AVR__)
  uint8_t device_slot = 0;
  uint8_t target = 0;

  bool valid() const { return device_slot != 0; }
  uint8_t device_index() const { return device_slot == 2 ? 1 : 0; }
#else
  MidiDevice *device = nullptr;
  uint8_t device_idx = 0;
  uint8_t device_slot = 0;
  uint8_t target = 0;

  bool valid() const { return device != nullptr; }
  uint8_t device_index() const { return device_idx; }
#endif

  uint8_t param_count() const;
  bool target_label(char *out, uint8_t len) const;
  bool param_label(uint8_t param, char *out, uint8_t len) const;
  bool get_param(uint8_t param, uint8_t *value) const;
  bool set_param(uint8_t param, uint8_t value,
                 MidiUartClass *uart_ = nullptr) const;

  uint8_t lock_param_count() const;
  bool lock_param_info(uint8_t param, MidiDeviceParamInfo *info) const;
  bool lock_param_label(uint8_t param, char *out, uint8_t len) const;
  bool lock_current_value(uint8_t param, uint8_t *value) const;
  bool uses_step_pitch() const;
  uint8_t pitch_lock_param() const;

  bool perf_param_from_key(uint8_t key, uint8_t *param) const;
  bool perf_key_for_param(uint8_t param, uint8_t *key) const;
  bool begin_perf_param_editor(uint8_t *params, uint8_t count) const;
};

namespace DeviceParamResolver {

MidiDevice *slot_device(uint8_t device_slot);
uint8_t slot_device_idx(uint8_t device_slot);
uint8_t slot_target_count(uint8_t device_slot);
DeviceParamTarget slot(uint8_t device_slot, uint8_t dest);

uint8_t perf_target_count();
DeviceParamTarget perf(uint8_t dest);
uint8_t perf_dest_from_slot(uint8_t device_slot, uint8_t slot_dest);
void end_perf_param_editor();
void set_perf_rec_mode(uint8_t mode);
bool perf_scene_autofill(PerfData *data, uint8_t scene);

} // namespace DeviceParamResolver
