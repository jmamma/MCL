#include "DeviceCapabilities.h"

#include "MidiDevice.h"
#include "Project.h"
#include "SeqTrack.h"

DeviceMixerCapability::DeviceMixerCapability(MidiDevice &device)
    : DeviceCapability(device) {}

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
namespace {

void copy_param_number_label(char prefix, uint8_t number, char *out,
                             uint8_t len) {
  if (out == nullptr || len == 0) {
    return;
  }
  if (len < 2) {
    out[0] = '\0';
    return;
  }

  uint8_t pos = 0;
  out[pos++] = prefix;
  if (number >= 100 && pos + 1 < len) {
    out[pos++] = (char)('0' + number / 100);
    number %= 100;
  }
  if ((number >= 10 || pos > 1) && pos + 1 < len) {
    out[pos++] = (char)('0' + number / 10);
    number %= 10;
  }
  if (pos + 1 < len) {
    out[pos++] = (char)('0' + number);
  }
  out[pos] = '\0';
}

} // namespace

DeviceParamCapability::DeviceParamCapability(MidiDevice &device)
    : DeviceCapability(device) {}

uint8_t DeviceParamCapability::target_count(uint8_t device_idx) const {
  (void)device_idx;
  return 0;
}

uint8_t DeviceParamCapability::param_count(uint8_t device_idx,
                                           uint8_t target) const {
  (void)device_idx;
  (void)target;
  return 0;
}

bool DeviceParamCapability::target_label(uint8_t device_idx, uint8_t target,
                                         char *out, uint8_t len) const {
  (void)device_idx;
  (void)target;
  (void)out;
  (void)len;
  return false;
}

bool DeviceParamCapability::param_label(uint8_t device_idx, uint8_t target,
                                        uint8_t param, char *out,
                                        uint8_t len) {
  (void)device_idx;
  (void)target;
  (void)param;
  (void)out;
  (void)len;
  return false;
}

bool DeviceParamCapability::get_param(uint8_t device_idx, uint8_t target,
                                      uint8_t param, uint8_t *value) {
  (void)device_idx;
  (void)target;
  (void)param;
  (void)value;
  return false;
}

bool DeviceParamCapability::set_param(uint8_t device_idx, uint8_t target,
                                      uint8_t param, uint8_t value,
                                      MidiUartClass *uart_) {
  (void)device_idx;
  (void)target;
  (void)param;
  (void)value;
  (void)uart_;
  return false;
}

uint8_t DeviceParamCapability::sequencer_lock_param_count(
    uint8_t device_idx, uint8_t target) const {
  return param_count(device_idx, target);
}

bool DeviceParamCapability::sequencer_lock_param_info(
    uint8_t device_idx, uint8_t target, uint8_t param,
    MidiDeviceParamInfo *info) {
  if (info == nullptr ||
      param >= sequencer_lock_param_count(device_idx, target)) {
    return false;
  }
  *info = MidiDeviceParamInfo();
  info->active = true;
  info->sendable = true;
  info->param_id = param;
  info->ctrl = param;
  uint8_t value = 0;
  if (get_param(device_idx, target, param, &value)) {
    info->default_value = value;
    info->current_value = value;
  }
  return true;
}

bool DeviceParamCapability::sequencer_lock_param_label(
    uint8_t device_idx, uint8_t target, uint8_t param, char *out,
    uint8_t len) {
  if (out == nullptr || len == 0 ||
      param >= sequencer_lock_param_count(device_idx, target)) {
    return false;
  }
  if (param_label(device_idx, target, param, out, len)) {
    return true;
  }
  copy_param_number_label('P', param, out, len);
  return true;
}

bool DeviceParamCapability::sequencer_uses_step_pitch(
    uint8_t device_idx, uint8_t target) const {
  (void)device_idx;
  (void)target;
  return false;
}

uint8_t DeviceParamCapability::sequencer_pitch_lock_param(
    uint8_t device_idx, uint8_t target) const {
  (void)device_idx;
  (void)target;
  return 0;
}

DevicePerfCapability::DevicePerfCapability(MidiDevice &device)
    : DeviceCapability(device) {}

bool DevicePerfCapability::perf_param_from_key(uint8_t device_idx,
                                               uint8_t target, uint8_t key,
                                               uint8_t *param) {
  (void)device_idx;
  (void)target;
  (void)key;
  (void)param;
  return false;
}

bool DevicePerfCapability::perf_key_for_param(uint8_t device_idx,
                                              uint8_t target, uint8_t param,
                                              uint8_t *key) {
  (void)device_idx;
  (void)target;
  (void)param;
  (void)key;
  return false;
}

