#include "TBD.h"

#ifdef PLATFORM_TBD

#include "GUI.h"
#include "GUI_hardware.h"
#include "MCL.h"
#include "MCLGUI.h"
#include "MCLSysConfig.h"
#include "MCLSeq.h"
#include "MidiDeviceGrid.h"
#include "MidiSetup.h"
#include "Project.h"
#include "SeqPages.h"
#include "SeqStepTrackRef.h"
#include "TBDTrack.h"
#include "TbdUiMode.h"
#include "TbdP4Command.h"
#include "TbdP4Realtime.h"
#include <Arduino.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef VERSION_STR
#define VERSION_STR "dev"
#endif

namespace {

struct P4BootPreset {
  uint8_t track_index;
  char preset_id[TBD_PRESET_ID_LEN];
  uint8_t rom_bank;
  int32_t sample_slice;
};

struct P4BootPresetFallback {
  uint8_t track_index;
  const char *preset_id;
  uint8_t rom_bank;
  int32_t sample_slice;
};

constexpr uint32_t kP4PresetRetryMs = 2000;
constexpr uint32_t kP4PresetReadyProbeMs = 100;
constexpr uint32_t kP4PresetBootReadyMs = 30000;
constexpr uint32_t kP4PresetCommandTimeoutMs = 30000;
constexpr uint32_t kP4BootSettleMs = 1000;
constexpr uint32_t kP4RackProcessorSettleMs = 5000;
constexpr size_t kP4SoundTrackCount = 16;
constexpr size_t kP4TrackDefaultsJsonBytes = 8192;
constexpr size_t kP4PresetValueCount = 32;
constexpr uint8_t kP4DefaultRomBank = 0xFF;
constexpr int32_t kP4DefaultSampleSlice = -1;
constexpr uint8_t kP4AppFlags = 0x03;
constexpr uint8_t kP4InitProgressMax = 32;
constexpr uint8_t kTbdUiSlotPrimary = 0;
constexpr uint8_t kTbdUiSlotSecondary = 1;
constexpr uint8_t kTbdUiSlotNone = 255;
constexpr uint8_t kP4DriverMidiChannel = 13;
constexpr const char *kP4RackPluginId = "PicoSeqRack";
constexpr const char *kP4ReferenceAppName = "Groovebox";

class TbdMixerCapability : public DeviceMixerCapability {
public:
  explicit TbdMixerCapability(TbdDevice &device)
      : DeviceMixerCapability(device) {}
  virtual bool param(const DeviceContext &ctx, uint8_t track,
                     uint8_t param_idx,
                     MidiDeviceMixerParam *param) override;
  virtual bool set_param(const DeviceContext &ctx, uint8_t track,
                         uint8_t param_idx, int16_t value,
                         bool send = true) override;
  virtual void mute_track(const DeviceContext &ctx, uint8_t track, bool mute,
                          MidiUartClass *uart_ = nullptr) override;
  virtual void set_record_mutes(const DeviceContext &ctx, uint8_t track,
                                bool state, bool clear = false) override;
  virtual bool parse_cc(const DeviceContext &ctx, uint8_t channel, uint8_t cc,
                        uint8_t *track, uint8_t *param) const override;
  virtual void update_from_cc(const DeviceContext &ctx, uint8_t track,
                              uint8_t param, int16_t value) override;

private:
  TbdDevice &tbd() const { return (TbdDevice &)device_; }
};

class TbdStepTrackCapability : public DeviceStepTrackCapability {
public:
  explicit TbdStepTrackCapability(TbdDevice &device)
      : DeviceStepTrackCapability(device) {}
  virtual bool available(const DeviceContext &ctx) const override;
  virtual uint8_t track_count(const DeviceContext &ctx) const override;
  virtual SeqStepTrackRef track(const DeviceContext &ctx,
                                uint8_t track) const override;
  virtual SeqStepTrackRef active_track(const DeviceContext &ctx) const override;

private:
  TbdDevice &tbd() const { return (TbdDevice &)device_; }
};

class TbdParamCapability : public DeviceParamCapability {
public:
  explicit TbdParamCapability(TbdDevice &device)
      : DeviceParamCapability(device) {}
  virtual uint8_t target_count(const DeviceContext &ctx) const override;
  virtual uint8_t param_count(const DeviceContext &ctx,
                              uint8_t target) const override;
  virtual bool target_label(const DeviceContext &ctx, uint8_t target,
                            char *out, uint8_t len) const override;
  virtual bool param_label(const DeviceContext &ctx, uint8_t target,
                           uint8_t param, char *out, uint8_t len) override;
  virtual bool get_param(const DeviceContext &ctx, uint8_t target,
                         uint8_t param, uint8_t *value) override;
  virtual bool set_param(const DeviceContext &ctx, uint8_t target,
                         uint8_t param, uint8_t value,
                         MidiUartClass *uart_ = nullptr) override;
  virtual uint8_t sequencer_lock_param_count(const DeviceContext &ctx,
                                             uint8_t target) const override;
  virtual bool sequencer_lock_param_info(const DeviceContext &ctx,
                                         uint8_t target, uint8_t param,
                                         MidiDeviceParamInfo *info) override;
  virtual bool sequencer_lock_param_label(const DeviceContext &ctx,
                                          uint8_t target, uint8_t param,
                                          char *out, uint8_t len) override;
  virtual bool sequencer_uses_step_pitch(const DeviceContext &ctx,
                                         uint8_t target) const override;
  virtual uint8_t sequencer_pitch_lock_param(const DeviceContext &ctx,
                                             uint8_t target) const override;
};

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

const P4BootPresetFallback kP4BootPresetFallbacks[] = {
    {0, "db-all-def", kP4DefaultRomBank, kP4DefaultSampleSlice},
    {1, "fmb-all-def", kP4DefaultRomBank, kP4DefaultSampleSlice},
    {2, "ds-all-def", kP4DefaultRomBank, kP4DefaultSampleSlice},
    {3, "hh1-all-def", kP4DefaultRomBank, kP4DefaultSampleSlice},
    {4, "rs-all-def", kP4DefaultRomBank, kP4DefaultSampleSlice},
    {5, "cl-all-def", kP4DefaultRomBank, kP4DefaultSampleSlice},
    {6, "ro-full-def", 0, 3},
    {7, "ro-full-def", 1, 1},
    {8, "td3-all-def", kP4DefaultRomBank, kP4DefaultSampleSlice},
    {9, "td3-all-def", kP4DefaultRomBank, kP4DefaultSampleSlice},
    {10, "mo-all-def", kP4DefaultRomBank, kP4DefaultSampleSlice},
    {11, "wtosc-all-def", kP4DefaultRomBank, kP4DefaultSampleSlice},
    {12, "ro-full-def", 2, 2},
    {13, "ro-full-def", 3, 0},
    {14, "pp-all-def", kP4DefaultRomBank, kP4DefaultSampleSlice},
    {15, "inp-all-def", kP4DefaultRomBank, kP4DefaultSampleSlice},
};

P4BootPreset p4_boot_presets[kP4SoundTrackCount];
TbdP4SoundData p4_default_sounds[kP4SoundTrackCount];
bool p4_default_sound_valid[kP4SoundTrackCount];
TbdP4AudioParamGroup p4_driver_params;
bool p4_driver_params_initialized = false;
char p4_track_defaults_json[kP4TrackDefaultsJsonBytes];
uint8_t p4_boot_stage = 0;
uint8_t p4_boot_fail_stage = 0;
uint8_t p4_boot_fail_request = 0;
uint8_t p4_boot_loaded_tracks = 0;
uint32_t p4_trigger_count = 0;

uint8_t count_p4_audio_params(const TbdP4SoundData &sound) {
  uint8_t count = 0;
  for (uint8_t i = 0; i < TBD_P4_AUDIO_PARAM_COUNT; i++) {
    if (sound.audio_params.params[i].type != TBD_P4_PARAM_TYPE_NONE) {
      count++;
    }
  }
  return count;
}

uint8_t count_p4_sendable_params(const TbdP4SoundData &sound) {
  uint8_t count = 0;
  for (uint8_t i = 0; i < TBD_P4_AUDIO_PARAM_COUNT; i++) {
    if (sound.audio_params.params[i].is_sendable()) {
      count++;
    }
  }
  for (uint8_t i = 0; i < TBD_P4_MIXER_PARAM_COUNT; i++) {
    if (sound.mixer_params.params[i].is_sendable()) {
      count++;
    }
  }
  return count;
}

uint8_t count_p4_sendable_mixer_params(const TbdP4SoundData &sound) {
  uint8_t count = 0;
  for (uint8_t i = 0; i < TBD_P4_MIXER_PARAM_COUNT; i++) {
    if (sound.mixer_params.params[i].is_sendable()) {
      count++;
    }
  }
  return count;
}

void debug_p4_json_result(const char *label, bool ok, const char *json) {
#ifdef DEBUGMODE
  const size_t len = json == nullptr ? 0 : strlen(json);
  char head[33];
  head[0] = '\0';
  if (json != nullptr && len > 0) {
    const size_t copy_len = len < sizeof(head) - 1 ? len : sizeof(head) - 1;
    memcpy(head, json, copy_len);
    head[copy_len] = '\0';
    for (size_t i = 0; i < copy_len; i++) {
      if (head[i] == '\n' || head[i] == '\r' || head[i] == '\t') {
        head[i] = ' ';
      }
    }
  }

  DEBUG_PRINT("tbd_init json ");
  DEBUG_PRINT(label);
  DEBUG_PRINT(" ok=");
  DEBUG_PRINT(ok ? 1 : 0);
  DEBUG_PRINT(" len=");
  DEBUG_PRINT((unsigned)len);
  DEBUG_PRINT(" head=");
  DEBUG_PRINTLN(head);
#else
  (void)label;
  (void)ok;
  (void)json;
#endif
}

void debug_p4_boot_preset(const char *label, const P4BootPreset &preset) {
#ifdef DEBUGMODE
  DEBUG_PRINT("tbd_init preset ");
  DEBUG_PRINT(label);
  DEBUG_PRINT(" p4=");
  DEBUG_PRINT((unsigned)preset.track_index);
  DEBUG_PRINT(" id=");
  DEBUG_PRINT(preset.preset_id);
  DEBUG_PRINT(" bank=");
  DEBUG_PRINT((unsigned)preset.rom_bank);
  DEBUG_PRINT(" slice=");
  DEBUG_PRINTLN((long)preset.sample_slice);
#else
  (void)label;
  (void)preset;
#endif
}

void debug_p4_sound_summary(const char *label, const TbdP4SoundData &sound) {
#ifdef DEBUGMODE
  DEBUG_PRINT("tbd_init sound ");
  DEBUG_PRINT(label);
  DEBUG_PRINT(" p4=");
  DEBUG_PRINT((unsigned)sound.p4_track_index);
  DEBUG_PRINT(" ch=");
  DEBUG_PRINT((unsigned)sound.midi_channel);
  DEBUG_PRINT(" preset=");
  DEBUG_PRINT(sound.preset_id);
  DEBUG_PRINT(" macro=");
  DEBUG_PRINT(sound.macro_id);
  DEBUG_PRINT(" mach=");
  DEBUG_PRINT(sound.machine_id);
  DEBUG_PRINT(" apg=");
  DEBUG_PRINT((unsigned)sound.audio_params.num_pages);
  DEBUG_PRINT(" ap=");
  DEBUG_PRINT((unsigned)count_p4_audio_params(sound));
  DEBUG_PRINT(" mpg=");
  DEBUG_PRINT((unsigned)sound.mixer_params.num_pages);
  DEBUG_PRINT(" send=");
  DEBUG_PRINTLN((unsigned)count_p4_sendable_params(sound));
#else
  (void)label;
  (void)sound;
#endif
}

void draw_p4_init_progress(uint8_t cur) {
  if (cur > kP4InitProgressMax) {
    cur = kP4InitProgressMax;
  }

  oled_display.clearDisplay();
  oled_display.setFont();
  oled_display.setTextSize(2);
  oled_display.setTextColor(WHITE, BLACK);
  oled_display.setCursor(12, 10);
  oled_display.print("TBD");

  oled_display.setTextSize(1);
  oled_display.setCursor(60, 10);
  oled_display.print("Init P4");
  mcl_gui.draw_progress_bar(cur, kP4InitProgressMax, false, 60, 25);
}

bool activate_p4_sample_kit(uint8_t kit_index, uint32_t timeout_ms) {
  const bool set_ok = tbd_p4_command.set_active_sample_kit(kit_index, timeout_ms);
#ifdef DEBUGMODE
  DEBUG_PRINT("tbd_init set_sample_kit index=");
  DEBUG_PRINT((unsigned)kit_index);
  DEBUG_PRINT(" ok=");
  DEBUG_PRINTLN(set_ok ? 1 : 0);
#endif
  if (!set_ok) {
    return false;
  }

  delay(100);
  const bool bank_ok = tbd_p4_command.get_sample_bank_index_json(
      p4_track_defaults_json, sizeof(p4_track_defaults_json), timeout_ms);
  debug_p4_json_result("sample_bank_index", bank_ok, p4_track_defaults_json);
  return true;
}

void debug_p4_active_plugin(uint32_t timeout_ms) {
  char active_plugin[32] = {0};
  const bool get_ok = tbd_p4_command.get_active_plugin(
      0, active_plugin, sizeof(active_plugin), timeout_ms);
#ifdef DEBUGMODE
  DEBUG_PRINT("tbd_init active_plugin get=");
  DEBUG_PRINT(get_ok ? 1 : 0);
  DEBUG_PRINT(" id=");
  DEBUG_PRINTLN(active_plugin);
#endif
#ifdef DEBUGMODE
  if (get_ok && strcmp(active_plugin, kP4RackPluginId) != 0) {
    DEBUG_PRINT("tbd_init plugin_mismatch was=");
    DEBUG_PRINT(active_plugin);
    DEBUG_PRINT(" want=");
    DEBUG_PRINTLN(kP4RackPluginId);
  }
#endif
}

TbdP4SoundData *p4_sound_for_mixer(uint8_t device_idx, uint8_t track) {
  if (device_idx == kTbdUiSlotPrimary) {
    if (mcl_cfg.grid_x_device != GRID_X_DEVICE_TBD ||
        track >= mcl_seq.num_tbd_tracks) {
      return nullptr;
    }
    return &mcl_seq.tbd_tracks[track].p4_sound;
  }
  if (device_idx == kTbdUiSlotSecondary) {
    if (mcl_cfg.grid_y_device != GRID_Y_DEVICE_TBD ||
        track >= mcl_seq.num_midi_tracks) {
      return nullptr;
    }
    return &mcl_seq.midi_tracks[track].p4_sound;
  }
  return nullptr;
}

bool p4_param_available_for_mod(const TbdP4ParamDescriptor &desc) {
  return desc.is_visible() && desc.is_sendable();
}

TbdP4ParamDescriptor *p4_param_for_mod(uint8_t device_idx, uint8_t track,
                                       uint8_t param_idx) {
  TbdP4SoundData *sound = p4_sound_for_mixer(device_idx, track);
  if (sound == nullptr) {
    return nullptr;
  }

  uint8_t idx = 0;
  for (uint8_t i = 0; i < TBD_P4_AUDIO_PARAM_COUNT; i++) {
    TbdP4ParamDescriptor &desc = sound->audio_params.params[i];
    if (!p4_param_available_for_mod(desc)) {
      continue;
    }
    if (idx == param_idx) {
      return &desc;
    }
    idx++;
  }
  for (uint8_t i = 0; i < TBD_P4_MIXER_PARAM_COUNT; i++) {
    TbdP4ParamDescriptor &desc = sound->mixer_params.params[i];
    if (!p4_param_available_for_mod(desc)) {
      continue;
    }
    if (idx == param_idx) {
      return &desc;
    }
    idx++;
  }
  return nullptr;
}

uint8_t p4_param_count_for_mod(uint8_t device_idx, uint8_t track) {
  TbdP4SoundData *sound = p4_sound_for_mixer(device_idx, track);
  if (sound == nullptr) {
    return 0;
  }
  uint8_t count = 0;
  for (uint8_t i = 0; i < TBD_P4_AUDIO_PARAM_COUNT; i++) {
    if (p4_param_available_for_mod(sound->audio_params.params[i])) {
      count++;
    }
  }
  for (uint8_t i = 0; i < TBD_P4_MIXER_PARAM_COUNT; i++) {
    if (p4_param_available_for_mod(sound->mixer_params.params[i])) {
      count++;
    }
  }
  return count;
}

uint8_t p4_value_to_u7(const TbdP4ParamDescriptor &desc, int16_t value) {
  if (desc.max_value <= desc.min_value) {
    return 0;
  }
  if (value < desc.min_value) value = desc.min_value;
  if (value > desc.max_value) value = desc.max_value;
  uint16_t range = (uint16_t)(desc.max_value - desc.min_value);
  uint16_t offset = (uint16_t)(value - desc.min_value);
  return (uint8_t)(((uint32_t)offset * 127u + (range / 2u)) / range);
}

uint8_t p4_value_to_u7(const TbdP4ParamDescriptor &desc) {
  return p4_value_to_u7(desc, desc.value);
}

int16_t p4_u7_to_value(const TbdP4ParamDescriptor &desc, uint8_t value) {
  uint16_t value14 =
      (uint16_t)(((uint32_t)value * 0x3FFFu + 63u) / 127u);
  return tbd_p4_scale_lock_value(desc, value14);
}

TbdP4SoundData *active_p4_sound_for_note() {
  TbdP4SoundData *sound = tbd_ui_mode.active_sound();
  if (sound != nullptr) {
    return sound;
  }

  if (mcl_cfg.grid_x_device == GRID_X_DEVICE_TBD &&
      last_md_track < mcl_seq.num_tbd_tracks) {
    return &mcl_seq.tbd_tracks[last_md_track].p4_sound;
  }

#ifdef EXT_TRACKS
  if (mcl_cfg.grid_y_device == GRID_Y_DEVICE_TBD &&
      last_ext_track < mcl_seq.num_midi_tracks) {
    return &mcl_seq.midi_tracks[last_ext_track].p4_sound;
  }
#endif

  return nullptr;
}

SeqTrack *seq_track_for_mixer(uint8_t device_idx, uint8_t track) {
  if (device_idx == kTbdUiSlotPrimary) {
    if (mcl_cfg.grid_x_device != GRID_X_DEVICE_TBD ||
        track >= mcl_seq.num_tbd_tracks) {
      return nullptr;
    }
    return &mcl_seq.tbd_tracks[track];
  }
  if (device_idx == kTbdUiSlotSecondary) {
    if (mcl_cfg.grid_y_device != GRID_Y_DEVICE_TBD ||
        track >= mcl_seq.num_midi_tracks) {
      return nullptr;
    }
    return &mcl_seq.midi_tracks[track];
  }
  return nullptr;
}

void copy_fixed_string(char *dst, size_t dst_len, const char *src) {
  if (dst == nullptr || dst_len == 0) {
    return;
  }
  if (src == nullptr) {
    dst[0] = '\0';
    return;
  }
  strncpy(dst, src, dst_len);
  dst[dst_len - 1] = '\0';
}

void copy_preset_id(char *dst, const char *src) {
  if (src == nullptr) {
    dst[0] = '\0';
    return;
  }
  copy_fixed_string(dst, TBD_PRESET_ID_LEN, src);
}

void init_driver_param(TbdP4ParamDescriptor &param, const char *name,
                       uint8_t type, uint8_t ctrl, int16_t value,
                       int16_t min_value = 0, int16_t max_value = 127) {
  param.clear();
  param.type = type;
  copy_fixed_string(param.shortname, sizeof(param.shortname), name);
  param.min_value = min_value;
  param.max_value = max_value;
  param.default_value = value;
  param.value = value;
  param.ctrl = ctrl;
  param.ctrl_type = TBD_P4_CTRLTYPE_CC;
  param.resolution = 127;
}

void init_driver_page(uint8_t page, const char *name) {
  if (page >= TBD_P4_AUDIO_PARAM_PAGE_COUNT) {
    return;
  }
  copy_fixed_string(p4_driver_params.pages[page].name,
                    sizeof(p4_driver_params.pages[page].name), name);
  if (p4_driver_params.num_pages <= page) {
    p4_driver_params.num_pages = page + 1;
  }
}

void ensure_p4_driver_params_initialized() {
  if (p4_driver_params_initialized) {
    return;
  }

  p4_driver_params.clear();
  init_driver_page(0, "FX1 DLY1");
  init_driver_param(p4_driver_params.params[0], "Time",
                    TBD_P4_PARAM_TYPE_NUMBER, 20, 16);
  init_driver_param(p4_driver_params.params[1], "Sync",
                    TBD_P4_PARAM_TYPE_ONOFF, 21, 0);
  init_driver_param(p4_driver_params.params[2], "Freeze",
                    TBD_P4_PARAM_TYPE_ONOFF, 22, 0);
  init_driver_param(p4_driver_params.params[3], "Tapedig",
                    TBD_P4_PARAM_TYPE_TAPE_DIGITAL, 23, 0);

  init_driver_page(1, "FX1 DLY2");
  init_driver_param(p4_driver_params.params[4], "St.Wid",
                    TBD_P4_PARAM_TYPE_BIPOLAR, 24, 32);
  init_driver_param(p4_driver_params.params[5], "Fx2send",
                    TBD_P4_PARAM_TYPE_LEVEL, 25, 0);
  init_driver_param(p4_driver_params.params[6], "Feedb.",
                    TBD_P4_PARAM_TYPE_LEVEL, 26, 32);
  init_driver_param(p4_driver_params.params[7], "Base",
                    TBD_P4_PARAM_TYPE_NUMBER, 27, 0);

  init_driver_page(2, "FX1 DLY3");
  init_driver_param(p4_driver_params.params[8], "Width2",
                    TBD_P4_PARAM_TYPE_BIPOLAR, 28, 32);
  init_driver_param(p4_driver_params.params[9], "Dly.Lev",
                    TBD_P4_PARAM_TYPE_LEVEL, 29, 64);
  init_driver_param(p4_driver_params.params[10], "RevTime",
                    TBD_P4_PARAM_TYPE_NUMBER, 40, 64);
  init_driver_param(p4_driver_params.params[11], "RevLP",
                    TBD_P4_PARAM_TYPE_FILTER_CUTOFF, 41, 96);

  init_driver_page(3, "FX2 REV");
  init_driver_param(p4_driver_params.params[12], "Rev.Lev",
                    TBD_P4_PARAM_TYPE_LEVEL, 42, 64);
  init_driver_param(p4_driver_params.params[13], "Thres",
                    TBD_P4_PARAM_TYPE_DB_THRESHOLD, 60, 100);
  init_driver_param(p4_driver_params.params[14], "Ratio",
                    TBD_P4_PARAM_TYPE_RATIO, 61, 32);
  init_driver_param(p4_driver_params.params[15], "Atk",
                    TBD_P4_PARAM_TYPE_ENV_ATTACK, 62, 0);

  init_driver_page(4, "COMP");
  init_driver_param(p4_driver_params.params[16], "Rel",
                    TBD_P4_PARAM_TYPE_ENV_DECAY, 63, 20);
  init_driver_param(p4_driver_params.params[17], "LPF",
                    TBD_P4_PARAM_TYPE_FILTER_CUTOFF, 64, 48);
  init_driver_param(p4_driver_params.params[18], "Gain",
                    TBD_P4_PARAM_TYPE_DB_GAIN, 65, 0);
  init_driver_param(p4_driver_params.params[19], "Mix",
                    TBD_P4_PARAM_TYPE_LEVEL, 66, 64);

  init_driver_page(5, "SUM");
  init_driver_param(p4_driver_params.params[20], "Dly.Lev",
                    TBD_P4_PARAM_TYPE_LEVEL, 67, 64);
  init_driver_param(p4_driver_params.params[21], "Rev.Lev",
                    TBD_P4_PARAM_TYPE_LEVEL, 68, 64);
  init_driver_param(p4_driver_params.params[22], "Mute",
                    TBD_P4_PARAM_TYPE_ONOFF, 80, 0);
  init_driver_param(p4_driver_params.params[23], "Lev",
                    TBD_P4_PARAM_TYPE_LEVEL_MASTER, 81, 110);
  p4_driver_params_initialized = true;
}

void reset_p4_boot_presets_to_fallback() {
  for (auto &preset : p4_boot_presets) {
    preset.track_index = 0;
    preset.preset_id[0] = '\0';
    preset.rom_bank = kP4DefaultRomBank;
    preset.sample_slice = kP4DefaultSampleSlice;
  }

  for (const auto &fallback : kP4BootPresetFallbacks) {
    if (fallback.track_index >= kP4SoundTrackCount) {
      continue;
    }
    auto &preset = p4_boot_presets[fallback.track_index];
    preset.track_index = fallback.track_index;
    preset.rom_bank = fallback.rom_bank;
    preset.sample_slice = fallback.sample_slice;
    copy_preset_id(preset.preset_id, fallback.preset_id);
  }

  memset(p4_default_sound_valid, 0, sizeof(p4_default_sound_valid));
}

const char *skip_json_ws(const char *p, const char *end) {
  while (p < end && isspace((unsigned char)*p)) {
    p++;
  }
  return p;
}

const char *find_json_value(const char *begin, const char *end,
                            const char *key) {
  const size_t key_len = strlen(key);
  const char *p = begin;

  while (p < end) {
    if (*p != '"') {
      p++;
      continue;
    }

    if ((size_t)(end - p) < key_len + 3) {
      return nullptr;
    }

    if (strncmp(p + 1, key, key_len) == 0 && p[1 + key_len] == '"') {
      const char *v = skip_json_ws(p + key_len + 2, end);
      if (v < end && *v == ':') {
        return skip_json_ws(v + 1, end);
      }
    }
    p++;
  }

  return nullptr;
}

bool read_json_string(const char *begin, const char *end, const char *key,
                      char *dst, size_t dst_len) {
  if (dst == nullptr || dst_len == 0) {
    return false;
  }
  dst[0] = '\0';

  const char *p = find_json_value(begin, end, key);
  if (p == nullptr || p >= end || *p != '"') {
    return false;
  }
  p++;

  size_t copied = 0;
  bool escaped = false;
  while (p < end) {
    const char c = *p++;
    if (escaped) {
      if (copied + 1 < dst_len) {
        dst[copied++] = c;
      }
      escaped = false;
      continue;
    }
    if (c == '\\') {
      escaped = true;
      continue;
    }
    if (c == '"') {
      dst[copied] = '\0';
      return true;
    }
    if (copied + 1 < dst_len) {
      dst[copied++] = c;
    }
  }

  dst[copied] = '\0';
  return false;
}

bool read_json_int(const char *begin, const char *end, const char *key,
                   long *value) {
  const char *p = find_json_value(begin, end, key);
  if (p == nullptr || p >= end) {
    return false;
  }

  char *parse_end = nullptr;
  const long parsed = strtol(p, &parse_end, 10);
  if (parse_end == p || parse_end > end) {
    return false;
  }

  *value = parsed;
  return true;
}

const char *find_json_object_end(const char *begin, const char *end) {
  bool in_string = false;
  bool escaped = false;
  int depth = 0;

  for (const char *p = begin; p < end; p++) {
    const char c = *p;
    if (in_string) {
      if (escaped) {
        escaped = false;
      } else if (c == '\\') {
        escaped = true;
      } else if (c == '"') {
        in_string = false;
      }
      continue;
    }

    if (c == '"') {
      in_string = true;
    } else if (c == '{') {
      depth++;
    } else if (c == '}') {
      depth--;
      if (depth == 0) {
        return p + 1;
      }
    }
  }

  return nullptr;
}

const char *find_json_array_end(const char *begin, const char *end) {
  bool in_string = false;
  bool escaped = false;
  int depth = 0;

  for (const char *p = begin; p < end; p++) {
    const char c = *p;
    if (in_string) {
      if (escaped) {
        escaped = false;
      } else if (c == '\\') {
        escaped = true;
      } else if (c == '"') {
        in_string = false;
      }
      continue;
    }

    if (c == '"') {
      in_string = true;
    } else if (c == '[') {
      depth++;
    } else if (c == ']') {
      depth--;
      if (depth == 0) {
        return p + 1;
      }
    }
  }

  return nullptr;
}

uint8_t p4_ctrl_type_from_string(const char *type) {
  if (type != nullptr &&
      (strcmp(type, "nrpm") == 0 || strcmp(type, "nrpn") == 0)) {
    return TBD_P4_CTRLTYPE_NRPM;
  }
  return TBD_P4_CTRLTYPE_CC;
}

uint8_t p4_param_type_from_ui(const char *ui) {
  if (ui == nullptr) return TBD_P4_PARAM_TYPE_BIG_NUMBER;
  if (strcmp(ui, "freq") == 0) return TBD_P4_PARAM_TYPE_FREQ;
  if (strcmp(ui, "samplebank") == 0) return TBD_P4_PARAM_TYPE_SAMPLE_BANK;
  if (strcmp(ui, "sampleslice") == 0) return TBD_P4_PARAM_TYPE_SAMPLE_SLICE;
  if (strcmp(ui, "decay") == 0 || strcmp(ui, "envdecay") == 0)
    return TBD_P4_PARAM_TYPE_ENV_DECAY;
  if (strcmp(ui, "envattack") == 0) return TBD_P4_PARAM_TYPE_ENV_ATTACK;
  if (strcmp(ui, "envattackfast") == 0)
    return TBD_P4_PARAM_TYPE_ENV_ATTACK_FAST;
  if (strcmp(ui, "envamount") == 0) return TBD_P4_PARAM_TYPE_ENV_AMOUNT;
  if (strcmp(ui, "filtertype") == 0) return TBD_P4_PARAM_TYPE_FILTER_TYPE;
  if (strcmp(ui, "filtermode") == 0) return TBD_P4_PARAM_TYPE_FILTER_MODE;
  if (strcmp(ui, "filtercutoff") == 0)
    return TBD_P4_PARAM_TYPE_FILTER_CUTOFF;
  if (strcmp(ui, "filterq") == 0) return TBD_P4_PARAM_TYPE_FILTER_Q;
  if (strcmp(ui, "shape") == 0) return TBD_P4_PARAM_TYPE_SHAPE;
  if (strcmp(ui, "shape2") == 0) return TBD_P4_PARAM_TYPE_SHAPE2;
  if (strcmp(ui, "shape3") == 0) return TBD_P4_PARAM_TYPE_SHAPE3;
  if (strcmp(ui, "noise") == 0) return TBD_P4_PARAM_TYPE_NOISE;
  if (strcmp(ui, "distortion") == 0) return TBD_P4_PARAM_TYPE_DISTORTION;
  if (strcmp(ui, "pan") == 0) return TBD_P4_PARAM_TYPE_PAN;
  if (strcmp(ui, "level") == 0) return TBD_P4_PARAM_TYPE_LEVEL;
  if (strcmp(ui, "gainlevel") == 0) return TBD_P4_PARAM_TYPE_GAIN_LEVEL;
  if (strcmp(ui, "number") == 0) return TBD_P4_PARAM_TYPE_NUMBER;
  if (strcmp(ui, "macroshape") == 0) return TBD_P4_PARAM_TYPE_MACRO_SHAPE;
  if (strcmp(ui, "chord") == 0) return TBD_P4_PARAM_TYPE_CHORD_SHAPE;
  if (strcmp(ui, "chordinv") == 0) return TBD_P4_PARAM_TYPE_CHORD_INV;
  if (strcmp(ui, "nnotes") == 0) return TBD_P4_PARAM_TYPE_NNOTES;
  if (strcmp(ui, "onoff") == 0) return TBD_P4_PARAM_TYPE_ONOFF;
  if (strcmp(ui, "tapedigital") == 0) return TBD_P4_PARAM_TYPE_TAPE_DIGITAL;
  if (strcmp(ui, "freesync") == 0) return TBD_P4_PARAM_TYPE_FREE_SYNC;
  if (strcmp(ui, "timedivisor") == 0) return TBD_P4_PARAM_TYPE_TIME_DIVISOR;
  if (strcmp(ui, "scale") == 0) return TBD_P4_PARAM_TYPE_SCALE;
  if (strcmp(ui, "bipolar") == 0) return TBD_P4_PARAM_TYPE_BIPOLAR;
  if (strcmp(ui, "pitchsemi") == 0) return TBD_P4_PARAM_TYPE_PITCH_SEMI;
  if (strcmp(ui, "speedmult") == 0) return TBD_P4_PARAM_TYPE_SPEED_MULT;
  if (strcmp(ui, "bitcr") == 0) return TBD_P4_PARAM_TYPE_BITCR_BITS;
  if (strcmp(ui, "percent") == 0) return TBD_P4_PARAM_TYPE_PERCENT;
  if (strcmp(ui, "tbdmodel") == 0) return TBD_P4_PARAM_TYPE_TBD_MODEL;
  if (strcmp(ui, "tbdmtype") == 0) return TBD_P4_PARAM_TYPE_TBD_MOD_TYPE;
  if (strcmp(ui, "tbdvoices") == 0) return TBD_P4_PARAM_TYPE_TBD_VOICES;
  if (strcmp(ui, "plaitsmodel") == 0) return TBD_P4_PARAM_TYPE_PLAITS_MODEL;
  if (strcmp(ui, "plaitsharm") == 0) return TBD_P4_PARAM_TYPE_PLAITS_HARM;
  if (strcmp(ui, "plaitscolor") == 0) return TBD_P4_PARAM_TYPE_PLAITS_COLOR;
  if (strcmp(ui, "plaitsdetune") == 0 || strcmp(ui, "detune") == 0)
    return TBD_P4_PARAM_TYPE_PLAITS_DETUNE;
  if (strcmp(ui, "plaitsdecay") == 0) return TBD_P4_PARAM_TYPE_PLAITS_DECAY;
  if (strcmp(ui, "tbdchord") == 0) return TBD_P4_PARAM_TYPE_TBD_CHORD;
  if (strcmp(ui, "tbdenv") == 0) return TBD_P4_PARAM_TYPE_TBD_ENVSH;
  if (strcmp(ui, "tbdmsnap") == 0) return TBD_P4_PARAM_TYPE_TBD_MSNAP;
  if (strcmp(ui, "wtoscbank") == 0) return TBD_P4_PARAM_TYPE_WTOSC_BANK;
  if (strcmp(ui, "lforate") == 0) return TBD_P4_PARAM_TYPE_LFO_RATE;
  if (strcmp(ui, "td3ftype") == 0) return TBD_P4_PARAM_TYPE_TD3_FTYPE;
  if (strcmp(ui, "td3drive") == 0) return TBD_P4_PARAM_TYPE_TD3_DRIVE;
  if (strcmp(ui, "td3acclev") == 0) return TBD_P4_PARAM_TYPE_TD3_ACCLEV;
  if (strcmp(ui, "td3accent") == 0) return TBD_P4_PARAM_TYPE_TD3_ACCENT;
  if (strcmp(ui, "hidden") == 0) return TBD_P4_PARAM_TYPE_HIDDEN;
  return TBD_P4_PARAM_TYPE_BIG_NUMBER;
}

bool parse_p4_preset_values(const char *json, const char *end,
                            int16_t *values, bool *value_set,
                            size_t value_count) {
  const char *array = find_json_value(json, end, "values");
  if (array == nullptr || array >= end || *array != '[') {
    return false;
  }

  const char *array_end = find_json_array_end(array, end);
  if (array_end == nullptr) {
    return false;
  }

  size_t idx = 0;
  const char *p = array + 1;
  while (p < array_end && idx < value_count) {
    p = skip_json_ws(p, array_end);
    if (p >= array_end || *p == ']') {
      break;
    }

    char *parse_end = nullptr;
    long parsed = strtol(p, &parse_end, 10);
    if (parse_end != p && parse_end <= array_end) {
      if (parsed < INT16_MIN) parsed = INT16_MIN;
      if (parsed > INT16_MAX) parsed = INT16_MAX;
      values[idx] = (int16_t)parsed;
      value_set[idx] = true;
      idx++;
      p = parse_end;
      continue;
    }

    p++;
  }

  return idx > 0;
}

bool mapping_references_src(const char *obj, const char *obj_end, int idx) {
  const char *add = find_json_value(obj, obj_end, "add");
  if (add == nullptr || add >= obj_end || *add != '[') {
    return false;
  }

  const char *add_end = find_json_array_end(add, obj_end);
  if (add_end == nullptr) {
    return false;
  }

  const char *p = add + 1;
  while (p < add_end) {
    p = skip_json_ws(p, add_end);
    if (p >= add_end || *p == ']') break;
    if (*p != '{') {
      p++;
      continue;
    }

    const char *src_end = find_json_object_end(p, add_end);
    if (src_end == nullptr) {
      break;
    }

    long src = -1;
    if (read_json_int(p, src_end, "src", &src) && src == idx) {
      return true;
    }
    p = src_end;
  }

  return false;
}

bool scan_mapping_ctrl(const char *json, const char *end, int idx,
                       bool identity_only, uint8_t *ctrl,
                       uint8_t *ctrl_type) {
  const char *mappings = find_json_value(json, end, "mapping");
  if (mappings == nullptr || mappings >= end || *mappings != '[') {
    return false;
  }

  const char *mappings_end = find_json_array_end(mappings, end);
  if (mappings_end == nullptr) {
    return false;
  }

  const char *p = mappings + 1;
  while (p < mappings_end) {
    p = skip_json_ws(p, mappings_end);
    if (p >= mappings_end || *p == ']') break;
    if (*p != '{') {
      p++;
      continue;
    }

    const char *obj_end = find_json_object_end(p, mappings_end);
    if (obj_end == nullptr) {
      break;
    }

    long mapping_ctrl = -1;
    char type[8] = {0};
    read_json_string(p, obj_end, "type", type, sizeof(type));
    const bool identity = read_json_int(p, obj_end, "ctrl", &mapping_ctrl) &&
                          mapping_ctrl == idx + 8;
    const bool src_match = !identity_only && mapping_references_src(p, obj_end, idx);
    if ((identity_only && identity) || src_match) {
      if (mapping_ctrl < 0) mapping_ctrl = 0;
      if (mapping_ctrl > 255) mapping_ctrl = 255;
      *ctrl = (uint8_t)mapping_ctrl;
      *ctrl_type = p4_ctrl_type_from_string(type);
      return true;
    }

    p = obj_end;
  }

  return false;
}

bool find_mapping_ctrl_for_idx(const char *json, const char *end, int idx,
                               uint8_t *ctrl, uint8_t *ctrl_type) {
  return scan_mapping_ctrl(json, end, idx, true, ctrl, ctrl_type) ||
         scan_mapping_ctrl(json, end, idx, false, ctrl, ctrl_type);
}

uint8_t count_mapping_targets_for_idx(const char *json, const char *end,
                                      int idx) {
  const char *mappings = find_json_value(json, end, "mapping");
  if (mappings == nullptr || mappings >= end || *mappings != '[') {
    return 0;
  }

  const char *mappings_end = find_json_array_end(mappings, end);
  if (mappings_end == nullptr) {
    return 0;
  }

  uint8_t count = 0;
  const char *p = mappings + 1;
  while (p < mappings_end) {
    p = skip_json_ws(p, mappings_end);
    if (p >= mappings_end || *p == ']') break;
    if (*p != '{') {
      p++;
      continue;
    }

    const char *obj_end = find_json_object_end(p, mappings_end);
    if (obj_end == nullptr) {
      break;
    }
    if (mapping_references_src(p, obj_end, idx) && count < 255) {
      count++;
    }
    p = obj_end;
  }

  return count;
}

bool parse_p4_preset_json(const char *json, TbdP4SoundData &sound,
                          int16_t *values, bool *value_set,
                          size_t value_count) {
  if (json == nullptr) {
    return false;
  }

  const char *end = json + strlen(json);
  read_json_string(json, end, "id", sound.preset_id, sizeof(sound.preset_id));
  read_json_string(json, end, "name", sound.preset_name,
                   sizeof(sound.preset_name));
  read_json_string(json, end, "macro", sound.macro_id,
                   sizeof(sound.macro_id));
  parse_p4_preset_values(json, end, values, value_set, value_count);
  return sound.macro_id[0] != '\0';
}

bool parse_p4_macro_definition(const char *json, const int16_t *values,
                               const bool *value_set,
                               size_t value_count,
                               TbdP4SoundData &sound) {
  if (json == nullptr) {
    return false;
  }

  const char *end = json + strlen(json);
  read_json_string(json, end, "id", sound.macro_id, sizeof(sound.macro_id));
  read_json_string(json, end, "machine", sound.machine_id,
                   sizeof(sound.machine_id));

  sound.audio_params.clear();
  const char *groups = find_json_value(json, end, "groups");
  if (groups == nullptr || groups >= end || *groups != '[') {
    return false;
  }

  const char *groups_end = find_json_array_end(groups, end);
  if (groups_end == nullptr) {
    return false;
  }

  int8_t idx_to_slot[kP4PresetValueCount];
  for (size_t i = 0; i < kP4PresetValueCount; i++) {
    idx_to_slot[i] = -1;
  }

  bool parsed_any = false;
  uint8_t gidx = 0;
  const char *g = groups + 1;
  while (g < groups_end && gidx < TBD_P4_AUDIO_PARAM_PAGE_COUNT) {
    g = skip_json_ws(g, groups_end);
    if (g >= groups_end || *g == ']') break;
    if (*g != '{') {
      g++;
      continue;
    }

    const char *group_end = find_json_object_end(g, groups_end);
    if (group_end == nullptr) {
      break;
    }

    read_json_string(g, group_end, "name", sound.audio_params.pages[gidx].name,
                     sizeof(sound.audio_params.pages[gidx].name));
    sound.audio_params.num_pages = gidx + 1;

    const char *params = find_json_value(g, group_end, "parameters");
    if (params != nullptr && params < group_end && *params == '[') {
      const char *params_end = find_json_array_end(params, group_end);
      const char *p = params + 1;
      uint8_t pidx = 0;
      while (params_end != nullptr && p < params_end) {
        p = skip_json_ws(p, params_end);
        if (p >= params_end || *p == ']') break;
        if (*p != '{') {
          p++;
          continue;
        }

        const char *param_end = find_json_object_end(p, params_end);
        if (param_end == nullptr) {
          break;
        }

        const uint8_t slot = gidx * 4 + pidx;
        if (slot < TBD_P4_AUDIO_PARAM_COUNT) {
          TbdP4ParamDescriptor &desc = sound.audio_params.params[slot];
          desc.clear();

          char ui[24] = {0};
          long def = 0;
          long min_value = 0;
          long max_value = 127;
          long resolution = 127;
          long param_idx = -1;

          read_json_string(p, param_end, "name", desc.shortname,
                           sizeof(desc.shortname));
          read_json_string(p, param_end, "ui", ui, sizeof(ui));
          read_json_int(p, param_end, "def", &def);
          read_json_int(p, param_end, "min", &min_value);
          read_json_int(p, param_end, "max", &max_value);
          read_json_int(p, param_end, "res", &resolution);
          read_json_int(p, param_end, "idx", &param_idx);

          desc.type = p4_param_type_from_ui(ui);
          desc.min_value = (int16_t)min_value;
          desc.max_value = (int16_t)max_value;
          desc.default_value = (int16_t)def;
          desc.value = (int16_t)def;
          desc.resolution = (uint16_t)resolution;
          desc.ctrl_type = TBD_P4_CTRLTYPE_UNKNOWN;

          if (param_idx >= 0 && (size_t)param_idx < value_count) {
            if (value_set[param_idx]) {
              desc.value = values[param_idx];
            }
            if (desc.type != TBD_P4_PARAM_TYPE_HIDDEN) {
              idx_to_slot[param_idx] = (int8_t)slot;
            }
          }
          parsed_any = true;
        }

        pidx++;
        p = param_end;
      }
    }

    gidx++;
    g = group_end;
  }

  for (size_t idx = 0; idx < value_count; idx++) {
    const int8_t slot = idx_to_slot[idx];
    if (slot < 0) continue;

    TbdP4ParamDescriptor &desc = sound.audio_params.params[slot];
    uint8_t ctrl = 0;
    uint8_t ctrl_type = TBD_P4_CTRLTYPE_UNKNOWN;
    if (find_mapping_ctrl_for_idx(json, end, (int)idx, &ctrl, &ctrl_type)) {
      const int16_t base =
          sound.device_start_cc < 0 ? 0 : sound.device_start_cc;
      int16_t final_ctrl = base + ctrl;
      if (final_ctrl < 0) final_ctrl = 0;
      if (final_ctrl > 255) final_ctrl = 255;
      desc.ctrl = (uint8_t)final_ctrl;
      desc.ctrl_type = ctrl_type;
    }
    if (count_mapping_targets_for_idx(json, end, (int)idx) > 1) {
      desc.flags |= TBD_P4_AUDIO_PARAM_FLAG_MACRO;
    }
  }

  return parsed_any;
}

bool sound_has_audio_params(const TbdP4SoundData &sound) {
  for (uint8_t i = 0; i < TBD_P4_AUDIO_PARAM_COUNT; i++) {
    if (sound.audio_params.params[i].type != TBD_P4_PARAM_TYPE_NONE) {
      return true;
    }
  }
  return false;
}

void cache_default_sound(const TbdP4SoundData &sound) {
  if (sound.p4_track_index >= kP4SoundTrackCount) {
    debug_p4_sound_summary("cache_oob", sound);
    return;
  }
  debug_p4_sound_summary("cache", sound);
  p4_default_sounds[sound.p4_track_index] = sound;
  p4_default_sound_valid[sound.p4_track_index] = true;
}

bool parse_p4_track_defaults(const char *json, P4BootPreset *presets,
                             size_t preset_count, char *kit_id,
                             size_t kit_id_len) {
  if (json == nullptr || presets == nullptr || preset_count == 0) {
    return false;
  }

  const char *end = json + strlen(json);
  if (kit_id != nullptr && kit_id_len > 0) {
    char parsed_kit[32];
    if (read_json_string(json, end, "kit", parsed_kit,
                         sizeof(parsed_kit))) {
      copy_fixed_string(kit_id, kit_id_len, parsed_kit);
    }
  }

  const char *tracks = find_json_value(json, end, "tracks");
  if (tracks == nullptr || tracks >= end || *tracks != '[') {
    return false;
  }

  bool parsed_any = false;
  const char *p = tracks + 1;
  while (p < end) {
    p = skip_json_ws(p, end);
    if (p >= end || *p == ']') {
      break;
    }
    if (*p != '{') {
      p++;
      continue;
    }

    const char *obj_end = find_json_object_end(p, end);
    if (obj_end == nullptr) {
      break;
    }

    long track_index = -1;
    char preset_id[TBD_PRESET_ID_LEN];
    if (read_json_int(p, obj_end, "index", &track_index) &&
        read_json_string(p, obj_end, "preset", preset_id,
                         sizeof(preset_id)) &&
        track_index >= 0 && (size_t)track_index < preset_count &&
        preset_id[0] != '\0') {
      auto &preset = presets[track_index];
      preset.track_index = (uint8_t)track_index;
      copy_preset_id(preset.preset_id, preset_id);

      long sample_bank = kP4DefaultRomBank;
      long sample_slice = kP4DefaultSampleSlice;
      if (read_json_int(p, obj_end, "sampleBank", &sample_bank) &&
          sample_bank >= 0 && sample_bank <= 0xFE) {
        preset.rom_bank = (uint8_t)sample_bank;
      }
      if (read_json_int(p, obj_end, "sampleSlice", &sample_slice)) {
        preset.sample_slice = (int32_t)sample_slice;
      }
      parsed_any = true;
    }

    p = obj_end;
  }

  return parsed_any;
}

uint8_t find_p4_kit_index_for_id(const char *kit_json, const char *kit_id) {
  if (kit_json == nullptr || kit_id == nullptr || kit_id[0] == '\0') {
    return 0;
  }

  const char *end = kit_json + strlen(kit_json);
  const char *kits = find_json_value(kit_json, end, "kits");
  if (kits == nullptr || kits >= end || *kits != '[') {
    return 0;
  }

  uint8_t index = 0;
  const char *p = kits + 1;
  while (p < end) {
    p = skip_json_ws(p, end);
    if (p >= end || *p == ']') {
      break;
    }
    if (*p != '{') {
      p++;
      continue;
    }

    const char *obj_end = find_json_object_end(p, end);
    if (obj_end == nullptr) {
      break;
    }

    char id[24];
    if (read_json_string(p, obj_end, "id", id, sizeof(id)) &&
        strcmp(id, kit_id) == 0) {
      return index;
    }
    if (index < 0xFF) {
      index++;
    }
    p = obj_end;
  }

  return 0;
}

class TbdP4DiagOverlay : public LightPage {
public:
  virtual void display() override {
    TbdP4RealtimeStats stats;
    tbd_p4_realtime.get_stats(stats);

    constexpr uint8_t y = 32;
    oled_display.fillRect(0, y, 128, 32, BLACK);
    oled_display.drawFastHLine(0, y, 128, WHITE);

    char line[22];
    const uint8_t ui_slot = TBD.ui_device_idx();
    const uint8_t display_slot =
        ui_slot == kTbdUiSlotSecondary ? 2 : 1;
    oled_display.setCursor(0, y + 2);
    snprintf(line, sizeof(line), "T%u A%uS%uR%uD%uX%u p%uc%u",
             display_slot,
             stats.p4_alive ? 1 : 0,
             stats.p4_sync_seen ? 1 : 0,
             stats.p4_ready_pin ? 1 : 0,
             TBD.p4_defaults_loaded() ? 1 : 0,
             stats.extended_response_seen ? 1 : 0,
             stats.request_prepared ? 1 : 0,
             stats.can_prepare_request ? 1 : 0);
    oled_display.println(line);

    snprintf(line, sizeof(line), "F%lu/%lu E%lu L%lu",
             (unsigned long)(stats.tx_frames % 1000),
             (unsigned long)(stats.rx_frames % 1000),
             (unsigned long)(stats.error_count % 1000),
             (unsigned long)(stats.length_errors % 1000));
    oled_display.println(line);

    snprintf(line, sizeof(line), "P%lu B%lu D%lu U%lu",
             (unsigned long)(stats.p4_not_ready_count % 1000),
             (unsigned long)(stats.dma_busy_count % 1000),
             (unsigned long)(stats.dma_timeout_count % 1000),
             (unsigned long)(stats.dma_unavailable_count % 1000));
    oled_display.println(line);

    snprintf(line, sizeof(line), "G%lu Q%lu M%lu N%lu",
             (unsigned long)(p4_trigger_count % 1000),
             (unsigned long)(stats.queued_tx_midi_bytes % 1000),
             (unsigned long)(stats.tx_midi_bytes % 1000),
             (unsigned long)(stats.tx_note_on_messages % 1000));
    oled_display.println(line);
  }
};

TbdP4DiagOverlay tbd_p4_diag_overlay;

bool tbd_ui_slot_configured(uint8_t device_idx) {
  if (device_idx == kTbdUiSlotPrimary) {
    return mcl_cfg.grid_x_device == GRID_X_DEVICE_TBD;
  }
  if (device_idx == kTbdUiSlotSecondary) {
    return mcl_cfg.grid_y_device == GRID_Y_DEVICE_TBD;
  }
  return false;
}

uint8_t tbd_grid_track_type(uint8_t device_idx) {
  if (device_idx == kTbdUiSlotPrimary) {
    return TBD_TRACK_TYPE;
  }
  if (device_idx == kTbdUiSlotSecondary) {
    return TBD_MIDI_TRACK_TYPE;
  }
  return EMPTY_TRACK_TYPE;
}

void cleanup_tbd_grid_devices(uint8_t device_idx) {
  uint8_t track_type = tbd_grid_track_type(device_idx);
  if (track_type == EMPTY_TRACK_TYPE) {
    return;
  }
  for (uint8_t grid_idx = 0; grid_idx < NUM_GRIDS; grid_idx++) {
    for (uint8_t track_idx = 0; track_idx < GRID_WIDTH; track_idx++) {
      GridDeviceTrack &track = proj.grids[grid_idx].tracks[track_idx];
      if (track.device_idx == device_idx && track.track_type == track_type) {
        track.init();
      }
    }
  }
}

bool tbd_ui_request_from_event(gui_event_t *event, uint8_t *device_idx) {
  if (event == nullptr || device_idx == nullptr || !EVENT_BUTTON(event)) {
    return false;
  }
  if (event->source == ButtonsClass::BUTTON2 &&
      (event->mask == EVENT_BUTTON_PRESSED ||
       event->mask == EVENT_BUTTON_RELEASED)) {
    *device_idx = kTbdUiSlotPrimary;
    return tbd_ui_slot_configured(*device_idx);
  }
  if (event->source == ButtonsClass::TBD_BUTTON_TR &&
      event->mask == EVENT_BUTTON_PRESSED) {
    *device_idx = kTbdUiSlotSecondary;
    return tbd_ui_slot_configured(*device_idx);
  }
  return false;
}

} // namespace

