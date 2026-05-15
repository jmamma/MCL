#include "DeviceCapabilities.h"

#include "MidiDevice.h"
#include "MCLSeq.h"
#include "MCLSysConfig.h"
#include "Project.h"
#include "SeqStepTrackRef.h"
#include "SeqTrack.h"
#if !defined(__AVR__)
#include "SeqExtStepTrackApi.h"
#include "SeqPages.h"
#endif

DeviceMixerCapability::DeviceMixerCapability(MidiDevice &device,
                                             uint8_t default_param,
                                             uint8_t mute_param)
    : DeviceCapability(device), default_param_(default_param),
      mute_param_(mute_param) {}

#if !defined(__AVR__)
uint8_t DeviceMixerCapability::track_count(const DeviceContext &ctx) const {
  uint8_t grid_idx = static_cast<uint8_t>(ctx.device_idx());
  if (grid_idx >= NUM_GRIDS) {
    return 0;
  }
  uint8_t count = 0;
  const MidiDeviceGrid &grid = proj.grids[grid_idx];
  for (uint8_t n = 0; n < GRID_WIDTH; n++) {
    const GridDeviceTrack &gdt = grid.tracks[n];
    if (gdt.track_type != EMPTY_TRACK_TYPE && gdt.group_type == GROUP_DEV &&
        gdt.device_idx == grid_idx && gdt.seq_track != nullptr) {
      count = n + 1;
    }
  }
  return count;
}

SeqTrack *DeviceMixerCapability::seq_track(const DeviceContext &ctx,
                                           uint8_t track) {
  uint8_t grid_idx = static_cast<uint8_t>(ctx.device_idx());
  if (grid_idx >= NUM_GRIDS || track >= GRID_WIDTH) {
    return nullptr;
  }
  GridDeviceTrack &gdt = proj.grids[grid_idx].tracks[track];
  if (gdt.track_type == EMPTY_TRACK_TYPE || gdt.group_type != GROUP_DEV ||
      gdt.device_idx != grid_idx) {
    return nullptr;
  }
  return gdt.seq_track;
}
#endif

uint8_t DeviceMixerCapability::default_param() const {
  return default_param_;
}

void DeviceMixerCapability::mute_track(const DeviceContext &ctx, uint8_t track,
                                       bool mute, MidiUartClass *uart_) {
  (void)ctx;
  device_.muteTrack(track, mute, uart_);
}

uint8_t DeviceMixerCapability::trig_group(const DeviceContext &ctx,
                                          uint8_t track) const {
  (void)ctx;
  (void)track;
  return 255;
}

void DeviceMixerCapability::select_track(const DeviceContext &ctx,
                                         uint8_t track) {
  (void)ctx;
  (void)track;
}

void DeviceMixerCapability::restore_track_params(const DeviceContext &ctx,
                                                 uint8_t track) {
  (void)ctx;
  (void)track;
}

bool DeviceMixerCapability::is_mute_param(uint8_t param) const {
  return param == mute_param_;
}

bool DeviceMixerSupport::ext_level_param(uint8_t track, uint8_t param_idx,
                                         const uint8_t *levels,
                                         MidiDeviceMixerParam *param,
                                         bool require_level_cc) {
  if (param == nullptr || levels == nullptr || track >= NUM_EXT_TRACKS ||
      param_idx != 0 ||
      (require_level_cc && mcl_cfg.uart2_cc_level > 127)) {
    return false;
  }
  param->set_value(levels[track]);
  param->set_metadata("LEV", 0, true);
  return true;
}

bool DeviceMixerSupport::set_ext_level(uint8_t track, uint8_t param_idx,
                                       MidiDeviceMixerValue value,
                                       uint8_t *levels, uint8_t *level,
                                       bool require_level_cc) {
  if (levels == nullptr || level == nullptr || track >= NUM_EXT_TRACKS ||
      param_idx != 0 ||
      (require_level_cc && mcl_cfg.uart2_cc_level > 127)) {
    return false;
  }
  if (value < 0) value = 0;
  if (value > 127) value = 127;
  *level = (uint8_t)value;
  levels[track] = *level;
  return true;
}