bool DevicePerfCapability::perf_begin_param_editor(uint8_t device_idx,
                                                   uint8_t target,
                                                   uint8_t *params,
                                                   uint8_t count) {
  (void)device_idx;
  (void)target;
  (void)params;
  (void)count;
  return false;
}

void DevicePerfCapability::perf_end_param_editor(uint8_t device_idx) {
  (void)device_idx;
}

void DevicePerfCapability::perf_set_rec_mode(uint8_t device_idx,
                                             uint8_t mode) {
  (void)device_idx;
  (void)mode;
}

bool DevicePerfCapability::perf_scene_autofill(uint8_t device_idx,
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

#if !defined(__AVR__)
DeviceStepEditCapability::DeviceStepEditCapability(MidiDevice &device)
    : DeviceCapability(device) {}

bool DeviceStepEditCapability::available(uint8_t device_idx) const {
  (void)device_idx;
  return false;
}

void DeviceStepEditCapability::set_rec_mode(uint8_t device_idx,
                                            uint8_t mode) {
  (void)device_idx;
  (void)mode;
}

void DeviceStepEditCapability::sync_track(uint8_t device_idx, uint8_t length,
                                          uint8_t speed,
                                          uint8_t step_count) {
  (void)device_idx;
  (void)length;
  (void)speed;
  (void)step_count;
}

void DeviceStepEditCapability::set_trig_leds(uint8_t device_idx,
                                             uint16_t mask, uint8_t mode,
                                             uint8_t blink) {
  (void)device_idx;
  (void)mask;
  (void)mode;
  (void)blink;
}

void DeviceStepEditCapability::set_live_param_update(uint8_t device_idx,
                                                     bool enabled) {
  (void)device_idx;
  (void)enabled;
}

bool DeviceStepEditCapability::configure_kit_sound_panel(
    uint8_t device_idx, uint8_t target, char *info, uint8_t info_len,
    uint8_t *pitch_max, bool *is_midi_model) const {
  (void)device_idx;
  (void)target;
  (void)info;
  (void)info_len;
  (void)pitch_max;
  (void)is_midi_model;
  return false;
}

bool DeviceStepEditCapability::kit_sound_uses_note_pitch(
    uint8_t device_idx, uint8_t target) const {
  (void)device_idx;
  (void)target;
  return false;
}

uint8_t DeviceStepEditCapability::kit_sound_default_pitch(
    uint8_t device_idx, uint8_t target) const {
  (void)device_idx;
  (void)target;
  return 0;
}

uint8_t DeviceStepEditCapability::kit_sound_note_from_pitch(
    uint8_t device_idx, uint8_t target, uint8_t pitch) const {
  (void)device_idx;
  (void)target;
  (void)pitch;
  return 255;
}

uint8_t DeviceStepEditCapability::kit_sound_pitch_from_note(
    uint8_t device_idx, uint8_t target, uint8_t note,
    uint8_t fine_tune) const {
  (void)device_idx;
  (void)target;
  (void)note;
  (void)fine_tune;
  return 255;
}

bool DeviceStepEditCapability::param_from_key(uint8_t device_idx,
                                              uint8_t target, uint8_t key,
                                              uint8_t *param) const {
  (void)device_idx;
  (void)target;
  (void)key;
  (void)param;
  return false;
}

bool DeviceStepEditCapability::key_for_param(uint8_t device_idx,
                                             uint8_t target, uint8_t param,
                                             uint8_t *key) const {
  (void)device_idx;
  (void)target;
  (void)param;
  (void)key;
  return false;
}

bool DeviceStepEditCapability::begin_param_editor(uint8_t device_idx,
                                                  uint8_t target,
                                                  uint8_t *params,
                                                  uint8_t count) {
  (void)device_idx;
  (void)target;
  (void)params;
  (void)count;
  return false;
}

void DeviceStepEditCapability::end_param_editor(uint8_t device_idx) {
  (void)device_idx;
}

void DeviceStepEditCapability::close_microtiming(uint8_t device_idx) {
  (void)device_idx;
}

void DeviceStepEditCapability::clear_popup(uint8_t device_idx) {
  (void)device_idx;
}

void DeviceStepEditCapability::popup_text(uint8_t device_idx, char *text,
                                          uint8_t persistent) {
  (void)device_idx;
  (void)text;
  (void)persistent;
}

bool DeviceStepEditCapability::parse_cc(uint8_t device_idx, uint8_t channel,
                                        uint8_t cc, uint8_t *target,
                                        uint8_t *param) const {
  (void)device_idx;
  (void)channel;
  (void)cc;
  (void)target;
  (void)param;
  return false;
}
#endif

void DevicePanelCapability::set_key_repeat(uint8_t enabled) {
  (void)enabled;
}