bool tbd_get_default_p4_sound(uint8_t p4_track_index,
                              TbdP4SoundData *sound) {
  if (sound == nullptr || p4_track_index >= kP4SoundTrackCount ||
      !p4_default_sound_valid[p4_track_index]) {
    return false;
  }

  *sound = p4_default_sounds[p4_track_index];
  return true;
}

bool tbd_hydrate_p4_sound(TbdP4SoundData &sound) {
  debug_p4_sound_summary("hydrate_enter", sound);
  if (sound.version != TBD_P4_SOUND_DATA_VERSION ||
      sound_has_audio_params(sound)) {
    const bool hydrated = sound_has_audio_params(sound);
#ifdef DEBUGMODE
    DEBUG_PRINT("tbd_init hydrate_skip result=");
    DEBUG_PRINTLN(hydrated ? 1 : 0);
#endif
    return hydrated;
  }

  int16_t values[kP4PresetValueCount];
  bool value_set[kP4PresetValueCount];
  memset(values, 0, sizeof(values));
  memset(value_set, 0, sizeof(value_set));

  tbd_p4_command.init();
  if (!tbd_p4_command.wait_ready(kP4PresetCommandTimeoutMs)) {
#ifdef DEBUGMODE
    DEBUG_PRINTLN("tbd_init hydrate wait_ready failed");
#endif
    return false;
  }

  if (sound.has_preset()) {
    char requested_preset[TBD_P4_ID_LEN];
    copy_fixed_string(requested_preset, sizeof(requested_preset),
                      sound.preset_id);
    const bool preset_ok = tbd_p4_command.get_macro_sound_preset(
        sound.preset_id, p4_track_defaults_json,
        sizeof(p4_track_defaults_json), kP4PresetCommandTimeoutMs);
    debug_p4_json_result("macro_preset", preset_ok, p4_track_defaults_json);
    if (!preset_ok) {
      return false;
    }
    const bool preset_parse_ok =
        parse_p4_preset_json(p4_track_defaults_json, sound, values, value_set,
                             kP4PresetValueCount);
#ifdef DEBUGMODE
    DEBUG_PRINT("tbd_init hydrate preset_parse=");
    DEBUG_PRINT(preset_parse_ok ? 1 : 0);
    DEBUG_PRINT(" req=");
    DEBUG_PRINT(requested_preset);
    DEBUG_PRINT(" got=");
    DEBUG_PRINT(sound.preset_id);
    DEBUG_PRINT(" macro=");
    DEBUG_PRINTLN(sound.macro_id);
    if (preset_parse_ok && strcmp(requested_preset, sound.preset_id) != 0) {
      DEBUG_PRINT("tbd_init hydrate preset_id_mismatch req=");
      DEBUG_PRINT(requested_preset);
      DEBUG_PRINT(" got=");
      DEBUG_PRINTLN(sound.preset_id);
    }
#endif
  }

  if (!sound.has_macro()) {
#ifdef DEBUGMODE
    DEBUG_PRINTLN("tbd_init hydrate no macro id");
#endif
    return false;
  }

  char requested_macro[TBD_P4_ID_LEN];
  copy_fixed_string(requested_macro, sizeof(requested_macro), sound.macro_id);
  const bool macro_ok = tbd_p4_command.get_macro_definition(
      sound.macro_id, p4_track_defaults_json, sizeof(p4_track_defaults_json),
      kP4PresetCommandTimeoutMs);
  debug_p4_json_result("macro_def", macro_ok, p4_track_defaults_json);
  if (!macro_ok) {
    return false;
  }

  const bool macro_parse_ok =
      parse_p4_macro_definition(p4_track_defaults_json, values, value_set,
                                kP4PresetValueCount, sound);
#ifdef DEBUGMODE
  DEBUG_PRINT("tbd_init hydrate macro_parse=");
  DEBUG_PRINT(macro_parse_ok ? 1 : 0);
  DEBUG_PRINT(" req=");
  DEBUG_PRINT(requested_macro);
  DEBUG_PRINT(" got=");
  DEBUG_PRINT(sound.macro_id);
  DEBUG_PRINT(" machine=");
  DEBUG_PRINTLN(sound.machine_id);
  if (macro_parse_ok && strcmp(requested_macro, sound.macro_id) != 0) {
    DEBUG_PRINT("tbd_init hydrate macro_id_mismatch req=");
    DEBUG_PRINT(requested_macro);
    DEBUG_PRINT(" got=");
    DEBUG_PRINTLN(sound.macro_id);
  }
#endif
  debug_p4_sound_summary(macro_parse_ok ? "hydrate_ok" : "hydrate_fail", sound);
  return macro_parse_ok;
}