bool DeviceMixerSupport::parse_ext_cc(uint8_t channel, uint8_t cc,
                                      uint8_t level_cc, uint8_t mute_cc,
                                      uint8_t *track, uint8_t *param) {
  if (track == nullptr || param == nullptr) {
    return false;
  }
  *track = mcl_seq.find_ext_track(channel);
  if (*track == 255) {
    return false;
  }
  if (cc == level_cc && level_cc <= 127) {
    *param = 0;
    return true;
  }
  if (cc == mute_cc) {
    *param = DeviceMixerCapability::MUTE_PARAM;
    return true;
  }
  return false;
}

void DeviceMixerSupport::update_ext_from_cc(uint8_t track, uint8_t param,
                                            MidiDeviceMixerValue value,
                                            uint8_t *levels) {
  if (track >= NUM_EXT_TRACKS) {
    return;
  }
  if (param == DeviceMixerCapability::MUTE_PARAM) {
    mcl_seq.ext_tracks[track].mute_state =
        value > 0 ? SEQ_MUTE_ON : SEQ_MUTE_OFF;
    return;
  }
  if (param == 0 && levels != nullptr) {
    if (value < 0) value = 0;
    if (value > 127) value = 127;
    levels[track] = (uint8_t)value;
  }
}

void DeviceMixerSupport::set_ext_record_mute(uint8_t track, bool state,
                                             bool clear) {
  if (track >= NUM_EXT_TRACKS) {
    return;
  }
  mcl_seq.ext_tracks[track].record_mutes = state;
  if (clear) {
    mcl_seq.ext_tracks[track].clear_mute();
  }
}

bool ExtMixerCapability::param(const DeviceContext &ctx, uint8_t track,
                               uint8_t param_idx,
                               MidiDeviceMixerParam *out) {
  (void)ctx;
  return DeviceMixerSupport::ext_level_param(track, param_idx, levels_, out,
                                              require_level_cc_);
}

uint8_t ExtMixerCapability::track_count(const DeviceContext &ctx) const {
  (void)ctx;
  return mcl_seq.num_ext_tracks;
}

SeqTrack *ExtMixerCapability::seq_track(const DeviceContext &ctx,
                                        uint8_t track) {
  (void)ctx;
  if (track >= mcl_seq.num_ext_tracks) {
    return nullptr;
  }
  return &mcl_seq.ext_tracks[track];
}

bool ExtMixerCapability::set_param(const DeviceContext &ctx, uint8_t track,
                                   uint8_t param_idx,
                                   MidiDeviceMixerValue value,
                                   bool send) {
  (void)ctx;
  uint8_t level = 0;
  if (!DeviceMixerSupport::set_ext_level(track, param_idx, value, levels_,
                                          &level, require_level_cc_)) {
    return false;
  }
  send_level(track, level, send);
  return true;
}

void ExtMixerCapability::set_record_mutes(const DeviceContext &ctx,
                                          uint8_t track, bool state,
                                          bool clear) {
  (void)ctx;
  DeviceMixerSupport::set_ext_record_mute(track, state, clear);
}

bool ExtMixerCapability::parse_cc(const DeviceContext &ctx, uint8_t channel,
                                  uint8_t cc, uint8_t *track,
                                  uint8_t *param) const {
  (void)ctx;
  return DeviceMixerSupport::parse_ext_cc(channel, cc, mcl_cfg.uart2_cc_level,
                                          device_.get_mute_cc(), track, param);
}

void ExtMixerCapability::update_from_cc(const DeviceContext &ctx,
                                        uint8_t track, uint8_t param,
                                        MidiDeviceMixerValue value) {
  (void)ctx;
  DeviceMixerSupport::update_ext_from_cc(track, param, value, levels_);
}

#if !defined(__AVR__)
DeviceStepTrackCapability::DeviceStepTrackCapability(MidiDevice &device)
    : DeviceCapability(device) {}

bool DeviceStepTrackCapability::available(const DeviceContext &ctx) const {
  (void)ctx;
  return false;
}

