#pragma once

#include "platform.h"

#ifdef PLATFORM_TBD

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static constexpr uint8_t TBD_P4_SOUND_DATA_VERSION = 1;

static constexpr uint8_t TBD_P4_SOUND_TRACK_COUNT = 16;
static constexpr uint8_t TBD_P4_BUS_TRACK_COUNT = 3;

static constexpr uint8_t TBD_P4_AUDIO_PARAM_COUNT = 24;
static constexpr uint8_t TBD_P4_AUDIO_PARAM_PAGE_COUNT = 6;
static constexpr uint8_t TBD_P4_MIXER_PARAM_COUNT = 8;
static constexpr uint8_t TBD_P4_MIXER_PARAM_PAGE_COUNT = 2;
static constexpr uint8_t TBD_P4_LOCK_AUDIO_PARAM_COUNT =
    TBD_P4_AUDIO_PARAM_COUNT;
static constexpr uint8_t TBD_P4_LOCK_MIXER_PARAM_BASE =
    TBD_P4_AUDIO_PARAM_COUNT;
static constexpr uint8_t TBD_P4_LOCK_MIXER_PARAM_COUNT =
    TBD_P4_MIXER_PARAM_COUNT;
static constexpr uint8_t TBD_P4_LOCK_RESERVED_PARAM_BASE =
    TBD_P4_LOCK_MIXER_PARAM_BASE + TBD_P4_LOCK_MIXER_PARAM_COUNT;
static constexpr uint8_t TBD_P4_LOCK_PARAM_COUNT =
    TBD_P4_LOCK_RESERVED_PARAM_BASE + 2;

static constexpr uint8_t TBD_P4_ID_LEN = 16;
static constexpr uint8_t TBD_P4_PARAM_PAGE_NAME_LEN = 16;
static constexpr uint8_t TBD_P4_PARAM_SHORT_NAME_LEN = 8;

static constexpr uint8_t TBD_P4_PARAM_TYPE_NONE = 0;
static constexpr uint8_t TBD_P4_PARAM_TYPE_HIDDEN = 100;

static constexpr uint8_t TBD_P4_AUDIO_PARAM_FLAG_MACRO = 0x01;

enum TbdP4CtrlType : uint8_t {
  TBD_P4_CTRLTYPE_UNKNOWN = 0,
  TBD_P4_CTRLTYPE_CC = 1,
  // The reference source names this NRPM; it is NRPN on the MIDI wire.
  TBD_P4_CTRLTYPE_NRPM = 2,
  TBD_P4_CTRLTYPE_NRPN = TBD_P4_CTRLTYPE_NRPM,
};

struct ATTR_PACKED() TbdP4ParamPage {
  char name[TBD_P4_PARAM_PAGE_NAME_LEN];

  void clear() { memset(this, 0, sizeof(*this)); }
};

struct ATTR_PACKED() TbdP4ParamDescriptor {
  uint8_t type;
  char shortname[TBD_P4_PARAM_SHORT_NAME_LEN];

  int16_t min_value;
  int16_t max_value;
  int16_t default_value;
  int16_t value;

  // Final outbound CC/NRPN control number after any P4 track base offset.
  uint8_t ctrl;
  uint8_t ctrl_type;

  uint16_t resolution;
  uint8_t flags;

  void clear() { memset(this, 0, sizeof(*this)); }

  bool is_visible() const {
    return type != TBD_P4_PARAM_TYPE_NONE &&
           type != TBD_P4_PARAM_TYPE_HIDDEN;
  }

  bool is_sendable() const {
    return is_visible() &&
           (ctrl_type == TBD_P4_CTRLTYPE_CC ||
            ctrl_type == TBD_P4_CTRLTYPE_NRPM);
  }

  bool is_macro() const { return (flags & TBD_P4_AUDIO_PARAM_FLAG_MACRO) != 0; }
};

struct ATTR_PACKED() TbdP4AudioParamGroup {
  uint8_t num_pages;
  TbdP4ParamPage pages[TBD_P4_AUDIO_PARAM_PAGE_COUNT];
  TbdP4ParamDescriptor params[TBD_P4_AUDIO_PARAM_COUNT];

  void clear() { memset(this, 0, sizeof(*this)); }
};

struct ATTR_PACKED() TbdP4MixerParamGroup {
  uint8_t num_pages;
  TbdP4ParamPage pages[TBD_P4_MIXER_PARAM_PAGE_COUNT];
  TbdP4ParamDescriptor params[TBD_P4_MIXER_PARAM_COUNT];

  void clear() { memset(this, 0, sizeof(*this)); }
};

struct ATTR_PACKED() TbdP4SoundData {
  uint8_t version;

  uint8_t p4_track_index;
  uint8_t midi_channel;
  uint8_t track_type;
  uint8_t trig_gate_time_ms;
  int8_t trig_note;
  int16_t note_cc;
  int16_t device_start_cc;

  uint8_t rom_bank;
  int32_t sample_slice;

  char preset_id[TBD_P4_ID_LEN];
  char preset_name[TBD_P4_ID_LEN];
  char macro_id[TBD_P4_ID_LEN];
  char machine_id[TBD_P4_ID_LEN];

  TbdP4AudioParamGroup audio_params;
  TbdP4MixerParamGroup mixer_params;

  uint8_t flags;
  uint8_t reserved[15];

  void clear() {
    memset(this, 0, sizeof(*this));
    version = TBD_P4_SOUND_DATA_VERSION;
    trig_note = -1;
    note_cc = -1;
    device_start_cc = -1;
    rom_bank = 0xFF;
    sample_slice = -1;
  }

  bool has_preset() const { return preset_id[0] != '\0'; }
  bool has_macro() const { return macro_id[0] != '\0'; }
  bool has_machine() const { return machine_id[0] != '\0'; }

  bool valid_param_index(uint8_t index) const {
    return index < TBD_P4_AUDIO_PARAM_COUNT;
  }

  bool valid_mixer_param_index(uint8_t index) const {
    return index < TBD_P4_MIXER_PARAM_COUNT;
  }
};

inline const TbdP4ParamDescriptor *
tbd_p4_sound_param_for_lock(const TbdP4SoundData &sound, uint8_t lock_param) {
  if (lock_param < TBD_P4_LOCK_AUDIO_PARAM_COUNT) {
    return &sound.audio_params.params[lock_param];
  }
  if (lock_param >= TBD_P4_LOCK_MIXER_PARAM_BASE &&
      lock_param < TBD_P4_LOCK_RESERVED_PARAM_BASE) {
    return &sound.mixer_params.params[lock_param - TBD_P4_LOCK_MIXER_PARAM_BASE];
  }
  return nullptr;
}

inline int16_t tbd_p4_scale_lock_value(const TbdP4ParamDescriptor &param,
                                       uint16_t value14) {
  if (value14 > 0x3FFF) {
    value14 = 0x3FFF;
  }
  if (param.max_value <= param.min_value) {
    return param.min_value;
  }

  const int32_t range = (int32_t)param.max_value - (int32_t)param.min_value;
  int32_t scaled = (int32_t)param.min_value +
                   ((range * (int32_t)value14 + 0x1FFF) / 0x3FFF);
  if (scaled < param.min_value) {
    scaled = param.min_value;
  } else if (scaled > param.max_value) {
    scaled = param.max_value;
  }
  return (int16_t)scaled;
}

static_assert(sizeof(TbdP4ParamDescriptor) == 22,
              "TBD P4 param descriptor size changed");
static_assert(sizeof(TbdP4SoundData) <= 1024,
              "TBD P4 sound data must stay grid-slot friendly");

#endif // PLATFORM_TBD
