#pragma once

#include "platform.h"
#include "../Drivers/DeviceContext.h"
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
  uint8_t device_slot = 0;
  uint8_t target = 0;

  bool valid() const { return device != nullptr; }
  uint8_t device_index() const { return context().grid_idx(); }
  DeviceContext context() const { return DeviceContext(device, device_slot); }
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
};

struct DevicePerfTarget {
  DeviceParamTarget params;

  bool valid() const { return params.valid(); }
  uint8_t device_index() const { return params.device_index(); }

  uint8_t param_count() const { return params.param_count(); }
  bool target_label(char *out, uint8_t len) const {
    return params.target_label(out, len);
  }
  bool param_label(uint8_t param, char *out, uint8_t len) const {
    return params.param_label(param, out, len);
  }
  bool get_param(uint8_t param, uint8_t *value) const {
    return params.get_param(param, value);
  }
  bool set_param(uint8_t param, uint8_t value,
                 MidiUartClass *uart_ = nullptr) const {
    return params.set_param(param, value, uart_);
  }

  bool param_from_key(uint8_t key, uint8_t *param) const;
  bool key_for_param(uint8_t param, uint8_t *key) const;
  bool begin_param_editor(uint8_t *params, uint8_t count) const;
};

namespace DeviceParamResolver {

MidiDevice *slot_device(uint8_t device_slot);
uint8_t slot_target_count(uint8_t device_slot);
DeviceParamTarget slot(uint8_t device_slot, uint8_t dest);

uint8_t perf_target_count();
DevicePerfTarget perf(uint8_t dest);
uint8_t perf_dest_from_slot(uint8_t device_slot, uint8_t slot_dest);
void end_perf_param_editor();
void set_perf_rec_mode(uint8_t mode);
bool perf_scene_autofill(PerfData *data, uint8_t scene);

} // namespace DeviceParamResolver