uint8_t DeviceStepTrackCapability::track_count(const DeviceContext &ctx) const {
  (void)ctx;
  return 0;
}

SeqStepTrackRef DeviceStepTrackCapability::track(const DeviceContext &ctx,
                                                 uint8_t track) const {
  (void)ctx;
  (void)track;
  return SeqStepTrackRef(mcl_seq.md_tracks[0]);
}

SeqStepTrackRef DeviceStepTrackCapability::active_track(
    const DeviceContext &ctx) const {
  return track(ctx, 0);
}

bool DeviceStepTrackCapability::parses_kit_cc(const DeviceContext &ctx) const {
  (void)ctx;
  return false;
}

bool DeviceStepTrackCapability::parse_kit_cc(const DeviceContext &ctx,
                                             uint8_t channel, uint8_t cc,
                                             uint8_t *track,
                                             uint8_t *param) const {
  (void)ctx;
  (void)channel;
  (void)cc;
  (void)track;
  (void)param;
  return false;
}

DeviceExtStepTrackCapability::DeviceExtStepTrackCapability(MidiDevice &device)
    : DeviceCapability(device) {}

bool DeviceExtStepTrackCapability::available(const DeviceContext &ctx) const {
  return ctx.device_idx() == DeviceIdx::Secondary;
}

uint8_t DeviceExtStepTrackCapability::track_count(
    const DeviceContext &ctx) const {
  (void)ctx;
  return mcl_seq.num_ext_tracks;
}

SeqExtStepTrackApi DeviceExtStepTrackCapability::track(
    const DeviceContext &ctx, uint8_t i) const {
  (void)ctx;
  if (i >= NUM_EXT_TRACKS) {
    i = 0;
  }
  return SeqExtStepTrackApi(mcl_seq.ext_tracks[i]);
}

SeqExtStepTrackApi DeviceExtStepTrackCapability::active_track(
    const DeviceContext &ctx) const {
  return track(ctx, last_ext_track);
}

bool DeviceExtStepTrackCapability::track_for_channel(
    const DeviceContext &ctx, uint8_t channel, uint8_t *track_index) const {
  (void)ctx;
  if (track_index == nullptr) {
    return false;
  }
  for (uint8_t n = 0; n < NUM_EXT_TRACKS; n++) {
    if (mcl_seq.ext_tracks[n].channel == channel) {
      *track_index = n;
      return true;
    }
  }
  *track_index = 255;
  return false;
}

MidiClass *DeviceExtStepTrackCapability::input_midi(
    const DeviceContext &ctx) const {
  (void)ctx;
  return device_.midi;
}

bool DeviceExtStepTrackCapability::is_mute_cc(const DeviceContext &ctx,
                                              uint8_t cc) const {
  (void)ctx;
  return cc == device_.get_mute_cc();
}

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

uint8_t DeviceParamCapability::target_count(const DeviceContext &ctx) const {
  (void)ctx;
  return 0;
}

uint8_t DeviceParamCapability::param_count(const DeviceContext &ctx,
                                           uint8_t target) const {
  (void)ctx;
  (void)target;
  return 0;
}

bool DeviceParamCapability::target_label(const DeviceContext &ctx,
                                         uint8_t target, char *out,
                                         uint8_t len) const {
  (void)ctx;
  (void)target;
  (void)out;
  (void)len;
  return false;
}

bool DeviceParamCapability::param_label(const DeviceContext &ctx,
                                        uint8_t target, uint8_t param,
                                        char *out, uint8_t len) {
  (void)ctx;
  (void)target;
  (void)param;
  (void)out;
  (void)len;
  return false;
}

bool DeviceParamCapability::get_param(const DeviceContext &ctx, uint8_t target,
                                      uint8_t param, uint8_t *value) {
  (void)ctx;
  (void)target;
  (void)param;
  (void)value;
  return false;
}

bool DeviceParamCapability::set_param(const DeviceContext &ctx, uint8_t target,
                                      uint8_t param, uint8_t value,
                                      MidiUartClass *uart_,
                                      bool update_kit) {
  (void)ctx;
  (void)target;
  (void)param;
  (void)value;
  (void)uart_;
  (void)update_kit;
  return false;
}

