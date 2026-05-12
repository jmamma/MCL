#include "DeviceParamTargets.h"

#include "../Drivers/MD/MD.h"
#include "../Drivers/MidiDevice.h"
#include "DeviceManager.h"
#include "MCLSeq.h"
#include "PerfData.h"
#include <string.h>

namespace {

#if defined(__AVR__)
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
  if (target < NUM_MD_TRACKS) {
    return false;
  }
  switch (target - NUM_MD_TRACKS) {
  case 0:
    return copy_label("ECH", out, len);
  case 1:
    return copy_label("REV", out, len);
  case 2:
    return copy_label("EQ", out, len);
  case 3:
    return copy_label("DYN", out, len);
  default:
    return false;
  }
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
#else
DeviceParamTargetRef resolve_slot(uint8_t device_slot, uint8_t dest) {
  DeviceParamTargetRef ref;
  if (dest == 0) {
    return ref;
  }
  ref.device = DeviceParamTargets::slot_device(device_slot);
  ref.device_idx = DeviceParamTargets::slot_device_idx(device_slot);
  ref.target = dest - 1;
  DeviceParamCapability *params = ref.device ? ref.device->params() : nullptr;
  if (params == nullptr || ref.target >= params->target_count(ref.device_idx)) {
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
  DeviceParamCapability *primary_params = primary->params();
  uint8_t primary_count = primary_params->target_count(0);
  uint8_t target = dest - 1;
  if (target < primary_count) {
    ref.device = primary;
    ref.device_idx = 0;
    ref.target = target;
    return ref;
  }

  target -= primary_count;
  MidiDevice *secondary = device_manager.secondary_device();
  DeviceParamCapability *secondary_params = secondary->params();
  uint8_t secondary_count = secondary_params->target_count(1);
  if (target < secondary_count) {
    ref.device = secondary;
    ref.device_idx = 1;
    ref.target = target;
  }
  return ref;
}
#endif

} // namespace

namespace DeviceParamTargets {

#if defined(__AVR__)
MidiDevice *slot_device(uint8_t device_slot) {
  return device_slot == 2 ? device_manager.secondary_device()
                          : device_manager.primary_device();
}

uint8_t slot_device_idx(uint8_t device_slot) {
  return device_slot == 2 ? 1 : 0;
}

uint8_t slot_target_count(uint8_t device_slot) {
  return device_slot == 2 ? NUM_EXT_TRACKS : NUM_MD_TRACKS + 4;
}

uint8_t slot_param_count(uint8_t device_slot, uint8_t dest) {
  if (dest == 0) {
    return 0;
  }
  uint8_t target = dest - 1;
  return device_slot == 2 ? (target < NUM_EXT_TRACKS ? 128 : 0)
                          : md_param_count(target);
}

bool slot_target_label(uint8_t device_slot, uint8_t dest, char *out,
                       uint8_t len) {
  if (device_slot == 2 || dest == 0) {
    return false;
  }
  return md_target_label(dest - 1, out, len);
}

bool slot_param_label(uint8_t device_slot, uint8_t dest, uint8_t param,
                      char *out, uint8_t len) {
  if (device_slot == 2 || dest == 0) {
    return false;
  }
  return md_param_label(dest - 1, param, out, len);
}

bool slot_get_param(uint8_t device_slot, uint8_t dest, uint8_t param,
                    uint8_t *value) {
  if (device_slot == 2 || dest == 0) {
    return false;
  }
  return md_get_param(dest - 1, param, value);
}

bool slot_set_param(uint8_t device_slot, uint8_t dest, uint8_t param,
                    uint8_t value, MidiUartClass *uart_) {
  if (dest == 0) {
    return false;
  }
  uint8_t target = dest - 1;
  if (device_slot == 2) {
    if (target >= NUM_EXT_TRACKS) {
      return false;
    }
    mcl_seq.ext_tracks[target].send_cc(param, value, uart_);
    return true;
  }
  if (param >= md_param_count(target)) {
    return false;
  }
  if (target < NUM_MD_TRACKS) {
    MD.setTrackParam(target, param, value, uart_);
  } else {
    MD.setFXParam(param, value, md_fx_type(target), false, uart_);
  }
  return true;
}

uint8_t slot_lock_param_count(uint8_t device_slot, uint8_t dest) {
  if (device_slot == 2 || dest == 0) {
    return 0;
  }
  uint8_t target = dest - 1;
  return target < NUM_MD_TRACKS ? MD_PARAMS_PER_TRACK : 0;
}

bool slot_lock_param_info(uint8_t device_slot, uint8_t dest, uint8_t param,
                          MidiDeviceParamInfo *info) {
  if (info == nullptr || param >= slot_lock_param_count(device_slot, dest)) {
    return false;
  }
  *info = MidiDeviceParamInfo();
  info->active = true;
  info->sendable = true;
  info->param_id = param;
  info->ctrl = param;
  uint8_t value = 0;
  if (slot_get_param(device_slot, dest, param, &value)) {
    info->default_value = value;
    info->current_value = value;
  }
  return true;
}

bool slot_lock_param_label(uint8_t device_slot, uint8_t dest, uint8_t param,
                           char *out, uint8_t len) {
  if (device_slot == 2 || dest == 0 ||
      param >= slot_lock_param_count(device_slot, dest)) {
    return false;
  }
  uint8_t target = dest - 1;
  return copy_short_label(model_param_name(MD.kit.get_model(target), param),
                          out, len, 3);
}

bool slot_lock_current_value(uint8_t device_slot, uint8_t dest, uint8_t param,
                             uint8_t *value) {
  return slot_get_param(device_slot, dest, param, value);
}

bool slot_uses_step_pitch(uint8_t device_slot, uint8_t dest) {
  return device_slot != 2 && dest > 0 && dest <= NUM_MD_TRACKS;
}

uint8_t slot_pitch_lock_param(uint8_t device_slot, uint8_t dest) {
  (void)device_slot;
  (void)dest;
  return 0;
}

uint8_t perf_target_count() { return NUM_MD_TRACKS + 4 + NUM_EXT_TRACKS; }

uint8_t perf_param_count(uint8_t dest) {
  if (dest == 0) {
    return 0;
  }
  uint8_t target = dest - 1;
  return target >= NUM_MD_TRACKS + 4 ? 128 : md_param_count(target);
}

bool perf_target_label(uint8_t dest, char *out, uint8_t len) {
  if (dest == 0) {
    return false;
  }
  return md_target_label(dest - 1, out, len);
}

bool perf_param_label(uint8_t dest, uint8_t param, char *out, uint8_t len) {
  if (dest == 0) {
    return false;
  }
  uint8_t target = dest - 1;
  if (target >= NUM_MD_TRACKS + 4) {
    return false;
  }
  return md_param_label(target, param, out, len);
}

bool perf_get_param(uint8_t dest, uint8_t param, uint8_t *value) {
  if (dest == 0) {
    return false;
  }
  uint8_t target = dest - 1;
  if (target >= NUM_MD_TRACKS + 4) {
    return false;
  }
  return md_get_param(target, param, value);
}

bool perf_set_param(uint8_t dest, uint8_t param, uint8_t value,
                    MidiUartClass *uart_, MidiUartClass *uart2_) {
  if (dest == 0) {
    return false;
  }
  uint8_t target = dest - 1;
  if (target >= NUM_MD_TRACKS + 4) {
    uint8_t ext_track = target - (NUM_MD_TRACKS + 4);
    if (ext_track >= NUM_EXT_TRACKS) {
      return false;
    }
    mcl_seq.ext_tracks[ext_track].send_cc(param, value, uart2_);
  } else if (target >= NUM_MD_TRACKS) {
    MD.setFXParam(param, value, md_fx_type(target), false, uart_);
  } else {
    MD.setTrackParam(target, param, value, uart_, false);
  }
  return true;
}
#else
MidiDevice *slot_device(uint8_t device_slot) {
  return device_slot == 2 ? device_manager.secondary_device()
                          : device_manager.primary_device();
}

uint8_t slot_device_idx(uint8_t device_slot) {
  return device_slot == 2 ? 1 : 0;
}

uint8_t slot_target_count(uint8_t device_slot) {
  MidiDevice *device = slot_device(device_slot);
  return device->params()->target_count(slot_device_idx(device_slot));
}

uint8_t slot_param_count(uint8_t device_slot, uint8_t dest) {
  DeviceParamTargetRef ref = resolve_slot(device_slot, dest);
  if (!ref.valid()) {
    return 0;
  }
  return ref.device->params()->param_count(ref.device_idx, ref.target);
}

bool slot_target_label(uint8_t device_slot, uint8_t dest, char *out,
                       uint8_t len) {
  DeviceParamTargetRef ref = resolve_slot(device_slot, dest);
  return ref.valid() &&
         ref.device->params()->target_label(ref.device_idx, ref.target, out,
                                            len);
}

bool slot_param_label(uint8_t device_slot, uint8_t dest, uint8_t param,
                      char *out, uint8_t len) {
  DeviceParamTargetRef ref = resolve_slot(device_slot, dest);
  return ref.valid() &&
         ref.device->params()->param_label(ref.device_idx, ref.target, param,
                                           out, len);
}

bool slot_get_param(uint8_t device_slot, uint8_t dest, uint8_t param,
                    uint8_t *value) {
  DeviceParamTargetRef ref = resolve_slot(device_slot, dest);
  return ref.valid() &&
         ref.device->params()->get_param(ref.device_idx, ref.target, param,
                                         value);
}

bool slot_set_param(uint8_t device_slot, uint8_t dest, uint8_t param,
                    uint8_t value, MidiUartClass *uart_) {
  DeviceParamTargetRef ref = resolve_slot(device_slot, dest);
  return ref.valid() &&
         ref.device->params()->set_param(ref.device_idx, ref.target, param,
                                         value, uart_);
}

uint8_t slot_lock_param_count(uint8_t device_slot, uint8_t dest) {
  DeviceParamTargetRef ref = resolve_slot(device_slot, dest);
  if (!ref.valid()) {
    return 0;
  }
  return ref.device->params()->sequencer_lock_param_count(ref.device_idx,
                                                          ref.target);
}

bool slot_lock_param_info(uint8_t device_slot, uint8_t dest, uint8_t param,
                          MidiDeviceParamInfo *info) {
  DeviceParamTargetRef ref = resolve_slot(device_slot, dest);
  return ref.valid() &&
         ref.device->params()->sequencer_lock_param_info(
             ref.device_idx, ref.target, param, info);
}

bool slot_lock_param_label(uint8_t device_slot, uint8_t dest, uint8_t param,
                           char *out, uint8_t len) {
  DeviceParamTargetRef ref = resolve_slot(device_slot, dest);
  return ref.valid() &&
         ref.device->params()->sequencer_lock_param_label(
             ref.device_idx, ref.target, param, out, len);
}

bool slot_lock_current_value(uint8_t device_slot, uint8_t dest, uint8_t param,
                             uint8_t *value) {
  MidiDeviceParamInfo info;
  if (!slot_lock_param_info(device_slot, dest, param, &info)) {
    return false;
  }
  if (value != nullptr) {
    *value = (uint8_t)info.current_value;
  }
  return true;
}

bool slot_uses_step_pitch(uint8_t device_slot, uint8_t dest) {
  DeviceParamTargetRef ref = resolve_slot(device_slot, dest);
  return ref.valid() &&
         ref.device->params()->sequencer_uses_step_pitch(ref.device_idx,
                                                         ref.target);
}

uint8_t slot_pitch_lock_param(uint8_t device_slot, uint8_t dest) {
  DeviceParamTargetRef ref = resolve_slot(device_slot, dest);
  if (!ref.valid()) {
    return 0;
  }
  return ref.device->params()->sequencer_pitch_lock_param(ref.device_idx,
                                                          ref.target);
}

uint8_t perf_target_count() {
  MidiDevice *primary = device_manager.primary_device();
  uint8_t count = primary->params()->target_count(0);
  MidiDevice *secondary = device_manager.secondary_device();
  return count + secondary->params()->target_count(1);
}

uint8_t perf_param_count(uint8_t dest) {
  DeviceParamTargetRef ref = resolve_perf(dest);
  if (!ref.valid()) {
    return 0;
  }
  return ref.device->params()->param_count(ref.device_idx, ref.target);
}

bool perf_target_label(uint8_t dest, char *out, uint8_t len) {
  DeviceParamTargetRef ref = resolve_perf(dest);
  return ref.valid() &&
         ref.device->params()->target_label(ref.device_idx, ref.target, out,
                                            len);
}

bool perf_param_label(uint8_t dest, uint8_t param, char *out, uint8_t len) {
  DeviceParamTargetRef ref = resolve_perf(dest);
  return ref.valid() &&
         ref.device->params()->param_label(ref.device_idx, ref.target, param,
                                           out, len);
}

bool perf_get_param(uint8_t dest, uint8_t param, uint8_t *value) {
  DeviceParamTargetRef ref = resolve_perf(dest);
  return ref.valid() &&
         ref.device->params()->get_param(ref.device_idx, ref.target, param,
                                         value);
}

bool perf_set_param(uint8_t dest, uint8_t param, uint8_t value,
                    MidiUartClass *uart_, MidiUartClass *uart2_) {
  DeviceParamTargetRef ref = resolve_perf(dest);
  if (!ref.valid()) {
    return false;
  }
  MidiUartClass *uart = ref.device_idx == 1 ? uart2_ : uart_;
  return ref.device->params()->set_param(ref.device_idx, ref.target, param,
                                         value, uart);
}
#endif

uint8_t perf_dest_from_slot(uint8_t device_slot, uint8_t slot_dest) {
  if (slot_dest == 0 || slot_dest > slot_target_count(device_slot)) {
    return 255;
  }
  uint8_t offset = device_slot == 2 ? slot_target_count(1) : 0;
  return offset + slot_dest - 1;
}

#if defined(__AVR__)
static bool perf_is_primary_md_track_dest(uint8_t dest) {
  return dest > 0 && dest <= NUM_MD_TRACKS;
}
#endif

bool perf_param_from_key(uint8_t dest, uint8_t key, uint8_t *param) {
#if defined(__AVR__)
  if (param == nullptr || !perf_is_primary_md_track_dest(dest) || key < 0x10 ||
      key > 0x17) {
    return false;
  }
  uint8_t value = MD.currentSynthPage * 8 + key - 0x10;
  if (value >= perf_param_count(dest)) {
    return false;
  }
  *param = value;
  return true;
#else
  DeviceParamTargetRef ref = resolve_perf(dest);
  return ref.valid() &&
         ref.device->params()->perf_param_from_key(ref.device_idx, ref.target,
                                                   key, param);
#endif
}

bool perf_key_for_param(uint8_t dest, uint8_t param, uint8_t *key) {
#if defined(__AVR__)
  if (key == nullptr || !perf_is_primary_md_track_dest(dest) ||
      param >= perf_param_count(dest)) {
    return false;
  }
  int16_t value = (int16_t)param - (int16_t)MD.currentSynthPage * 8 + 0x10;
  if (value < 0x10 || value > 0x17) {
    return false;
  }
  *key = (uint8_t)value;
  return true;
#else
  DeviceParamTargetRef ref = resolve_perf(dest);
  return ref.valid() &&
         ref.device->params()->perf_key_for_param(ref.device_idx, ref.target,
                                                  param, key);
#endif
}

bool perf_begin_param_editor(uint8_t dest, uint8_t *params, uint8_t count) {
#if defined(__AVR__)
  if (!perf_is_primary_md_track_dest(dest) || params == nullptr ||
      count < MD_PARAMS_PER_TRACK) {
    return false;
  }
  MD.activate_encoder_interface(params);
  return true;
#else
  DeviceParamTargetRef ref = resolve_perf(dest);
  return ref.valid() &&
         ref.device->params()->perf_begin_param_editor(
             ref.device_idx, ref.target, params, count);
#endif
}

void perf_end_param_editor() {
#if defined(__AVR__)
  MD.deactivate_encoder_interface();
#else
  slot_device(1)->params()->perf_end_param_editor(slot_device_idx(1));
  slot_device(2)->params()->perf_end_param_editor(slot_device_idx(2));
#endif
}

void perf_set_rec_mode(uint8_t mode) {
#if defined(__AVR__)
  MD.set_rec_mode(mode);
#else
  slot_device(1)->params()->perf_set_rec_mode(slot_device_idx(1), mode);
  slot_device(2)->params()->perf_set_rec_mode(slot_device_idx(2), mode);
#endif
}

bool perf_scene_autofill(PerfData *data, uint8_t scene) {
  if (data == nullptr || scene >= NUM_SCENES) {
    return false;
  }

#if defined(__AVR__)
  bool filled = false;
  uint8_t num_params =
      mcl_seq.using_spsx_tracks ? SPS_PARAMS_PER_TRACK : MD_PARAMS_PER_TRACK;
  for (uint8_t track = 0; track < NUM_MD_TRACKS; track++) {
    uint8_t dest = perf_dest_from_slot(1, track + 1);
    if (dest == 255) {
      continue;
    }
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
    uint8_t dest = perf_dest_from_slot(1, NUM_MD_TRACKS + fx + 1);
    if (dest == 255) {
      continue;
    }
    uint8_t value = fxs[n];
    if (data->add_param(dest, param, scene, value) == 255) {
      continue;
    }
    MD.setFXParam(param, fxs_orig[n], fx + MD_FX_ECHO, true);
    MD.setFXParam(param, value, fx + MD_FX_ECHO, false);
    filled = true;
  }
  return filled;
#else
  bool filled = false;
  uint8_t offset = 0;
  for (uint8_t slot = 1; slot <= 2; slot++) {
    MidiDevice *device = slot_device(slot);
    filled |= device->params()->perf_scene_autofill(slot_device_idx(slot),
                                                    offset, data, scene);
    offset += slot_target_count(slot);
  }
  return filled;
#endif
}

} // namespace DeviceParamTargets