bool TbdDevice::get_default_p4_sound(uint8_t p4_track_index,
                                     TbdP4SoundData *sound) const {
  return tbd_get_default_p4_sound(p4_track_index, sound);
}

bool TbdDevice::hydrate_p4_sound(TbdP4SoundData &sound) {
  return tbd_hydrate_p4_sound(sound);
}

void tbd_p4_send_param_value(MidiUartClass *uart, uint8_t midi_channel,
                             const TbdP4ParamDescriptor &param,
                             int16_t value) {
  if (uart == nullptr || midi_channel >= 16 || !param.is_sendable()) {
    return;
  }

  if (param.ctrl_type == TBD_P4_CTRLTYPE_CC) {
    if (value < 0) value = 0;
    if (value > 127) value = 127;
    uart->sendCC(midi_channel, param.ctrl, (uint8_t)value);
    return;
  }

  if (param.ctrl_type == TBD_P4_CTRLTYPE_NRPM) {
    if (value < 0) value = 0;
    if (value > 0x3FFF) value = 0x3FFF;
    const uint16_t nrpn_value = (uint16_t)value;
    uart->sendCC(midi_channel, 98, param.ctrl & 0x7F);
    uart->sendCC(midi_channel, 99, (param.ctrl >> 7) & 0x7F);
    uart->sendCC(midi_channel, 38, nrpn_value & 0x7F);
    uart->sendCC(midi_channel, 6, (nrpn_value >> 7) & 0x7F);
  }
}

