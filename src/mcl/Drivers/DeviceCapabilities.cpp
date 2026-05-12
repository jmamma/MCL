#include "DeviceCapabilities.h"

#include "MidiDevice.h"
#include "Project.h"
#include "SeqTrack.h"

DeviceMixerCapability::DeviceMixerCapability(MidiDevice &device)
    : device_(device) {}

uint8_t DeviceMixerCapability::track_count(uint8_t device_idx) const {
  if (device_idx >= NUM_GRIDS) {
    return 0;
  }
  uint8_t count = 0;
  const MidiDeviceGrid &grid = proj.grids[device_idx];
  for (uint8_t n = 0; n < GRID_WIDTH; n++) {
    const GridDeviceTrack &gdt = grid.tracks[n];
    if (gdt.track_type != EMPTY_TRACK_TYPE && gdt.group_type == GROUP_DEV &&
        gdt.device_idx == device_idx && gdt.seq_track != nullptr) {
      count = n + 1;
    }
  }
  return count;
}

SeqTrack *DeviceMixerCapability::seq_track(uint8_t device_idx,
                                           uint8_t track) {
  if (device_idx >= NUM_GRIDS || track >= GRID_WIDTH) {
    return nullptr;
  }
  GridDeviceTrack &gdt = proj.grids[device_idx].tracks[track];
  if (gdt.track_type == EMPTY_TRACK_TYPE || gdt.group_type != GROUP_DEV ||
      gdt.device_idx != device_idx) {
    return nullptr;
  }
  return gdt.seq_track;
}

uint8_t DeviceMixerCapability::default_param(uint8_t device_idx) const {
  (void)device_idx;
  return 0;
}

bool DeviceMixerCapability::param(uint8_t device_idx, uint8_t track,
                                  uint8_t param_idx,
                                  MidiDeviceMixerParam *out) {
  (void)device_idx;
  (void)track;
  (void)param_idx;
  (void)out;
  return false;
}

bool DeviceMixerCapability::set_param(uint8_t device_idx, uint8_t track,
                                      uint8_t param_idx, int16_t value,
                                      bool send) {
  (void)device_idx;
  (void)track;
  (void)param_idx;
  (void)value;
  (void)send;
  return false;
}

void DeviceMixerCapability::mute_track(uint8_t device_idx, uint8_t track,
                                       bool mute, MidiUartClass *uart_) {
  (void)device_idx;
  device_.muteTrack(track, mute, uart_);
}

void DeviceMixerCapability::set_record_mutes(uint8_t device_idx,
                                             uint8_t track, bool state,
                                             bool clear) {
  (void)clear;
  SeqTrack *track_ptr = seq_track(device_idx, track);
  if (track_ptr != nullptr) {
    track_ptr->record_mutes = state;
  }
}

uint8_t DeviceMixerCapability::trig_group(uint8_t device_idx,
                                          uint8_t track) const {
  (void)device_idx;
  (void)track;
  return 255;
}

void DeviceMixerCapability::select_track(uint8_t device_idx, uint8_t track) {
  (void)device_idx;
  (void)track;
}

void DeviceMixerCapability::restore_track_params(uint8_t device_idx,
                                                 uint8_t track) {
  (void)device_idx;
  (void)track;
}

#if !defined(__AVR__)
DeviceParamCapability::DeviceParamCapability(MidiDevice &device)
    : device_(device) {}

uint8_t DeviceParamCapability::target_count(uint8_t device_idx) const {
  return device_.param_target_count(device_idx);
}

uint8_t DeviceParamCapability::param_count(uint8_t device_idx,
                                           uint8_t target) const {
  return device_.param_count(device_idx, target);
}

bool DeviceParamCapability::target_label(uint8_t device_idx, uint8_t target,
                                         char *out, uint8_t len) const {
  return device_.param_target_label(device_idx, target, out, len);
}

bool DeviceParamCapability::param_label(uint8_t device_idx, uint8_t target,
                                        uint8_t param, char *out,
                                        uint8_t len) {
  return device_.param_label(device_idx, target, param, out, len);
}

bool DeviceParamCapability::get_param(uint8_t device_idx, uint8_t target,
                                      uint8_t param, uint8_t *value) {
  return device_.get_param(device_idx, target, param, value);
}

bool DeviceParamCapability::set_param(uint8_t device_idx, uint8_t target,
                                      uint8_t param, uint8_t value,
                                      MidiUartClass *uart_) {
  return device_.set_param(device_idx, target, param, value, uart_);
}

uint8_t DeviceParamCapability::sequencer_lock_param_count(
    uint8_t device_idx, uint8_t target) const {
  return device_.sequencer_lock_param_count(device_idx, target);
}

bool DeviceParamCapability::sequencer_lock_param_info(
    uint8_t device_idx, uint8_t target, uint8_t param,
    MidiDeviceParamInfo *info) {
  return device_.sequencer_lock_param_info(device_idx, target, param, info);
}

bool DeviceParamCapability::sequencer_lock_param_label(
    uint8_t device_idx, uint8_t target, uint8_t param, char *out,
    uint8_t len) {
  return device_.sequencer_lock_param_label(device_idx, target, param, out,
                                            len);
}

bool DeviceParamCapability::sequencer_uses_step_pitch(
    uint8_t device_idx, uint8_t target) const {
  return device_.sequencer_uses_step_pitch(device_idx, target);
}

uint8_t DeviceParamCapability::sequencer_pitch_lock_param(
    uint8_t device_idx, uint8_t target) const {
  return device_.sequencer_pitch_lock_param(device_idx, target);
}

bool DeviceParamCapability::perf_param_from_key(uint8_t device_idx,
                                                uint8_t target, uint8_t key,
                                                uint8_t *param) {
  (void)device_idx;
  (void)target;
  (void)key;
  (void)param;
  return false;
}

bool DeviceParamCapability::perf_key_for_param(uint8_t device_idx,
                                               uint8_t target, uint8_t param,
                                               uint8_t *key) {
  (void)device_idx;
  (void)target;
  (void)param;
  (void)key;
  return false;
}

bool DeviceParamCapability::perf_begin_param_editor(uint8_t device_idx,
                                                    uint8_t target,
                                                    uint8_t *params,
                                                    uint8_t count) {
  (void)device_idx;
  (void)target;
  (void)params;
  (void)count;
  return false;
}

void DeviceParamCapability::perf_end_param_editor(uint8_t device_idx) {
  (void)device_idx;
}

void DeviceParamCapability::perf_set_rec_mode(uint8_t device_idx,
                                              uint8_t mode) {
  (void)device_idx;
  (void)mode;
}

bool DeviceParamCapability::perf_scene_autofill(uint8_t device_idx,
                                                uint8_t dest_offset,
                                                PerfData *data,
                                                uint8_t scene) {
  (void)device_idx;
  (void)dest_offset;
  (void)data;
  (void)scene;
  return false;
}
#endif

void DevicePanelCapability::set_key_repeat(uint8_t enabled) {
  (void)enabled;
}
