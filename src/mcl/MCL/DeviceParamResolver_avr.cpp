#include "MCLFeatureConfig.h"

#if defined(__AVR__)

#include "DeviceParamResolver.h"

#include "../Drivers/MD/MD.h"
#include "MCLSeq.h"
#include "PerfData.h"
#include <string.h>

namespace {

uint8_t md_fx_type(uint8_t target) {
  return MD_FX_ECHO + target - NUM_MD_TRACKS;
}

bool copy_label(const char *label, char *out, uint8_t len) {
  if (label == nullptr || out == nullptr || len == 0) {
    return false;
  }
  strncpy(out, label, len - 1);
  out[len - 1] = '\0';
  return true;
}

bool copy_short_label(const char *label, char *out, uint8_t len,
                      uint8_t max_chars) {
  if (label == nullptr || out == nullptr || len == 0) {
    return false;
  }
  uint8_t pos = 0;
  while (label[pos] != '\0' && pos + 1 < len && pos < max_chars) {
    out[pos] = label[pos];
    pos++;
  }
  out[pos] = '\0';
  if (pos == 2 && pos + 1 < len) {
    out[pos++] = ' ';
    out[pos] = '\0';
  }
  return true;
}

uint8_t md_param_count(uint8_t target) {
  if (target < NUM_MD_TRACKS) {
    return MD_PARAMS_PER_TRACK;
  }
  if (target < NUM_MD_TRACKS + 4) {
    return 8;
  }
  return 0;
}

bool md_target_label(uint8_t target, char *out, uint8_t len) {
  target -= NUM_MD_TRACKS;
  if (target >= 4) {
    return false;
  }
  const char *label = "ECH";
  switch (target) {
  case 0:
    break;
  case 1:
    label = "REV";
    break;
  case 2:
    label = "EQ";
    break;
  case 3:
    label = "DYN";
    break;
  default:
    return false;
  }
  return copy_label(label, out, len);
}

bool md_param_label(uint8_t target, uint8_t param, char *out, uint8_t len) {
  if (param >= md_param_count(target)) {
    return false;
  }
  const char *label = target < NUM_MD_TRACKS
                          ? model_param_name(MD.kit.get_model(target), param)
                          : fx_param_name(md_fx_type(target), param);
  return copy_label(label, out, len);
}

bool md_get_param(uint8_t target, uint8_t param, uint8_t *value) {
  if (value == nullptr || param >= md_param_count(target)) {
    return false;
  }
  *value = target < NUM_MD_TRACKS
               ? MD.kit.params[target][param]
               : MD.kit.get_fx_param(md_fx_type(target), param);
  return true;
}

bool md_set_param(uint8_t target, uint8_t param, uint8_t value,
                  MidiUartClass *uart_, bool base) {
  if (param >= md_param_count(target)) {
    return false;
  }
  if (target < NUM_MD_TRACKS) {
    MD.setTrackParam(target, param, value, uart_, base);
  } else {
    MD.setFXParam(param, value, md_fx_type(target), base, uart_);
  }
  return true;
}

bool ext_set_param(uint8_t target, uint8_t param, uint8_t value,
                   MidiUartClass *uart_) {
  if (target >= NUM_EXT_TRACKS) {
    return false;
  }
  mcl_seq.ext_tracks[target].send_cc(param, value, uart_);
  return true;
}

bool set_resolved_param(DeviceParamTarget target, uint8_t param,
                        uint8_t value, MidiUartClass *uart_, bool base) {
  if (!target.valid()) {
    return false;
  }
  if (target.device_idx == DeviceIdx::Secondary) {
    return ext_set_param(target.target, param, value, uart_);
  }
  return md_set_param(target.target, param, value, uart_, base);
}

} // namespace

uint8_t DeviceParamTarget::param_count() const {
  if (!valid()) {
    return 0;
  }
  return device_idx == DeviceIdx::Secondary ? (target < NUM_EXT_TRACKS ? 128 : 0)
                                            : md_param_count(target);
}