uint8_t DeviceParamCapability::sequencer_lock_param_count(
    const DeviceContext &ctx, uint8_t target) const {
  return param_count(ctx, target);
}

bool DeviceParamCapability::sequencer_lock_param_info(
    const DeviceContext &ctx, uint8_t target, uint8_t param,
    MidiDeviceParamInfo *info) {
  if (info == nullptr ||
      param >= sequencer_lock_param_count(ctx, target)) {
    return false;
  }
  *info = MidiDeviceParamInfo();
  info->active = true;
  info->sendable = true;
  info->param_id = param;
  info->ctrl = param;
  uint8_t value = 0;
  if (get_param(ctx, target, param, &value)) {
    info->default_value = value;
    info->current_value = value;
  }
  return true;
}

bool DeviceParamCapability::sequencer_lock_param_label(
    const DeviceContext &ctx, uint8_t target, uint8_t param, char *out,
    uint8_t len) {
  if (out == nullptr || len == 0 ||
      param >= sequencer_lock_param_count(ctx, target)) {
    return false;
  }
  if (param_label(ctx, target, param, out, len)) {
    return true;
  }
  copy_param_number_label('P', param, out, len);
  return true;
}

bool DeviceParamCapability::sequencer_uses_step_pitch(
    const DeviceContext &ctx, uint8_t target) const {
  (void)ctx;
  (void)target;
  return false;
}

uint8_t DeviceParamCapability::sequencer_pitch_lock_param(
    const DeviceContext &ctx, uint8_t target) const {
  (void)ctx;
  (void)target;
  return 0;
}

DevicePerfCapability::DevicePerfCapability(MidiDevice &device)
    : DeviceCapability(device) {}

bool DevicePerfCapability::perf_param_from_key(const DeviceContext &ctx,
                                               uint8_t target, uint8_t key,
                                               uint8_t *param) {
  DeviceStepEditCapability *editor = device_.step_edit();
  return editor != nullptr &&
         editor->param_from_key(ctx, target, key, param);
}

bool DevicePerfCapability::perf_key_for_param(const DeviceContext &ctx,
                                              uint8_t target, uint8_t param,
                                              uint8_t *key) {
  DeviceStepEditCapability *editor = device_.step_edit();
  return editor != nullptr &&
         editor->key_for_param(ctx, target, param, key);
}

bool DevicePerfCapability::perf_begin_param_editor(const DeviceContext &ctx,
                                                   uint8_t target,
                                                   uint8_t *params,
                                                   uint8_t count) {
  DeviceStepEditCapability *editor = device_.step_edit();
  return editor != nullptr &&
         editor->begin_param_editor(ctx, target, params, count);
}

void DevicePerfCapability::perf_end_param_editor(const DeviceContext &ctx) {
  DeviceStepEditCapability *editor = device_.step_edit();
  if (editor != nullptr) {
    editor->end_param_editor(ctx);
  }
}

void DevicePerfCapability::perf_set_rec_mode(const DeviceContext &ctx,
                                             uint8_t mode) {
  DeviceStepEditCapability *editor = device_.step_edit();
  if (editor != nullptr) {
    editor->set_rec_mode(ctx, mode);
  }
}

bool DevicePerfCapability::perf_scene_autofill(const DeviceContext &ctx,
                                               uint8_t dest_offset,
                                               PerfData *data,
                                               uint8_t scene) {
  (void)ctx;
  (void)dest_offset;
  (void)data;
  (void)scene;
  return false;
}
#endif

#if !defined(__AVR__)
DeviceStepEditCapability::DeviceStepEditCapability(MidiDevice &device)
    : DeviceCapability(device) {}

bool DeviceStepEditCapability::available(const DeviceContext &ctx) const {
  (void)ctx;
  return false;
}

void DeviceStepEditCapability::set_rec_mode(const DeviceContext &ctx,
                                            uint8_t mode) {
  (void)ctx;
  (void)mode;
}

void DeviceStepEditCapability::sync_track(const DeviceContext &ctx,
                                          uint8_t length, uint8_t speed,
                                          uint8_t step_count) {
  (void)ctx;
  (void)length;
  (void)speed;
  (void)step_count;
}

