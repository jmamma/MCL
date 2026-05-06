#include "TBD.h"

#ifdef PLATFORM_TBD

#include "MCL.h"
#include "MCLGUI.h"
#include "MCLSysConfig.h"
#include "MCLSeq.h"
#include "MidiDeviceGrid.h"
#include "MidiSetup.h"
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
};

constexpr uint32_t kP4PresetRetryMs = 2000;
constexpr uint32_t kP4PresetReadyProbeMs = 100;
constexpr uint32_t kP4PresetCommandTimeoutMs = 3000;
constexpr size_t kP4SoundTrackCount = 16;
constexpr size_t kP4TrackDefaultsJsonBytes = 8192;
constexpr size_t kP4PresetValueCount = 32;
constexpr uint8_t kP4DefaultRomBank = 0xFF;
constexpr int32_t kP4DefaultSampleSlice = -1;
constexpr uint8_t kTbdUiSlotPrimary = 0;
constexpr uint8_t kTbdUiSlotSecondary = 1;
constexpr uint8_t kTbdUiSlotNone = 255;

const P4BootPresetFallback kP4BootPresetFallbacks[] = {
    {0, "db-all-def"},    {1, "fmb-all-def"},
    {2, "ds-all-def"},    {3, "hh1-all-def"},
    {4, "rs-all-def"},    {5, "cl-all-def"},
    {6, "ro-all-def"},    {7, "ro-all-def"},
    {8, "td3-all-def"},   {9, "td3-all-def"},
    {10, "mo-all-def"},   {11, "wtosc-all-def"},
    {12, "ro-all-def"},   {13, "ro-all-def"},
    {14, "pp-all-def"},   {15, "inp-all-def"},
};

P4BootPreset p4_boot_presets[kP4SoundTrackCount];
TbdP4SoundData p4_default_sounds[kP4SoundTrackCount];
bool p4_default_sound_valid[kP4SoundTrackCount];
char p4_track_defaults_json[kP4TrackDefaultsJsonBytes];

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
    preset.rom_bank = kP4DefaultRomBank;
    preset.sample_slice = kP4DefaultSampleSlice;
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
    return;
  }
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
    read_json_string(json, end, "kit", kit_id, kit_id_len);
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
    snprintf(line, sizeof(line), "TBD%u A%u S%u R%u E%lu",
             display_slot,
             stats.p4_alive ? 1 : 0,
             stats.p4_sync_seen ? 1 : 0,
             stats.p4_ready_pin ? 1 : 0,
             (unsigned long)(stats.error_count % 1000));
    oled_display.println(line);

    snprintf(line, sizeof(line), "D%u WS%lu NR%lu F%lu",
             stats.dma_ready ? 1 : 0,
             (unsigned long)(stats.ws_sync_count % 1000),
             (unsigned long)(stats.p4_not_ready_count % 1000),
             (unsigned long)(stats.fingerprint_errors % 1000));
    oled_display.println(line);

    snprintf(line, sizeof(line), "L%lu C%lu SQ%lu MS%lu",
             (unsigned long)(stats.length_errors % 1000),
             (unsigned long)(stats.crc_errors % 1000),
             (unsigned long)(stats.sequence_errors % 1000),
             (unsigned long)(stats.missed_ws_sync_count % 1000));
    oled_display.println(line);

    snprintf(line, sizeof(line), "FR%lu/%lu TO%lu DU%lu",
             (unsigned long)(stats.tx_frames % 1000),
             (unsigned long)(stats.rx_frames % 1000),
             (unsigned long)(stats.dma_timeout_count % 1000),
             (unsigned long)(stats.dma_unavailable_count % 1000));
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