void tbd_p4_send_sound_state(const TbdP4SoundData &sound) {
  if (TBD.uart == nullptr || sound.midi_channel >= 16) {
    return;
  }

  tbd_p4_realtime.set_active_track(sound.p4_track_index);
  for (uint8_t i = 0; i < TBD_P4_AUDIO_PARAM_COUNT; i++) {
    const TbdP4ParamDescriptor &param = sound.audio_params.params[i];
    tbd_p4_send_param_value(TBD.uart, sound.midi_channel, param, param.value);
  }
  for (uint8_t i = 0; i < TBD_P4_MIXER_PARAM_COUNT; i++) {
    const TbdP4ParamDescriptor &param = sound.mixer_params.params[i];
    tbd_p4_send_param_value(TBD.uart, sound.midi_channel, param, param.value);
  }
}

void tbd_p4_send_sound_mixer_state(const TbdP4SoundData &sound) {
  if (TBD.uart == nullptr || sound.midi_channel >= 16) {
    return;
  }

  tbd_p4_realtime.set_active_track(sound.p4_track_index);
  for (uint8_t i = 0; i < TBD_P4_MIXER_PARAM_COUNT; i++) {
    const TbdP4ParamDescriptor &param = sound.mixer_params.params[i];
    tbd_p4_send_param_value(TBD.uart, sound.midi_channel, param, param.value);
  }
}