void DeviceStepEditCapability::set_trig_leds(const DeviceContext &ctx,
                                             uint16_t mask, uint8_t mode,
                                             uint8_t blink) {
  (void)ctx;
  (void)mask;
  (void)mode;
  (void)blink;
}

void DeviceStepEditCapability::set_live_param_update(const DeviceContext &ctx,
                                                     bool enabled) {
  (void)ctx;
  (void)enabled;
}

bool DeviceStepEditCapability::configure_kit_sound_panel(
    const DeviceContext &ctx, uint8_t target, char *info, uint8_t info_len,
    uint8_t *pitch_max, bool *is_midi_model) const {
  (void)ctx;
  (void)target;
  (void)info;
  (void)info_len;
  (void)pitch_max;
  (void)is_midi_model;
  return false;
}

bool DeviceStepEditCapability::kit_sound_uses_note_pitch(
    const DeviceContext &ctx, uint8_t target) const {
  (void)ctx;
  (void)target;
  return false;
}

bool DeviceStepEditCapability::kit_sound_voice_allocatable(
    const DeviceContext &ctx, uint8_t target) const {
  (void)ctx;
  (void)target;
  return false;
}

uint8_t DeviceStepEditCapability::kit_sound_default_pitch(
    const DeviceContext &ctx, uint8_t target) const {
  (void)ctx;
  (void)target;
  return 0;
}

uint8_t DeviceStepEditCapability::kit_sound_note_from_pitch(
    const DeviceContext &ctx, uint8_t target, uint8_t pitch) const {
  (void)ctx;
  (void)target;
  (void)pitch;
  return 255;
}

uint8_t DeviceStepEditCapability::kit_sound_pitch_from_note(
    const DeviceContext &ctx, uint8_t target, uint8_t note,
    uint8_t fine_tune) const {
  (void)ctx;
  (void)target;
  (void)note;
  (void)fine_tune;
  return 255;
}

bool DeviceStepEditCapability::param_from_key(const DeviceContext &ctx,
                                              uint8_t target, uint8_t key,
                                              uint8_t *param) const {
  (void)ctx;
  (void)target;
  (void)key;
  (void)param;
  return false;
}

bool DeviceStepEditCapability::key_for_param(const DeviceContext &ctx,
                                             uint8_t target, uint8_t param,
                                             uint8_t *key) const {
  (void)ctx;
  (void)target;
  (void)param;
  (void)key;
  return false;
}

bool DeviceStepEditCapability::begin_param_editor(const DeviceContext &ctx,
                                                  uint8_t target,
                                                  uint8_t *params,
                                                  uint8_t count) {
  (void)ctx;
  (void)target;
  (void)params;
  (void)count;
  return false;
}

void DeviceStepEditCapability::end_param_editor(const DeviceContext &ctx) {
  (void)ctx;
}

void DeviceStepEditCapability::close_microtiming(const DeviceContext &ctx) {
  (void)ctx;
}

void DeviceStepEditCapability::clear_popup(const DeviceContext &ctx) {
  (void)ctx;
}

void DeviceStepEditCapability::popup_text(const DeviceContext &ctx,
                                          char *text, uint8_t persistent) {
  (void)ctx;
  (void)text;
  (void)persistent;
}

bool DeviceStepEditCapability::parse_cc(const DeviceContext &ctx,
                                        uint8_t channel, uint8_t cc,
                                        uint8_t *target,
                                        uint8_t *param) const {
  (void)ctx;
  (void)channel;
  (void)cc;
  (void)target;
  (void)param;
  return false;
}
#endif

#if !defined(__AVR__)
void DevicePanelCapability::set_key_repeat(uint8_t enabled) {
  (void)enabled;
}

void DevicePanelCapability::set_rec_mode(uint8_t mode) {
  (void)mode;
}

void DevicePanelCapability::sync_seqtrack(uint8_t length, uint8_t speed,
                                          uint8_t step_count) {
  (void)length;
  (void)speed;
  (void)step_count;
}
#endif