bool tbd_ui_request_from_event(gui_event_t *event, uint8_t *device_idx) {
  if (event == nullptr || device_idx == nullptr || !EVENT_BUTTON(event)) {
    return false;
  }
  if (event->source == ButtonsClass::BUTTON2 &&
      event->mask == EVENT_BUTTON_RELEASED) {
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
  if (sound.version != TBD_P4_SOUND_DATA_VERSION ||
      sound_has_audio_params(sound)) {
    return sound_has_audio_params(sound);
  }

  int16_t values[kP4PresetValueCount];
  bool value_set[kP4PresetValueCount];
  memset(values, 0, sizeof(values));
  memset(value_set, 0, sizeof(value_set));

  tbd_p4_command.init();
  if (!tbd_p4_command.wait_ready(kP4PresetReadyProbeMs)) {
    return false;
  }

  if (sound.has_preset()) {
    if (!tbd_p4_command.get_macro_sound_preset(
            sound.preset_id, p4_track_defaults_json,
            sizeof(p4_track_defaults_json), kP4PresetCommandTimeoutMs)) {
      return false;
    }
    parse_p4_preset_json(p4_track_defaults_json, sound, values, value_set,
                         kP4PresetValueCount);
  }

  if (!sound.has_macro()) {
    return false;
  }

  if (!tbd_p4_command.get_macro_definition(sound.macro_id,
                                           p4_track_defaults_json,
                                           sizeof(p4_track_defaults_json),
                                           kP4PresetCommandTimeoutMs)) {
    return false;
  }

  return parse_p4_macro_definition(p4_track_defaults_json, values, value_set,
                                   kP4PresetValueCount, sound);
}

bool TbdDevice::get_default_p4_sound(uint8_t p4_track_index,
                                     TbdP4SoundData *sound) const {
  return tbd_get_default_p4_sound(p4_track_index, sound);
}

bool TbdDevice::hydrate_p4_sound(TbdP4SoundData &sound) {
  return tbd_hydrate_p4_sound(sound);
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

uint8_t TbdDevice::mixer_default_param(uint8_t device_idx) const {
  (void)device_idx;
  return 0;
}

bool TbdDevice::mixer_param(uint8_t device_idx, uint8_t track,
                            uint8_t param_idx,
                            MidiDeviceMixerParam *param) {
  if (param == nullptr) {
    return false;
  }
  TbdP4SoundData *sound = p4_sound_for_mixer(device_idx, track);
  if (sound == nullptr || param_idx >= TBD_P4_MIXER_PARAM_COUNT) {
    return false;
  }

  TbdP4ParamDescriptor &desc = sound->mixer_params.params[param_idx];
  if (!desc.is_visible()) {
    return false;
  }

  param->label = desc.shortname;
  param->min_value = desc.min_value;
  param->max_value = desc.max_value;
  param->value = desc.value;
  param->type = desc.type;
  param->sendable = desc.is_sendable();
  return true;
}

bool TbdDevice::set_mixer_param(uint8_t device_idx, uint8_t track,
                                uint8_t param_idx, int16_t value,
                                bool send) {
  TbdP4SoundData *sound = p4_sound_for_mixer(device_idx, track);
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

  if (!send || !desc.is_sendable() || uart == nullptr) {
    return true;
  }

  tbd_p4_realtime.set_active_track(sound->p4_track_index);
  if (desc.ctrl_type == TBD_P4_CTRLTYPE_CC) {
    int16_t cc_value = value;
    if (cc_value < 0) cc_value = 0;
    if (cc_value > 127) cc_value = 127;
    uart->sendCC(sound->midi_channel, desc.ctrl, (uint8_t)cc_value);
  } else if (desc.ctrl_type == TBD_P4_CTRLTYPE_NRPM) {
    uint16_t nrpn_value = value < 0 ? 0 : (uint16_t)value;
    uart->sendNRPN(sound->midi_channel, desc.ctrl, nrpn_value);
  }
  return true;
}

void TbdDevice::mixer_mute_track(uint8_t device_idx, uint8_t track,
                                 bool mute, MidiUartClass *uart_) {
  (void)uart_;
  TbdP4SoundData *sound = p4_sound_for_mixer(device_idx, track);
  if (sound == nullptr) {
    return;
  }
  tbd_p4_command.set_track_mute(sound->p4_track_index, mute);
}

void TbdDevice::mixer_set_record_mutes(uint8_t device_idx, uint8_t track,
                                       bool state, bool clear) {
  SeqTrack *seq_track = seq_track_for_mixer(device_idx, track);
  if (seq_track == nullptr) {
    return;
  }
  seq_track->record_mutes = state;
  if (clear && device_idx == kTbdUiSlotPrimary) {
    mcl_seq.tbd_tracks[track].clear_mute();
  }
}

bool TbdDevice::probe() {
  connected = true;
  return true;
}

void TbdDevice::on_connection(uint8_t device_idx) {
  (void)device_idx;
  port = UARTP4_PORT;
  midi = &MidiP4;
  uart = MidiP4.uart;
  connected = true;
  cleanup(0);
  cleanup(1);
  if (mcl_cfg.grid_x_device == GRID_X_DEVICE_TBD) {
    init_grid_devices(0);
  }
  if (mcl_cfg.grid_y_device == GRID_Y_DEVICE_TBD) {
    init_grid_devices(1);
  }
  load_default_p4_presets();
}

void TbdDevice::init_grid_devices(uint8_t device_idx) {
  GridDeviceTrack gdt;

#if defined(PLATFORM_TBD)
  if (device_idx == 0) {
    for (uint8_t i = 0; i < mcl_seq.num_tbd_tracks; i++) {
      gdt.init(TBD_TRACK_TYPE, GROUP_DEV, device_idx,
               &(mcl_seq.tbd_tracks[i]));
      add_track_to_grid(0, i, &gdt);
    }
    return;
  }
#endif

  if (device_idx == 1) {
    for (uint8_t i = 0; i < NUM_EXT_TRACKS; i++) {
      const auto &def = tbd_track_default_for_slot(i);
      mcl_seq.midi_tracks[i].set_channel(def.midi_channel);
      gdt.init(TBD_MIDI_TRACK_TYPE, GROUP_DEV, device_idx,
               &(mcl_seq.midi_tracks[i]));
      add_track_to_grid(1, i, &gdt);
    }
  }
}

bool TbdDevice::load_default_p4_presets() {
  if (p4_defaults_loaded_) {
    return true;
  }

  const uint32_t now = millis();
  if (p4_defaults_last_attempt_ms_ != 0 &&
      now - p4_defaults_last_attempt_ms_ < kP4PresetRetryMs) {
    return false;
  }
  p4_defaults_last_attempt_ms_ = now;

  tbd_p4_command.init();
  if (!tbd_p4_command.wait_ready(kP4PresetReadyProbeMs)) {
    return false;
  }

  if (!tbd_p4_command.announce_app("MCL", 0, kP4PresetCommandTimeoutMs)) {
    return false;
  }

  reset_p4_boot_presets_to_fallback();

  char kit_id[32] = {0};
  if (tbd_p4_command.get_track_default_presets(
          p4_track_defaults_json, sizeof(p4_track_defaults_json), nullptr,
          kP4PresetCommandTimeoutMs)) {
    parse_p4_track_defaults(p4_track_defaults_json, p4_boot_presets,
                            kP4SoundTrackCount, kit_id, sizeof(kit_id));
  }

  if (kit_id[0] != '\0' &&
      tbd_p4_command.get_kit_index_json(
          p4_track_defaults_json, sizeof(p4_track_defaults_json),
          kP4PresetCommandTimeoutMs)) {
    const uint8_t kit_index =
        find_p4_kit_index_for_id(p4_track_defaults_json, kit_id);
    if (!tbd_p4_command.set_active_sample_kit(kit_index,
                                              kP4PresetCommandTimeoutMs)) {
      return false;
    }
  }

  for (const auto &preset : p4_boot_presets) {
    if (preset.preset_id[0] == '\0') {
      continue;
    }
    tbd_update_track_default_from_p4(preset.track_index, preset.preset_id,
                                     preset.rom_bank, preset.sample_slice);

    TbdP4SoundData sound;
    sound.clear();
    sound.p4_track_index = preset.track_index;
    sound.rom_bank = preset.rom_bank;
    sound.sample_slice = preset.sample_slice;
    copy_preset_id(sound.preset_id, preset.preset_id);
    tbd_init_p4_sound_runtime_defaults(sound);
    tbd_hydrate_p4_sound(sound);
    cache_default_sound(sound);

    if (!tbd_p4_command.load_track_sound_preset(
            preset.track_index, preset.preset_id, preset.rom_bank,
            preset.sample_slice,
            kP4PresetCommandTimeoutMs)) {
      return false;
    }
  }

  p4_defaults_loaded_ = true;
  return true;
}

void TbdDevice::note_on(uint8_t note) {
  if (port != UARTP4_PORT || uart == nullptr) return;
  note_off();
  active_note_ = note;
  tbd_p4_realtime.set_active_track(tbd_track_default_for_slot(0).p4_track_index);
  uart->sendNoteOn(0, note, 100);
}

void TbdDevice::note_off() {
  if (active_note_ == 255 || uart == nullptr) return;
  uart->sendNoteOff(0, active_note_);
  active_note_ = 255;
}

bool TbdDevice::enter_ui(gui_event_t *event) {
  if (port != UARTP4_PORT) return false;

  uint8_t device_idx = kTbdUiSlotNone;
  if (!tbd_ui_request_from_event(event, &device_idx)) {
    return false;
  }

  ui_device_idx_ = device_idx;
  tbd_ui_mode.enter(device_idx);
  if (!tbd_ui_mode.is_active()) ui_device_idx_ = kTbdUiSlotNone;
  return true;
}

bool TbdDevice::handle_ui_event(gui_event_t *event) {
  return tbd_ui_mode.handle_event(event);
}

void TbdDevice::ui_loop() {
  if (!p4_defaults_loaded_) {
    load_default_p4_presets();
  }

  tbd_ui_mode.poll_encoders();
}

bool TbdDevice::is_ui_active() {
  return tbd_ui_mode.is_active();
}

void TbdDevice::exit_ui() {
  note_off();
  tbd_ui_mode.disable();
  ui_device_idx_ = kTbdUiSlotNone;
}

#endif // PLATFORM_TBD