uint8_t tbd_p4_driver_param_page_count() {
  ensure_p4_driver_params_initialized();
  return p4_driver_params.num_pages;
}

TbdP4ParamDescriptor *tbd_p4_driver_param(uint8_t index) {
  ensure_p4_driver_params_initialized();
  if (index >= TBD_P4_AUDIO_PARAM_COUNT) {
    return nullptr;
  }
  return &p4_driver_params.params[index];
}

void tbd_p4_send_driver_param(uint8_t index) {
  TbdP4ParamDescriptor *param = tbd_p4_driver_param(index);
  if (TBD.uart == nullptr || param == nullptr || !param->is_sendable()) {
    return;
  }
  tbd_p4_send_param_value(TBD.uart, kP4DriverMidiChannel, *param, param->value);
}

void tbd_p4_send_driver_params() {
  ensure_p4_driver_params_initialized();
  for (uint8_t i = 0; i < TBD_P4_AUDIO_PARAM_COUNT; i++) {
    tbd_p4_send_driver_param(i);
  }
}

TbdDevice::TbdDevice() : MidiDevice(&MidiP4, "TBD", DEVICE_MIDI, false) {
  port = UARTP4_PORT;
}

bool TbdDevice::supports_capability(MidiDeviceCapability capability) const {
  switch (capability) {
  case MidiDeviceCapability::MdTrigInterface:
    return true;
  case MidiDeviceCapability::MdSequencerTracks:
  case MidiDeviceCapability::MdPatternImport:
    return false;
  }
  return false;
}

