#include "DeviceParamTargets.h"

#if defined(__AVR__)
#include "../Drivers/MD/MD.h"
#include "DeviceManager.h"
#include "MCLSeq.h"
#include <string.h>
#else
#include "../Drivers/MidiDevice.h"
#include "DeviceManager.h"
#endif

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

  target -= primary_count;
  MidiDevice *secondary = device_manager.secondary_device();
  uint8_t secondary_count = secondary->param_target_count(1);
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
    uint8_t channel = target - (NUM_MD_TRACKS + 4);
    if (channel >= NUM_EXT_TRACKS) {
      return false;
    }
    uart2_->sendCC(channel, param, value);
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
  return count + secondary->param_target_count(1);
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
#endif

} // namespace DeviceParamTargets
