#pragma once

#include "platform.h"

#ifdef PLATFORM_TBD

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static constexpr uint8_t TBD_P4_SOUND_DATA_VERSION = 1;

static constexpr uint8_t TBD_P4_SOUND_TRACK_COUNT = 16;
static constexpr uint8_t TBD_P4_BUS_TRACK_COUNT = 3;

static constexpr uint8_t TBD_P4_TRACK_TYPE_MONOSYNTH = 1;
static constexpr uint8_t TBD_P4_TRACK_TYPE_POLYSYNTH = 2;
static constexpr uint8_t TBD_P4_TRACK_TYPE_DRUM = 3;

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
static constexpr uint8_t TBD_P4_LOCK_NOTE_PARAM =
    TBD_P4_LOCK_RESERVED_PARAM_BASE;
static constexpr uint8_t TBD_P4_LOCK_PARAM_COUNT =
    TBD_P4_LOCK_RESERVED_PARAM_BASE + 2;
static constexpr uint8_t TBD_P4_DEFAULT_STEP_NOTE = 60;

static constexpr uint8_t TBD_P4_ID_LEN = 16;
static constexpr uint8_t TBD_P4_PARAM_PAGE_NAME_LEN = 16;
static constexpr uint8_t TBD_P4_PARAM_SHORT_NAME_LEN = 8;