void TbdDevice::muteTrack(uint8_t track, bool mute, MidiUartClass *uart_) {
  (void)uart_;
  tbd_p4_command.set_track_mute(track, mute);
}

void TbdDevice::triggerTrack(uint8_t track, uint8_t velocity,
                             MidiUartClass *uart_) {
  p4_trigger_count++;
  if (track >= mcl_seq.num_tbd_tracks) {
    return;
  }
  MidiUartClass *port = uart_ ? uart_ : uart;
  tbd_p4_realtime.set_active_track(
      mcl_seq.tbd_tracks[track].p4_sound.p4_track_index);
  mcl_seq.tbd_tracks[track].trigger(velocity, port);
}

DeviceMixerCapability *TbdDevice::mixer() {
  static TbdMixerCapability capability(*this);
  return &capability;
}

DeviceStepTrackCapability *TbdDevice::step_tracks() {
  static TbdStepTrackCapability capability(*this);
  return &capability;
}

DeviceParamCapability *TbdDevice::params() {
  static TbdParamCapability capability(*this);
  return &capability;
}

bool TbdStepTrackCapability::available(const DeviceContext &ctx) const {
  return ctx.device_idx() == kTbdUiSlotPrimary &&
         mcl_cfg.grid_x_device == GRID_X_DEVICE_TBD;
}

uint8_t TbdStepTrackCapability::track_count(const DeviceContext &ctx) const {
  return available(ctx) ? mcl_seq.num_tbd_tracks : 0;
}

SeqStepTrackRef TbdStepTrackCapability::track(const DeviceContext &ctx,
                                              uint8_t track_idx) const {
  (void)ctx;
  if (track_idx >= mcl_seq.num_tbd_tracks) {
    track_idx = 0;
  }
  return SeqStepTrackRef(mcl_seq.tbd_tracks[track_idx]);
}

SeqStepTrackRef TbdStepTrackCapability::active_track(
    const DeviceContext &ctx) const {
  return track(ctx, last_md_track);
}

bool TbdMixerCapability::param(const DeviceContext &ctx, uint8_t track,
                               uint8_t param_idx,
                               MidiDeviceMixerParam *param) {
  if (param == nullptr) {
    return false;
  }
  TbdP4SoundData *sound = p4_sound_for_mixer(ctx.device_idx(), track);
  if (sound == nullptr || param_idx >= TBD_P4_MIXER_PARAM_COUNT) {
    return false;
  }

  TbdP4ParamDescriptor &desc = sound->mixer_params.params[param_idx];
  if (!desc.is_visible()) {
    return false;
  }

  param->set_value(desc.value, desc.min_value, desc.max_value);
  param->set_metadata(desc.shortname, desc.type, desc.is_sendable());
  return true;
}

bool TbdMixerCapability::set_param(const DeviceContext &ctx, uint8_t track,
                                   uint8_t param_idx, int16_t value,
                                   bool send) {
  TbdP4SoundData *sound = p4_sound_for_mixer(ctx.device_idx(), track);
  if (sound == nullptr || param_idx >= TBD_P4_MIXER_PARAM_COUNT) {
    return false;
  }

  TbdP4ParamDescriptor &desc = sound->mixer_params.params[param_idx];
  if (!desc.is_visible()) {
    return false;
  }
  if (value < desc.min_value) value = desc.min_value;
  if (value > desc.max_value) value = desc.max_value;
  desc.value = value;

  MidiUartClass *port = tbd().uart;
  if (!send || !desc.is_sendable() || port == nullptr) {
    return true;
  }

  tbd_p4_realtime.set_active_track(sound->p4_track_index);
  if (desc.ctrl_type == TBD_P4_CTRLTYPE_CC) {
    tbd_p4_send_param_value(port, sound->midi_channel, desc, value);
  } else if (desc.ctrl_type == TBD_P4_CTRLTYPE_NRPM) {
    tbd_p4_send_param_value(port, sound->midi_channel, desc, value);
  }
  return true;
}

bool TbdMixerCapability::parse_cc(const DeviceContext &ctx, uint8_t channel,
                                  uint8_t cc, uint8_t *track,
                                  uint8_t *param) const {
  if (track == nullptr || param == nullptr) {
    return false;
  }
  uint8_t count = track_count(ctx);
  for (uint8_t t = 0; t < count; t++) {
    TbdP4SoundData *sound = p4_sound_for_mixer(ctx.device_idx(), t);
    if (sound == nullptr || sound->midi_channel != channel) {
      continue;
    }
    for (uint8_t p = 0; p < TBD_P4_MIXER_PARAM_COUNT; p++) {
      TbdP4ParamDescriptor &desc = sound->mixer_params.params[p];
      if (desc.is_visible() && desc.ctrl_type == TBD_P4_CTRLTYPE_CC &&
          desc.ctrl == cc) {
        *track = t;
        *param = p;
        return true;
      }
    }
  }
  return false;
}

void TbdMixerCapability::update_from_cc(const DeviceContext &ctx, uint8_t track,
                                        uint8_t param, int16_t value) {
  TbdP4SoundData *sound = p4_sound_for_mixer(ctx.device_idx(), track);
  if (sound == nullptr || param >= TBD_P4_MIXER_PARAM_COUNT) {
    return;
  }
  TbdP4ParamDescriptor &desc = sound->mixer_params.params[param];
  if (!desc.is_visible()) {
    return;
  }
  desc.value = p4_u7_to_value(desc, (uint8_t)value);
}

uint8_t TbdParamCapability::target_count(const DeviceContext &ctx) const {
  uint8_t grid_idx = ctx.device_idx();
  if (grid_idx == kTbdUiSlotPrimary) {
    return mcl_cfg.grid_x_device == GRID_X_DEVICE_TBD ? mcl_seq.num_tbd_tracks
                                                       : 0;
  }
  if (grid_idx == kTbdUiSlotSecondary) {
    return mcl_cfg.grid_y_device == GRID_Y_DEVICE_TBD ? mcl_seq.num_midi_tracks
                                                       : 0;
  }
  return 0;
}

uint8_t TbdParamCapability::param_count(const DeviceContext &ctx,
                                        uint8_t target) const {
  return p4_param_count_for_mod(ctx.device_idx(), target);
}

bool TbdParamCapability::target_label(const DeviceContext &ctx, uint8_t target,
                                      char *out, uint8_t len) const {
  if (out == nullptr || len == 0 || target >= target_count(ctx)) {
    return false;
  }
  TbdP4SoundData *sound = p4_sound_for_mixer(ctx.device_idx(), target);
  if (sound != nullptr && tbd_p4_copy_sound_notice(*sound, out, len)) {
    return true;
  }
  return tbd_p4_copy_track_label(target, out, len);
}

bool TbdParamCapability::param_label(const DeviceContext &ctx, uint8_t target,
                                     uint8_t param, char *out, uint8_t len) {
  TbdP4ParamDescriptor *desc =
      p4_param_for_mod(ctx.device_idx(), target, param);
  return desc != nullptr && tbd_p4_copy_param_label(*desc, out, len);
}

bool TbdParamCapability::get_param(const DeviceContext &ctx, uint8_t target,
                                   uint8_t param, uint8_t *value) {
  if (value == nullptr) {
    return false;
  }
  TbdP4ParamDescriptor *desc =
      p4_param_for_mod(ctx.device_idx(), target, param);
  if (desc == nullptr) {
    return false;
  }
  *value = p4_value_to_u7(*desc);
  return true;
}

bool TbdParamCapability::set_param(const DeviceContext &ctx, uint8_t target,
                                   uint8_t param, uint8_t value,
                                   MidiUartClass *uart_) {
  uint8_t grid_idx = ctx.device_idx();
  TbdP4SoundData *sound = p4_sound_for_mixer(grid_idx, target);
  TbdP4ParamDescriptor *desc = p4_param_for_mod(grid_idx, target, param);
  if (sound == nullptr || desc == nullptr) {
    return false;
  }
  int16_t scaled = p4_u7_to_value(*desc, value);
  MidiUartClass *port = uart_ ? uart_ : device_.uart;
  tbd_p4_send_param_value(port, sound->midi_channel, *desc, scaled);
  return true;
}

uint8_t TbdParamCapability::sequencer_lock_param_count(const DeviceContext &ctx,
                                                       uint8_t target) const {
  return p4_sound_for_mixer(ctx.device_idx(), target) != nullptr
             ? TBD_P4_LOCK_PARAM_COUNT
             : 0;
}

bool TbdParamCapability::sequencer_lock_param_info(const DeviceContext &ctx,
                                                   uint8_t target,
                                                   uint8_t param,
                                                   MidiDeviceParamInfo *info) {
  if (info == nullptr ||
      param >= sequencer_lock_param_count(ctx, target)) {
    return false;
  }
  *info = MidiDeviceParamInfo();
  TbdP4SoundData *sound = p4_sound_for_mixer(ctx.device_idx(), target);
  if (sound == nullptr) {
    return false;
  }

  if (param == TBD_P4_LOCK_NOTE_PARAM) {
    if (!tbd_p4_sound_uses_step_note(*sound)) {
      return false;
    }
    info->active = true;
    info->sendable = true;
    info->param_id = param;
    info->ctrl = param;
    info->default_value = TBD_P4_DEFAULT_STEP_NOTE;
    info->current_value = TBD_P4_DEFAULT_STEP_NOTE;
    return true;
  }

  const TbdP4ParamDescriptor *desc = tbd_p4_sound_param_for_lock(*sound, param);
  if (desc == nullptr || !desc->is_visible()) {
    return false;
  }
  info->active = true;
  info->p4_param = true;
  info->sendable = desc->is_sendable();
  info->nrpn = desc->ctrl_type == TBD_P4_CTRLTYPE_NRPM;
  info->macro = desc->is_macro();
  info->param_id = param;
  info->ctrl = desc->ctrl;
  info->ctrl_type = desc->ctrl_type;
  info->resolution = desc->resolution;
  info->min_value = desc->min_value;
  info->max_value = desc->max_value;
  info->default_value = p4_value_to_u7(*desc, desc->default_value);
  info->current_value = p4_value_to_u7(*desc);
  return true;
}