bool DeviceParamTarget::target_label(char *out, uint8_t len) const {
  if (!valid()) {
    return false;
  }
  return device_idx != DeviceIdx::Secondary &&
         md_target_label(target, out, len);
}

bool DeviceParamTarget::param_label(uint8_t param, char *out,
                                    uint8_t len) const {
  if (!valid()) {
    return false;
  }
  return device_idx != DeviceIdx::Secondary &&
         md_param_label(target, param, out, len);
}

bool DeviceParamTarget::get_param(uint8_t param, uint8_t *value) const {
  if (!valid()) {
    return false;
  }
  return device_idx != DeviceIdx::Secondary &&
         md_get_param(target, param, value);
}

bool DeviceParamTarget::set_param(uint8_t param, uint8_t value,
                                  MidiUartClass *uart_) const {
  return send_modulated_param(param, value, uart_);
}

bool DeviceParamTarget::get_base_param(uint8_t param, uint8_t *value) const {
  return get_param(param, value);
}

bool DeviceParamTarget::set_base_param(uint8_t param, uint8_t value,
                                       MidiUartClass *uart_) const {
  return set_resolved_param(*this, param, value, uart_, true);
}

bool DeviceParamTarget::send_modulated_param(uint8_t param, uint8_t value,
                                             MidiUartClass *uart_) const {
  return set_resolved_param(*this, param, value, uart_, false);
}

uint8_t DeviceParamTarget::lock_param_count() const {
  if (!valid()) {
    return 0;
  }
  return device_idx != DeviceIdx::Secondary && target < NUM_MD_TRACKS
             ? MD_PARAMS_PER_TRACK
             : 0;
}

bool DeviceParamTarget::lock_param_info(uint8_t param,
                                        MidiDeviceParamInfo *info) const {
  if (!valid()) {
    return false;
  }
  if (info == nullptr || param >= lock_param_count()) {
    return false;
  }
  *info = MidiDeviceParamInfo();
  info->active = true;
  info->sendable = true;
  info->param_id = param;
  info->ctrl = param;
  uint8_t value = 0;
  if (get_param(param, &value)) {
    info->default_value = value;
    info->current_value = value;
  }
  return true;
}

bool DeviceParamTarget::lock_param_label(uint8_t param, char *out,
                                         uint8_t len) const {
  if (!valid()) {
    return false;
  }
  if (device_idx == DeviceIdx::Secondary || param >= lock_param_count()) {
    return false;
  }
  return copy_short_label(model_param_name(MD.kit.get_model(target), param),
                          out, len, 3);
}

bool DeviceParamTarget::lock_current_value(uint8_t param,
                                           uint8_t *value) const {
  return get_param(param, value);
}

bool DeviceParamTarget::uses_step_pitch() const {
  if (!valid()) {
    return false;
  }
  return device_idx != DeviceIdx::Secondary && target < NUM_MD_TRACKS;
}

uint8_t DeviceParamTarget::pitch_lock_param() const {
  if (!valid()) {
    return 0;
  }
  return 0;
}

bool DevicePerfTarget::param_from_key(uint8_t key, uint8_t *param) const {
  if (!valid()) {
    return false;
  }
  if (param == nullptr || params.device_idx == DeviceIdx::Secondary ||
      params.target >= NUM_MD_TRACKS || key < 0x10 || key > 0x17) {
    return false;
  }
  uint8_t value = MD.currentSynthPage * 8 + key - 0x10;
  if (value >= param_count()) {
    return false;
  }
  *param = value;
  return true;
}

bool DevicePerfTarget::key_for_param(uint8_t param, uint8_t *key) const {
  if (!valid()) {
    return false;
  }
  if (key == nullptr || params.device_idx == DeviceIdx::Secondary ||
      params.target >= NUM_MD_TRACKS || param >= param_count()) {
    return false;
  }
  int8_t value = (int8_t)param - (int8_t)MD.currentSynthPage * 8 + 0x10;
  if (value < 0x10 || value > 0x17) {
    return false;
  }
  *key = (uint8_t)value;
  return true;
}