static constexpr uint8_t TBD_P4_PARAM_TYPE_NONE = 0;
static constexpr uint8_t TBD_P4_PARAM_TYPE_NUMBER = 1;
static constexpr uint8_t TBD_P4_PARAM_TYPE_BIG_NUMBER = 2;
static constexpr uint8_t TBD_P4_PARAM_TYPE_LEVEL = 10;
static constexpr uint8_t TBD_P4_PARAM_TYPE_PAN = 11;
static constexpr uint8_t TBD_P4_PARAM_TYPE_BIPOLAR = 12;
static constexpr uint8_t TBD_P4_PARAM_TYPE_GAIN_LEVEL = 14;
static constexpr uint8_t TBD_P4_PARAM_TYPE_LEVEL_MASTER = 15;
static constexpr uint8_t TBD_P4_PARAM_TYPE_FILTER_TYPE = 20;
static constexpr uint8_t TBD_P4_PARAM_TYPE_FILTER_CUTOFF = 21;
static constexpr uint8_t TBD_P4_PARAM_TYPE_FILTER_Q = 22;
static constexpr uint8_t TBD_P4_PARAM_TYPE_FILTER_MODE = 23;
static constexpr uint8_t TBD_P4_PARAM_TYPE_ENV_ATTACK = 30;
static constexpr uint8_t TBD_P4_PARAM_TYPE_ENV_DECAY = 31;
static constexpr uint8_t TBD_P4_PARAM_TYPE_ENV_AMOUNT = 32;
static constexpr uint8_t TBD_P4_PARAM_TYPE_ENV_ATTACK_FAST = 33;
static constexpr uint8_t TBD_P4_PARAM_TYPE_DB_THRESHOLD = 34;
static constexpr uint8_t TBD_P4_PARAM_TYPE_DB_GAIN = 35;
static constexpr uint8_t TBD_P4_PARAM_TYPE_RATIO = 36;
static constexpr uint8_t TBD_P4_PARAM_TYPE_SECONDS = 37;
static constexpr uint8_t TBD_P4_PARAM_TYPE_MS = 38;
static constexpr uint8_t TBD_P4_PARAM_TYPE_COMP_ATTACK = 39;
static constexpr uint8_t TBD_P4_PARAM_TYPE_DISTORTION = 40;
static constexpr uint8_t TBD_P4_PARAM_TYPE_SHAPE = 41;
static constexpr uint8_t TBD_P4_PARAM_TYPE_SHAPE2 = 42;
static constexpr uint8_t TBD_P4_PARAM_TYPE_SHAPE3 = 43;
static constexpr uint8_t TBD_P4_PARAM_TYPE_FREQ = 44;
static constexpr uint8_t TBD_P4_PARAM_TYPE_NOISE = 45;
static constexpr uint8_t TBD_P4_PARAM_TYPE_COMP_RELEASE = 46;
static constexpr uint8_t TBD_P4_PARAM_TYPE_SAMPLE_BANK = 60;
static constexpr uint8_t TBD_P4_PARAM_TYPE_SAMPLE_SLICE = 61;
static constexpr uint8_t TBD_P4_PARAM_TYPE_MACRO_SHAPE = 62;
static constexpr uint8_t TBD_P4_PARAM_TYPE_CHORD_SHAPE = 70;
static constexpr uint8_t TBD_P4_PARAM_TYPE_CHORD_INV = 71;
static constexpr uint8_t TBD_P4_PARAM_TYPE_NNOTES = 74;
static constexpr uint8_t TBD_P4_PARAM_TYPE_ONOFF = 77;
static constexpr uint8_t TBD_P4_PARAM_TYPE_TAPE_DIGITAL = 78;
static constexpr uint8_t TBD_P4_PARAM_TYPE_FREE_SYNC = 79;
static constexpr uint8_t TBD_P4_PARAM_TYPE_SCALE = 80;
static constexpr uint8_t TBD_P4_PARAM_TYPE_TIME_DIVISOR = 81;
static constexpr uint8_t TBD_P4_PARAM_TYPE_PERCENT = 82;
static constexpr uint8_t TBD_P4_PARAM_TYPE_PITCH_SEMI = 85;
static constexpr uint8_t TBD_P4_PARAM_TYPE_SPEED_MULT = 86;
static constexpr uint8_t TBD_P4_PARAM_TYPE_BITCR_BITS = 87;
static constexpr uint8_t TBD_P4_PARAM_TYPE_TBD_MODEL = 88;
static constexpr uint8_t TBD_P4_PARAM_TYPE_TBD_MOD_TYPE = 89;
static constexpr uint8_t TBD_P4_PARAM_TYPE_TBD_VOICES = 90;
static constexpr uint8_t TBD_P4_PARAM_TYPE_PLAITS_MODEL = 91;
static constexpr uint8_t TBD_P4_PARAM_TYPE_PLAITS_HARM = 92;
static constexpr uint8_t TBD_P4_PARAM_TYPE_PLAITS_COLOR = 93;
static constexpr uint8_t TBD_P4_PARAM_TYPE_PLAITS_DETUNE = 94;
static constexpr uint8_t TBD_P4_PARAM_TYPE_TBD_CHORD = 95;
static constexpr uint8_t TBD_P4_PARAM_TYPE_PLAITS_DECAY = 96;
static constexpr uint8_t TBD_P4_PARAM_TYPE_TBD_ENVSH = 97;
static constexpr uint8_t TBD_P4_PARAM_TYPE_TBD_MSNAP = 98;
static constexpr uint8_t TBD_P4_PARAM_TYPE_WTOSC_BANK = 99;
static constexpr uint8_t TBD_P4_PARAM_TYPE_HIDDEN = 100;
static constexpr uint8_t TBD_P4_PARAM_TYPE_LFO_RATE = 101;
static constexpr uint8_t TBD_P4_PARAM_TYPE_TD3_FTYPE = 102;
static constexpr uint8_t TBD_P4_PARAM_TYPE_TD3_DRIVE = 103;
static constexpr uint8_t TBD_P4_PARAM_TYPE_TD3_ACCLEV = 104;
static constexpr uint8_t TBD_P4_PARAM_TYPE_TD3_ACCENT = 105;
static constexpr uint8_t TBD_P4_PARAM_TYPE_MS_2S = 110;

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

  bool has_sendable_params() const {
    for (uint8_t i = 0; i < TBD_P4_AUDIO_PARAM_COUNT; i++) {
      if (audio_params.params[i].is_sendable()) {
        return true;
      }
    }
    for (uint8_t i = 0; i < TBD_P4_MIXER_PARAM_COUNT; i++) {
      if (mixer_params.params[i].is_sendable()) {
        return true;
      }
    }
    return false;
  }

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

inline bool tbd_p4_track_type_uses_step_note(uint8_t track_type) {
  return track_type == TBD_P4_TRACK_TYPE_MONOSYNTH ||
         track_type == TBD_P4_TRACK_TYPE_POLYSYNTH;
}

inline bool tbd_p4_sound_uses_step_note(const TbdP4SoundData &sound) {
  return tbd_p4_track_type_uses_step_note(sound.track_type);
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