bool TbdParamCapability::sequencer_lock_param_label(const DeviceContext &ctx,
                                                    uint8_t target,
                                                    uint8_t param, char *out,
                                                    uint8_t len) {
  if (out == nullptr || len == 0) {
    return false;
  }
  out[0] = '\0';
  MidiDeviceParamInfo info;
  if (!sequencer_lock_param_info(ctx, target, param, &info)) {
    return false;
  }
  if (param == TBD_P4_LOCK_NOTE_PARAM) {
    return tbd_p4_copy_param_label_literal("NOT", out, len, 3);
  }

  TbdP4SoundData *sound = p4_sound_for_mixer(ctx.device_idx(), target);
  const TbdP4ParamDescriptor *desc =
      sound != nullptr ? tbd_p4_sound_param_for_lock(*sound, param) : nullptr;
  if (desc != nullptr && desc->shortname[0] != '\0' &&
      tbd_p4_copy_param_label(*desc, out, len)) {
    return true;
  }
  copy_param_number_label(info.nrpn ? 'N' : 'P', param, out, len);
  return true;
}

bool TbdParamCapability::sequencer_uses_step_pitch(const DeviceContext &ctx,
                                                   uint8_t target) const {
  TbdP4SoundData *sound = p4_sound_for_mixer(ctx.device_idx(), target);
  return sound != nullptr && tbd_p4_sound_uses_step_note(*sound);
}

uint8_t TbdParamCapability::sequencer_pitch_lock_param(const DeviceContext &ctx,
                                                       uint8_t target) const {
  (void)ctx;
  (void)target;
  return TBD_P4_LOCK_NOTE_PARAM;
}

void TbdMixerCapability::mute_track(const DeviceContext &ctx, uint8_t track,
                                    bool mute, MidiUartClass *uart_) {
  (void)uart_;
  TbdP4SoundData *sound = p4_sound_for_mixer(ctx.device_idx(), track);
  if (sound == nullptr) {
    return;
  }
  tbd_p4_command.set_track_mute(sound->p4_track_index, mute);
}

void TbdMixerCapability::set_record_mutes(const DeviceContext &ctx,
                                          uint8_t track, bool state,
                                          bool clear) {
  uint8_t grid_idx = ctx.device_idx();
  SeqTrack *seq_track = seq_track_for_mixer(grid_idx, track);
  if (seq_track == nullptr) {
    return;
  }
  seq_track->record_mutes = state;
  if (clear && grid_idx == kTbdUiSlotPrimary) {
    mcl_seq.tbd_tracks[track].clear_mute();
  } else if (clear && grid_idx == kTbdUiSlotSecondary) {
    mcl_seq.midi_tracks[track].clear_mute();
  }
}

bool TbdDevice::probe() {
  connected = true;
  return true;
}

void TbdDevice::disconnect(uint8_t device_idx) {
  cleanup_tbd_grid_devices(device_idx);
  if (device_idx < 2) {
    grid_devices_initialized_[device_idx] = false;
  }
  p4_defaults_init_in_progress_ = false;
  connected = grid_devices_initialized_[0] || grid_devices_initialized_[1];
}

void TbdDevice::on_connection(uint8_t device_idx) {
  (void)device_idx;
  port = UARTP4_PORT;
  midi = &MidiP4;
  uart = MidiP4.uart;
  connected = true;
  cleanup_tbd_grid_devices(0);
  cleanup_tbd_grid_devices(1);
  grid_devices_initialized_[0] = false;
  grid_devices_initialized_[1] = false;
  p4_defaults_loaded_ = false;
  p4_defaults_init_failed_ = false;
  p4_defaults_init_in_progress_ = false;
  p4_defaults_last_attempt_ms_ = 0;
  if (load_default_p4_presets(true)) {
    apply_runtime_p4_defaults();
  }
  sync_grid_devices();
}

void TbdDevice::init_grid_devices(uint8_t device_idx) {
  if (device_idx < 2 && grid_devices_initialized_[device_idx]) {
    return;
  }

  GridDeviceTrack gdt;

#if defined(PLATFORM_TBD)
  if (device_idx == 0) {
    for (uint8_t i = 0; i < mcl_seq.num_tbd_tracks; i++) {
      tbd_ensure_step_sound_default(mcl_seq.tbd_tracks[i].p4_sound, i);
      gdt.init(TBD_TRACK_TYPE, GROUP_DEV, device_idx,
               &(mcl_seq.tbd_tracks[i]));
      add_track_to_grid(0, i, &gdt);
    }
    grid_devices_initialized_[0] = true;
    return;
  }
#endif

  if (device_idx == 1) {
    for (uint8_t i = 0; i < NUM_EXT_TRACKS; i++) {
      tbd_ensure_midi_sound_default(mcl_seq.midi_tracks[i].p4_sound, i);
      mcl_seq.midi_tracks[i].set_channel(
          mcl_seq.midi_tracks[i].p4_sound.midi_channel);
      gdt.init(TBD_MIDI_TRACK_TYPE, GROUP_DEV, device_idx,
               &(mcl_seq.midi_tracks[i]));
      add_track_to_grid(1, i, &gdt);
    }
    grid_devices_initialized_[1] = true;
  }
}

void TbdDevice::sync_grid_devices() {
  if (mcl_cfg.grid_x_device == GRID_X_DEVICE_TBD) {
    init_grid_devices(0);
  } else if (grid_devices_initialized_[0]) {
    cleanup_tbd_grid_devices(0);
    grid_devices_initialized_[0] = false;
  }

  if (mcl_cfg.grid_y_device == GRID_Y_DEVICE_TBD) {
    init_grid_devices(1);
  } else if (grid_devices_initialized_[1]) {
    cleanup_tbd_grid_devices(1);
    grid_devices_initialized_[1] = false;
  }
}

void TbdDevice::apply_runtime_p4_defaults() {
#ifdef DEBUGMODE
  DEBUG_PRINTLN("tbd_init apply_runtime_defaults begin");
#endif
  for (uint8_t i = 0; i < mcl_seq.num_tbd_tracks; i++) {
#ifdef DEBUGMODE
    DEBUG_PRINT("tbd_init runtime_tbd slot=");
    DEBUG_PRINTLN((unsigned)i);
#endif
    debug_p4_sound_summary("runtime_tbd_before",
                           mcl_seq.tbd_tracks[i].p4_sound);
    tbd_ensure_step_sound_default(mcl_seq.tbd_tracks[i].p4_sound, i);
    tbd_p4_set_track_length(
        mcl_seq.tbd_tracks[i].p4_sound,
        mcl_seq.tbd_tracks[i].length
            ? mcl_seq.tbd_tracks[i].length
            : TBD_P4_DEFAULT_TRACK_LENGTH);
    debug_p4_sound_summary("runtime_tbd_after",
                           mcl_seq.tbd_tracks[i].p4_sound);
  }

  for (uint8_t i = 0; i < mcl_seq.num_midi_tracks; i++) {
#ifdef DEBUGMODE
    DEBUG_PRINT("tbd_init runtime_midi slot=");
    DEBUG_PRINTLN((unsigned)i);
#endif
    debug_p4_sound_summary("runtime_midi_before",
                           mcl_seq.midi_tracks[i].p4_sound);
    tbd_ensure_midi_sound_default(mcl_seq.midi_tracks[i].p4_sound, i);
    tbd_p4_set_track_length(
        mcl_seq.midi_tracks[i].p4_sound,
        mcl_seq.midi_tracks[i].seq_data.length
            ? mcl_seq.midi_tracks[i].seq_data.length
            : TBD_P4_DEFAULT_TRACK_LENGTH);
    mcl_seq.midi_tracks[i].set_channel(
        mcl_seq.midi_tracks[i].p4_sound.midi_channel);
    debug_p4_sound_summary("runtime_midi_after",
                           mcl_seq.midi_tracks[i].p4_sound);
  }
#ifdef DEBUGMODE
  DEBUG_PRINTLN("tbd_init apply_runtime_defaults end");
#endif
}

void TbdDevice::sync_active_p4_track() {
  TbdP4SoundData *sound = tbd_ui_mode.active_sound();
  if (sound != nullptr) {
    tbd_p4_realtime.set_active_track(sound->p4_track_index);
    return;
  }

#ifdef EXT_TRACKS
  if (mcl.currentPage() == SEQ_EXTSTEP_PAGE &&
      mcl_cfg.grid_y_device == GRID_Y_DEVICE_TBD &&
      last_ext_track < mcl_seq.num_midi_tracks) {
    tbd_p4_realtime.set_active_track(
        mcl_seq.midi_tracks[last_ext_track].p4_sound.p4_track_index);
    return;
  }
#endif

  if (mcl_cfg.grid_x_device == GRID_X_DEVICE_TBD &&
      last_md_track < mcl_seq.num_tbd_tracks) {
    tbd_p4_realtime.set_active_track(
        mcl_seq.tbd_tracks[last_md_track].p4_sound.p4_track_index);
    return;
  }

#ifdef EXT_TRACKS
  if (mcl_cfg.grid_y_device == GRID_Y_DEVICE_TBD &&
      last_ext_track < mcl_seq.num_midi_tracks) {
    tbd_p4_realtime.set_active_track(
        mcl_seq.midi_tracks[last_ext_track].p4_sound.p4_track_index);
  }
#endif
}