bool DevicePerfTarget::begin_param_editor(uint8_t *editor_params,
                                          uint8_t count) const {
  if (!valid()) {
    return false;
  }
  if (params.device_idx == DeviceIdx::Secondary ||
      params.target >= NUM_MD_TRACKS || editor_params == nullptr ||
      count < MD_PARAMS_PER_TRACK) {
    return false;
  }
  MD.activate_encoder_interface(editor_params);
  return true;
}

namespace DeviceParamResolver {

uint8_t target_count_for_idx(DeviceIdx device_idx) {
  return device_idx == DeviceIdx::Secondary ? NUM_EXT_TRACKS
                                            : NUM_MD_TRACKS + 4;
}

DeviceParamTarget target_for_idx(DeviceIdx device_idx, uint8_t dest) {
  DeviceParamTarget target;
  if (dest == 0 || device_idx == DeviceIdx::None) {
    return target;
  }
  target.device_idx = device_idx;
  target.target = dest - 1;
  uint8_t max_target = device_idx == DeviceIdx::Secondary
                           ? NUM_EXT_TRACKS
                           : NUM_MD_TRACKS + 4;
  if (target.target >= max_target) {
    target.device_idx = DeviceIdx::None;
  }
  return target;
}

uint8_t perf_target_count() {
  return target_slot_count_for_idx(DeviceIdx::Primary) +
         target_slot_count_for_idx(DeviceIdx::Secondary);
}

DevicePerfTarget perf(uint8_t dest) {
  DevicePerfTarget perf_target;
  DeviceParamTarget &target = perf_target.params;
  DeviceIdx device_idx = DeviceIdx::None;
  uint8_t local_target = 0;
  if (perf_dest_to_target(dest, &device_idx, &local_target)) {
    target.device_idx = device_idx;
    target.target = local_target;
  }
  return perf_target;
}

void end_perf_param_editor() {
  MD.deactivate_encoder_interface();
}

void set_perf_rec_mode(uint8_t mode) {
  MD.set_rec_mode(mode);
}

bool perf_scene_autofill(PerfData *data, uint8_t scene) {
  if (data == nullptr || scene >= NUM_SCENES) {
    return false;
  }
  bool filled = false;
  uint8_t num_params =
      mcl_seq.using_spsx_tracks ? SPS_PARAMS_PER_TRACK : MD_PARAMS_PER_TRACK;
  for (uint8_t track = 0; track < NUM_MD_TRACKS; track++) {
    uint8_t dest = track;
    for (uint8_t param = 0; param < num_params; param++) {
      if (MD.kit.params[track][param] == MD.kit.params_orig[track][param]) {
        continue;
      }
      uint8_t value = MD.kit.params[track][param];
      if (data->add_param(dest, param, scene, value) == 255) {
        continue;
      }
      // Kit encoders go back to normal for save.
      MD.setTrackParam(track, param, MD.kit.params_orig[track][param], nullptr,
                       true);
      MD.setTrackParam(track, param, value, nullptr, false);
      filled = true;
    }
  }

  uint8_t *fxs = (uint8_t *)&MD.kit.reverb;
  uint8_t *fxs_orig = (uint8_t *)&MD.kit.fx_orig;
  for (uint8_t n = 0; n < 8 * 4; n++) {
    uint8_t fx = n / 8;
    uint8_t param = n - fx * 8;
    // Delay and reverb are flipped in memory.
    if (fx == 0) {
      fx = 1;
    } else if (fx == 1) {
      fx = 0;
    }
    if (fxs[n] == fxs_orig[n]) {
      continue;
    }
    uint8_t dest = NUM_MD_TRACKS + fx;
    uint8_t value = fxs[n];
    if (data->add_param(dest, param, scene, value) == 255) {
      continue;
    }
    MD.setFXParam(param, fxs_orig[n], fx + MD_FX_ECHO, true);
    MD.setFXParam(param, value, fx + MD_FX_ECHO, false);
    filled = true;
  }
  return filled;
}

} // namespace DeviceParamResolver

#endif // defined(__AVR__)
