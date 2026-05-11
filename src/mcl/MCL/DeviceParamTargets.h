#pragma once

#include "platform.h"
#include "../Drivers/MidiDeviceParam.h"
#include <inttypes.h>

class MidiDevice;
class MidiUartClass;
class PerfData;

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
uint8_t slot_lock_param_count(uint8_t device_slot, uint8_t dest);
bool slot_lock_param_info(uint8_t device_slot, uint8_t dest, uint8_t param,
                          MidiDeviceParamInfo *info);
bool slot_lock_param_label(uint8_t device_slot, uint8_t dest, uint8_t param,
                           char *out, uint8_t len);
bool slot_lock_current_value(uint8_t device_slot, uint8_t dest, uint8_t param,
                             uint8_t *value);
bool slot_uses_step_pitch(uint8_t device_slot, uint8_t dest);
uint8_t slot_pitch_lock_param(uint8_t device_slot, uint8_t dest);

uint8_t perf_target_count();
uint8_t perf_param_count(uint8_t dest);
bool perf_target_label(uint8_t dest, char *out, uint8_t len);
bool perf_param_label(uint8_t dest, uint8_t param, char *out, uint8_t len);
bool perf_get_param(uint8_t dest, uint8_t param, uint8_t *value);
bool perf_set_param(uint8_t dest, uint8_t param, uint8_t value,
                    MidiUartClass *uart_ = nullptr,
                    MidiUartClass *uart2_ = nullptr);
uint8_t perf_dest_from_slot(uint8_t device_slot, uint8_t slot_dest);
bool perf_param_from_key(uint8_t dest, uint8_t key, uint8_t *param);
bool perf_key_for_param(uint8_t dest, uint8_t param, uint8_t *key);
bool perf_begin_param_editor(uint8_t dest, uint8_t *params, uint8_t count);
void perf_end_param_editor();
void perf_set_rec_mode(uint8_t mode);
bool perf_scene_autofill(PerfData *data, uint8_t scene);

} // namespace DeviceParamTargets