bool TbdDevice::load_default_p4_presets(bool show_progress) {
  if (p4_defaults_loaded_) {
    return true;
  }
  if (p4_defaults_init_in_progress_) {
#ifdef DEBUGMODE
    DEBUG_PRINTLN("tbd_init already in progress");
#endif
    return false;
  }
  if (p4_defaults_init_failed_) {
#ifdef DEBUGMODE
    DEBUG_PRINT("tbd_init previous failure stage=");
    DEBUG_PRINT((unsigned)p4_boot_fail_stage);
    DEBUG_PRINT(" request=");
    DEBUG_PRINTLN((unsigned)p4_boot_fail_request);
#endif
    return false;
  }

  auto set_stage = [](uint8_t stage) {
    p4_boot_stage = stage;
#ifdef DEBUGMODE
    DEBUG_PRINT("tbd_init stage ");
    DEBUG_PRINTLN((unsigned)stage);
#endif
  };
  auto fail = [this](uint8_t stage, uint8_t request) {
    p4_boot_stage = stage;
    p4_boot_fail_stage = stage;
    p4_boot_fail_request = request;
    p4_defaults_init_failed_ = true;
    p4_defaults_init_in_progress_ = false;
#ifdef DEBUGMODE
    DEBUG_PRINT("tbd_init FAIL stage=");
    DEBUG_PRINT((unsigned)stage);
    DEBUG_PRINT(" request=");
    DEBUG_PRINTLN((unsigned)request);
#endif
    return false;
  };
  auto advance_progress = [show_progress](uint8_t &progress) {
    if (show_progress) {
      draw_p4_init_progress(++progress);
    }
  };
  auto finish_progress = [show_progress]() {
    if (show_progress) {
      draw_p4_init_progress(kP4InitProgressMax);
    }
  };
  uint8_t progress = 0;
  p4_boot_fail_stage = 0;
  p4_boot_fail_request = 0;
  p4_boot_loaded_tracks = 0;
  set_stage(1);
  advance_progress(progress);

  const uint32_t now = millis();
  if (p4_defaults_last_attempt_ms_ != 0 &&
      now - p4_defaults_last_attempt_ms_ < kP4PresetRetryMs) {
#ifdef DEBUGMODE
    DEBUG_PRINTLN("tbd_init retry suppressed");
#endif
    return false;
  }
  p4_defaults_last_attempt_ms_ = now;
  p4_defaults_init_in_progress_ = true;

  const uint32_t ready_timeout_ms =
      show_progress ? kP4PresetBootReadyMs : kP4PresetReadyProbeMs;

  tbd_p4_command.init();
  advance_progress(progress);
  set_stage(2);
  if (!tbd_p4_command.wait_ready(ready_timeout_ms)) {
    return fail(2, 0x00);
  }
  advance_progress(progress);

  if (show_progress) {
    set_stage(3);
    const bool reboot_ok = tbd_p4_command.reboot(kP4PresetCommandTimeoutMs);
#ifdef DEBUGMODE
    DEBUG_PRINT("tbd_init reboot=");
    DEBUG_PRINTLN(reboot_ok ? 1 : 0);
#endif
    delay(kP4BootSettleMs);
    const bool post_reboot_ready = tbd_p4_command.wait_ready(kP4PresetBootReadyMs);
#ifdef DEBUGMODE
    DEBUG_PRINT("tbd_init post_reboot_ready=");
    DEBUG_PRINTLN(post_reboot_ready ? 1 : 0);
#endif
    advance_progress(progress);
  }

  set_stage(4);
  const bool announce_ok = tbd_p4_command.announce_app(
      kP4ReferenceAppName, kP4AppFlags, kP4PresetCommandTimeoutMs);
#ifdef DEBUGMODE
  DEBUG_PRINT("tbd_init announce=");
  DEBUG_PRINTLN(announce_ok ? 1 : 0);
#endif
  if (!announce_ok) {
    return fail(4, 0xAB);
  }
  advance_progress(progress);

  set_stage(5);
  const bool version_ok = tbd_p4_command.report_pico_version(
      "MCL " VERSION_STR, kP4PresetCommandTimeoutMs);
#ifdef DEBUGMODE
  DEBUG_PRINT("tbd_init report_version=");
  DEBUG_PRINTLN(version_ok ? 1 : 0);
#endif
  delay(kP4BootSettleMs);
  advance_progress(progress);

  set_stage(6);
  debug_p4_active_plugin(kP4PresetCommandTimeoutMs);
  delay(kP4RackProcessorSettleMs);
  const bool rack_ready = tbd_p4_command.wait_ready(kP4PresetCommandTimeoutMs);
#ifdef DEBUGMODE
  DEBUG_PRINT("tbd_init rack_settle=");
  DEBUG_PRINTLN(rack_ready ? 1 : 0);
#endif
  if (!rack_ready) {
    return fail(6, 0x00);
  }
  advance_progress(progress);

  set_stage(7);
  reset_p4_boot_presets_to_fallback();
  advance_progress(progress);

  char kit_id[32] = "default";
  set_stage(8);
  const bool defaults_ok = tbd_p4_command.get_track_default_presets(
      p4_track_defaults_json, sizeof(p4_track_defaults_json), nullptr,
      kP4PresetCommandTimeoutMs);
  debug_p4_json_result("track_defaults", defaults_ok, p4_track_defaults_json);
  if (defaults_ok) {
    const bool parsed_defaults =
        parse_p4_track_defaults(p4_track_defaults_json, p4_boot_presets,
                                kP4SoundTrackCount, kit_id, sizeof(kit_id));
#ifdef DEBUGMODE
    DEBUG_PRINT("tbd_init track_defaults_parse=");
    DEBUG_PRINT(parsed_defaults ? 1 : 0);
    DEBUG_PRINT(" kit=");
    DEBUG_PRINTLN(kit_id);
#endif
    for (const auto &preset : p4_boot_presets) {
      debug_p4_boot_preset("default", preset);
    }
  }
  advance_progress(progress);

  set_stage(9);
  const bool kit_index_ok =
      kit_id[0] != '\0' &&
      tbd_p4_command.get_kit_index_json(
          p4_track_defaults_json, sizeof(p4_track_defaults_json),
          kP4PresetCommandTimeoutMs);
  debug_p4_json_result("kit_index", kit_index_ok, p4_track_defaults_json);
  if (kit_index_ok) {
    const uint8_t kit_index =
        find_p4_kit_index_for_id(p4_track_defaults_json, kit_id);
#ifdef DEBUGMODE
    DEBUG_PRINT("tbd_init kit id=");
    DEBUG_PRINT(kit_id);
    DEBUG_PRINT(" index=");
    DEBUG_PRINTLN((unsigned)kit_index);
#endif
    if (!activate_p4_sample_kit(kit_index, kP4PresetCommandTimeoutMs)) {
      return fail(9, 0x18);
    }
  }
  advance_progress(progress);

  set_stage(10);
  for (const auto &preset : p4_boot_presets) {
    if (preset.preset_id[0] == '\0') {
      continue;
    }
    advance_progress(progress);
    debug_p4_boot_preset("load", preset);
    tbd_update_track_default_from_p4(preset.track_index, preset.preset_id,
                                     preset.rom_bank, preset.sample_slice);

    const bool load_ok = tbd_p4_command.load_track_sound_preset(
        preset.track_index, preset.preset_id, preset.rom_bank,
        preset.sample_slice, kP4PresetCommandTimeoutMs);
#ifdef DEBUGMODE
    DEBUG_PRINT("tbd_init p4_load_sound p4=");
    DEBUG_PRINT((unsigned)preset.track_index);
    DEBUG_PRINT(" ok=");
    DEBUG_PRINTLN(load_ok ? 1 : 0);
#endif
    if (!load_ok) {
      return fail(10, 0xA4);
    }

    TbdP4SoundData sound;
    sound.clear();
    sound.p4_track_index = preset.track_index;
    sound.rom_bank = preset.rom_bank;
    sound.sample_slice = preset.sample_slice;
    copy_preset_id(sound.preset_id, preset.preset_id);
    tbd_init_p4_sound_runtime_defaults(sound);
    const bool hydrated = tbd_hydrate_p4_sound(sound);
#ifdef DEBUGMODE
    DEBUG_PRINT("tbd_init hydrate_result p4=");
    DEBUG_PRINT((unsigned)preset.track_index);
    DEBUG_PRINT(" ok=");
    DEBUG_PRINTLN(hydrated ? 1 : 0);
#endif
    cache_default_sound(sound);

    p4_boot_loaded_tracks++;
  }

  set_stage(11);

  set_stage(12);
#ifdef DEBUGMODE
  DEBUG_PRINTLN("tbd_init send driver and cached mixer state");
#endif
  tbd_p4_send_driver_params();
  for (uint8_t i = 0; i < kP4SoundTrackCount; i++) {
    if (!p4_default_sound_valid[i]) {
      continue;
    }
#ifdef DEBUGMODE
    DEBUG_PRINT("tbd_init send_mixer p4=");
    DEBUG_PRINT((unsigned)i);
    DEBUG_PRINT(" params=");
    DEBUG_PRINTLN((unsigned)count_p4_sendable_mixer_params(p4_default_sounds[i]));
#endif
    tbd_p4_send_sound_mixer_state(p4_default_sounds[i]);
    tbd_mark_p4_sound_applied(p4_default_sounds[i]);
  }

  p4_defaults_loaded_ = true;
  p4_defaults_init_failed_ = false;
  p4_defaults_init_in_progress_ = false;
  set_stage(13);
#ifdef DEBUGMODE
  DEBUG_PRINT("tbd_init complete tracks=");
  DEBUG_PRINTLN((unsigned)p4_boot_loaded_tracks);
#endif
  finish_progress();
  return true;
}

void TbdDevice::note_on(uint8_t note) {
  if (port != UARTP4_PORT || uart == nullptr) return;
  TbdP4SoundData *sound = active_p4_sound_for_note();
  if (sound == nullptr || sound->midi_channel >= 16) return;

  note_off();
  active_note_ = note;
  active_note_channel_ = sound->midi_channel;
  tbd_p4_realtime.set_active_track(sound->p4_track_index);
  if (sound->note_cc >= 0 && sound->note_cc <= 127) {
    uart->sendCC(sound->midi_channel, (uint8_t)sound->note_cc, note);
  }
  uart->sendNoteOn(sound->midi_channel, note, 100);
}

void TbdDevice::note_off() {
  if (active_note_ == 255 || uart == nullptr) return;
  uart->sendNoteOff(active_note_channel_, active_note_);
  active_note_ = 255;
}

bool TbdDevice::enter_ui(gui_event_t *event) {
  if (port != UARTP4_PORT) return false;

  uint8_t device_idx = kTbdUiSlotNone;
  if (!tbd_ui_request_from_event(event, &device_idx)) {
    return false;
  }

  diag_active_ = false;
  ui_device_idx_ = device_idx;
  tbd_ui_mode.enter(device_idx);
  if (!tbd_ui_mode.is_active()) ui_device_idx_ = kTbdUiSlotNone;
  return true;
}

bool TbdDevice::enter_diag_ui(uint8_t device_idx) {
  if (port != UARTP4_PORT || !tbd_ui_slot_configured(device_idx)) {
    return false;
  }

  tbd_ui_mode.disable();
  diag_active_ = true;
  ui_device_idx_ = device_idx;
  GUI_hardware.led.set_tbd_driver_leds(device_idx == kTbdUiSlotPrimary,
                                       device_idx == kTbdUiSlotSecondary);
  GUI.setOverlay(&tbd_p4_diag_overlay);
  return true;
}

bool TbdDevice::select_ui_track(uint8_t track_idx) {
  if (diag_active_) return false;
  return tbd_ui_mode.select_track(track_idx);
}

bool TbdDevice::handle_ui_event(gui_event_t *event) {
  const bool arrow_trace =
      EVENT_BUTTON(event) &&
      event->source >= ButtonsClass::FUNC_BUTTON6 &&
      event->source <= ButtonsClass::FUNC_BUTTON9;
  if (arrow_trace) {
    DEBUG_PRINT("  TbdDevice::handle_ui_event diag=");
    DEBUG_PRINT((unsigned)diag_active_);
    DEBUG_PRINT(" tbd_ui_mode.active=");
    DEBUG_PRINTLN((unsigned)tbd_ui_mode.is_active());
  }
  if (diag_active_) {
    if (GUI.overlay != &tbd_p4_diag_overlay) {
      diag_active_ = false;
      ui_device_idx_ = kTbdUiSlotNone;
      GUI_hardware.led.set_tbd_driver_leds(false, false);
      return false;
    }
    if (EVENT_BUTTON(event) && event->mask == EVENT_BUTTON_PRESSED &&
        (event->source == ButtonsClass::BUTTON1 ||
         event->source == ButtonsClass::TBD_BUTTON_TR)) {
      exit_ui();
    }
    return true;
  }
  bool result = tbd_ui_mode.handle_event(event);
  if (arrow_trace) {
    DEBUG_PRINT("  tbd_ui_mode.handle_event returned ");
    DEBUG_PRINTLN((unsigned)result);
  }
  return result;
}

void TbdDevice::ui_loop() {
  if (!p4_defaults_loaded_ && load_default_p4_presets()) {
    apply_runtime_p4_defaults();
  }

  tbd_ui_mode.poll_encoders();
  sync_active_p4_track();
}

bool TbdDevice::is_ui_active() {
  return diag_active_ || tbd_ui_mode.is_active();
}

bool TbdDevice::is_ui_collapsed() {
  return !diag_active_ && tbd_ui_mode.is_collapsed();
}

void TbdDevice::exit_ui() {
  note_off();
  diag_active_ = false;
  tbd_ui_mode.disable();
  if (GUI.overlay == &tbd_p4_diag_overlay) {
    GUI.clearOverlay();
  }
  ui_device_idx_ = kTbdUiSlotNone;
}

void TbdDevice::on_ui_slot_button(uint8_t slot, bool pressed) {
  (void)slot;
  if (diag_active_) return;
  tbd_ui_mode.handle_ui_slot_button(pressed);
}

#endif // PLATFORM_TBD
