#include "MCLFeatureConfig.h"

#ifdef MCL_HAS_DEVICE_CAPABILITIES

#include "DeviceParamResolver.h"

#include "../Drivers/MidiDevice.h"
#include "DeviceManager.h"
#include "MCLSeq.h"
#include "PerfData.h"

namespace {

DeviceParamTarget resolve_target(DeviceIdx device_idx, uint8_t dest) {
  DeviceParamTarget target;
  if (dest == 0 || device_idx == DeviceIdx::None) {
    return target;
  }
  DeviceContext ctx = device_manager.context_for_device(device_idx);
  target.device = ctx.device();
  target.device_idx = device_idx;
  target.target = dest - 1;
  DeviceParamCapability *params =
      target.device != nullptr ? target.device->params() : nullptr;
  if (params == nullptr ||
      target.target >= params->target_count(target.context())) {
    target.device = nullptr;
  }
  return target;
}

} // namespace

uint8_t DeviceParamTarget::param_count() const {
  if (!valid()) {
    return 0;
  }
  return device->params()->param_count(context(), target);
}

bool DeviceParamTarget::target_label(char *out, uint8_t len) const {
  if (!valid()) {
    return false;
  }
  return device->params()->target_label(context(), target, out, len);
}

bool DeviceParamTarget::param_label(uint8_t param, char *out,
                                    uint8_t len) const {
  if (!valid()) {
    return false;
  }
  return device->params()->param_label(context(), target, param, out, len);
}

bool DeviceParamTarget::get_param(uint8_t param, uint8_t *value) const {
  if (!valid()) {
    return false;
  }
  return device->params()->get_param(context(), target, param, value);
}

bool DeviceParamTarget::set_param(uint8_t param, uint8_t value,
                                  MidiUartClass *uart_) const {
  if (!valid()) {
    return false;
  }
  return device->params()->set_param(context(), target, param, value, uart_);
}

uint8_t DeviceParamTarget::lock_param_count() const {
  if (!valid()) {
    return 0;
  }
  return device->params()->sequencer_lock_param_count(context(), target);
}

bool DeviceParamTarget::lock_param_info(uint8_t param,
                                        MidiDeviceParamInfo *info) const {
  if (!valid()) {
    return false;
  }
  return device->params()->sequencer_lock_param_info(context(), target, param,
                                                     info);
}

bool DeviceParamTarget::lock_param_label(uint8_t param, char *out,
                                         uint8_t len) const {
  if (!valid()) {
    return false;
  }
  return device->params()->sequencer_lock_param_label(context(), target, param,
                                                      out, len);
}

bool DeviceParamTarget::lock_current_value(uint8_t param,
                                           uint8_t *value) const {
  MidiDeviceParamInfo info;
  if (!lock_param_info(param, &info)) {
    return false;
  }
  if (value != nullptr) {
    *value = (uint8_t)info.current_value;
  }
  return true;
}

bool DeviceParamTarget::uses_step_pitch() const {
  if (!valid()) {
    return false;
  }
  return device->params()->sequencer_uses_step_pitch(context(), target);
}

uint8_t DeviceParamTarget::pitch_lock_param() const {
  if (!valid()) {
    return 0;
  }
  return device->params()->sequencer_pitch_lock_param(context(), target);
}

bool DevicePerfTarget::param_from_key(uint8_t key, uint8_t *param) const {
  if (!valid()) {
    return false;
  }
  return params.device->perf()->perf_param_from_key(
      params.context(), params.target, key, param);
}

bool DevicePerfTarget::key_for_param(uint8_t param, uint8_t *key) const {
  if (!valid()) {
    return false;
  }
  return params.device->perf()->perf_key_for_param(
      params.context(), params.target, param, key);
}

bool DevicePerfTarget::begin_param_editor(uint8_t *editor_params,
                                          uint8_t count) const {
  if (!valid()) {
    return false;
  }
  return params.device->perf()->perf_begin_param_editor(
      params.context(), params.target, editor_params, count);
}

namespace DeviceParamResolver {

uint8_t target_count_for_idx(DeviceIdx device_idx) {
  DeviceContext ctx = device_manager.context_for_device(device_idx);
  return ctx.device() != nullptr ? ctx.device()->params()->target_count(ctx)
                                 : 0;
}

DeviceParamTarget target_for_idx(DeviceIdx device_idx, uint8_t dest) {
  return resolve_target(device_idx, dest);
}

uint8_t perf_target_count() {
  DeviceContext primary_ctx = device_manager.primary_context();
  DeviceContext secondary_ctx = device_manager.secondary_context();
  uint8_t count = primary_ctx.device()->params()->target_count(primary_ctx);
  return count + secondary_ctx.device()->params()->target_count(secondary_ctx);
}

DevicePerfTarget perf(uint8_t dest) {
  DevicePerfTarget perf_target;
  DeviceParamTarget &target = perf_target.params;
  if (dest == 0) {
    return perf_target;
  }

  DeviceContext primary_ctx = device_manager.primary_context();
  DeviceParamCapability *primary_params = primary_ctx.device()->params();
  uint8_t primary_count = primary_params->target_count(primary_ctx);
  uint8_t local_target = dest - 1;
  if (local_target < primary_count) {
    target.device = primary_ctx.device();
    target.device_idx = DeviceIdx::Primary;
    target.target = local_target;
    return perf_target;
  }

  local_target -= primary_count;
  DeviceContext secondary_ctx = device_manager.secondary_context();
  DeviceParamCapability *secondary_params = secondary_ctx.device()->params();
  uint8_t secondary_count = secondary_params->target_count(secondary_ctx);
  if (local_target < secondary_count) {
    target.device = secondary_ctx.device();
    target.device_idx = DeviceIdx::Secondary;
    target.target = local_target;
  }
  return perf_target;
}

uint8_t primary_perf_editor_dest(uint8_t track) {
  uint8_t dest = perf_dest_from_idx(DeviceIdx::Primary, track + 1);
  return dest == 255 ? 0 : dest + 1;
}

void end_perf_param_editor() {
  for (uint8_t i = 0; i < NUM_GRIDS; i++) {
    DeviceContext ctx =
        device_manager.context_for_device(static_cast<DeviceIdx>(i));
    ctx.device()->perf()->perf_end_param_editor(ctx);
  }
}

void set_perf_rec_mode(uint8_t mode) {
  for (uint8_t i = 0; i < NUM_GRIDS; i++) {
    DeviceContext ctx =
        device_manager.context_for_device(static_cast<DeviceIdx>(i));
    ctx.device()->perf()->perf_set_rec_mode(ctx, mode);
  }
}

bool perf_scene_autofill(PerfData *data, uint8_t scene) {
  if (data == nullptr || scene >= NUM_SCENES) {
    return false;
  }
  bool filled = false;
  uint8_t offset = 0;
  for (uint8_t i = 0; i < NUM_GRIDS; i++) {
    DeviceIdx device_idx = static_cast<DeviceIdx>(i);
    DeviceContext ctx = device_manager.context_for_device(device_idx);
    filled |=
        ctx.device()->perf()->perf_scene_autofill(ctx, offset, data, scene);
    offset += target_count_for_idx(device_idx);
  }
  return filled;
}

} // namespace DeviceParamResolver

#endif // MCL_HAS_DEVICE_CAPABILITIES
