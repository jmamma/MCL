#include "TBDTrack.h"

#ifdef PLATFORM_TBD

#include "TBD.h"
#include "EmptyTrack.h"
#include "TbdP4Command.h"
#include "TbdP4Realtime.h"
#include <string.h>

namespace {

constexpr uint8_t kDefaultRomBank = 0xFF;
constexpr int32_t kDefaultSampleSlice = -1;
constexpr uint32_t kPresetApplyTimeoutMs = 3000;
constexpr uint8_t kTbdSongTrackTypeMonosynth = 1;
constexpr uint8_t kTbdSongTrackTypePolysynth = 2;
constexpr uint8_t kTbdSongTrackTypeDrum = 3;

struct TbdP4RuntimeDefault {
  uint8_t midi_channel;
  int8_t trig_note;
  int16_t device_start_cc;
  int16_t note_cc;
  uint8_t track_type;
};

TbdTrackDefault kTbdMidiTrackDefaults[] = {
    {8, 0, "td3-all-def", kDefaultRomBank, kDefaultSampleSlice},
    {9, 1, "td3-all-def", kDefaultRomBank, kDefaultSampleSlice},
    {10, 2, "mo-all-def", kDefaultRomBank, kDefaultSampleSlice},
    {11, 3, "wtosc-all-def", kDefaultRomBank, kDefaultSampleSlice},
    {12, 4, "ro-all-def", kDefaultRomBank, kDefaultSampleSlice},
    {13, 5, "ro-all-def", kDefaultRomBank, kDefaultSampleSlice},
};

TbdTrackDefault kTbdStepTrackDefaults[] = {
    {0, 0, "", kDefaultRomBank, kDefaultSampleSlice},
    {1, 0, "", kDefaultRomBank, kDefaultSampleSlice},
    {2, 0, "", kDefaultRomBank, kDefaultSampleSlice},
    {3, 0, "", kDefaultRomBank, kDefaultSampleSlice},
    {4, 0, "", kDefaultRomBank, kDefaultSampleSlice},
    {5, 0, "", kDefaultRomBank, kDefaultSampleSlice},
    {6, 0, "", kDefaultRomBank, kDefaultSampleSlice},
    {7, 0, "", kDefaultRomBank, kDefaultSampleSlice},
    {8, 0, "", kDefaultRomBank, kDefaultSampleSlice},
    {9, 0, "", kDefaultRomBank, kDefaultSampleSlice},
    {10, 0, "", kDefaultRomBank, kDefaultSampleSlice},
    {11, 0, "", kDefaultRomBank, kDefaultSampleSlice},
    {12, 0, "", kDefaultRomBank, kDefaultSampleSlice},
    {13, 0, "", kDefaultRomBank, kDefaultSampleSlice},
    {14, 0, "", kDefaultRomBank, kDefaultSampleSlice},
    {15, 0, "", kDefaultRomBank, kDefaultSampleSlice},
};

const TbdP4RuntimeDefault kTbdP4RuntimeDefaults[] = {
    {9, 36, 0, -1, kTbdSongTrackTypeDrum},
    {9, 37, 40, -1, kTbdSongTrackTypeDrum},
    {9, 38, 80, -1, kTbdSongTrackTypeDrum},
    {10, 36, 0, -1, kTbdSongTrackTypeDrum},
    {10, 37, 40, -1, kTbdSongTrackTypeDrum},
    {10, 38, 80, -1, kTbdSongTrackTypeDrum},
    {11, 36, 0, -1, kTbdSongTrackTypeDrum},
    {11, 37, 40, -1, kTbdSongTrackTypeDrum},
    {0, -1, 0, -1, kTbdSongTrackTypeMonosynth},
    {1, -1, 0, -1, kTbdSongTrackTypeMonosynth},
    {2, -1, 0, -1, kTbdSongTrackTypeMonosynth},
    {3, -1, 0, -1, kTbdSongTrackTypeMonosynth},
    {4, -1, 0, -1, kTbdSongTrackTypePolysynth},
    {5, -1, 0, -1, kTbdSongTrackTypePolysynth},
    {6, -1, 0, -1, kTbdSongTrackTypePolysynth},
    {7, -1, 0, -1, kTbdSongTrackTypeMonosynth},
};

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
  copy_fixed_string(dst, TBD_PRESET_ID_LEN, src);
}

void set_param(TbdP4ParamDescriptor &param, const char *name, uint8_t type,
               uint8_t ctrl, uint8_t ctrl_type, int16_t value,
               int16_t min_value = 0, int16_t max_value = 127,
               uint16_t resolution = 127) {
  param.clear();
  param.type = type;
  copy_fixed_string(param.shortname, sizeof(param.shortname), name);
  param.min_value = min_value;
  param.max_value = max_value;
  param.default_value = value;
  param.value = value;
  param.ctrl = ctrl;
  param.ctrl_type = ctrl_type;
  param.resolution = resolution;
}

void set_sound_from_default(TbdP4SoundData &sound, const TbdTrackDefault &def) {
  sound.clear();
  sound.p4_track_index = def.p4_track_index;
  sound.midi_channel = def.midi_channel;
  sound.rom_bank = def.rom_bank;
  sound.sample_slice = def.sample_slice;
  copy_fixed_string(sound.preset_id, sizeof(sound.preset_id), def.preset_id);
  tbd_init_p4_sound_runtime_defaults(sound);
}

void set_step_sound_default(TbdP4SoundData &sound, uint8_t slot) {
  if (slot >= sizeof(kTbdStepTrackDefaults) / sizeof(kTbdStepTrackDefaults[0])) {
    slot = 0;
  }
  const uint8_t p4_track_index = kTbdStepTrackDefaults[slot].p4_track_index;
  if (tbd_get_default_p4_sound(p4_track_index, &sound)) {
    return;
  }
  set_sound_from_default(sound, kTbdStepTrackDefaults[slot]);
}

void set_midi_sound_default(TbdP4SoundData &sound, uint8_t slot) {
  const auto &def = tbd_track_default_for_slot(slot);
  if (tbd_get_default_p4_sound(def.p4_track_index, &sound)) {
    return;
  }
  set_sound_from_default(sound, def);
}

void apply_p4_sound(const TbdP4SoundData &sound) {
  tbd_p4_realtime.set_active_track(sound.p4_track_index);
  if (sound.has_machine()) {
    tbd_p4_command.activate_track_machine(sound.p4_track_index,
                                          sound.machine_id,
                                          kPresetApplyTimeoutMs);
  }
  if (sound.has_preset()) {
    tbd_p4_command.load_track_sound_preset(sound.p4_track_index,
                                           sound.preset_id,
                                           sound.rom_bank,
                                           sound.sample_slice,
                                           kPresetApplyTimeoutMs);
  } else if (sound.has_macro()) {
    tbd_p4_command.load_track_macro_definition(sound.p4_track_index,
                                               sound.macro_id,
                                               kPresetApplyTimeoutMs);
  }
}

} // namespace

const TbdTrackDefault &tbd_track_default_for_slot(uint8_t slot) {
  if (slot >= sizeof(kTbdMidiTrackDefaults) / sizeof(kTbdMidiTrackDefaults[0])) {
    slot = 0;
  }
  return kTbdMidiTrackDefaults[slot];
}

void tbd_init_p4_sound_runtime_defaults(TbdP4SoundData &sound) {
  if (sound.p4_track_index >=
      sizeof(kTbdP4RuntimeDefaults) / sizeof(kTbdP4RuntimeDefaults[0])) {
    sound.p4_track_index = 0;
  }

  const auto &def = kTbdP4RuntimeDefaults[sound.p4_track_index];
  sound.midi_channel = def.midi_channel;
  sound.trig_note = def.trig_note;
  sound.note_cc = def.note_cc;
  sound.device_start_cc = def.device_start_cc;
  sound.track_type = def.track_type;
  sound.trig_gate_time_ms = 20;

  sound.mixer_params.clear();
  sound.mixer_params.num_pages = 1;
  copy_fixed_string(sound.mixer_params.pages[0].name,
                    sizeof(sound.mixer_params.pages[0].name), "MIX");

  const uint8_t base_cc =
      sound.device_start_cc < 0 ? 0 : (uint8_t)sound.device_start_cc;
  const int16_t level_default = sound.p4_track_index == 15 ? 0 : 64;
  set_param(sound.mixer_params.params[0], "LEVEL", TBD_P4_PARAM_TYPE_LEVEL,
            base_cc + 1, TBD_P4_CTRLTYPE_CC, level_default);
  set_param(sound.mixer_params.params[1], "PAN", TBD_P4_PARAM_TYPE_PAN,
            base_cc + 2, TBD_P4_CTRLTYPE_CC, 64);
  set_param(sound.mixer_params.params[2], "FX1", TBD_P4_PARAM_TYPE_LEVEL,
            base_cc + 3, TBD_P4_CTRLTYPE_CC, 0);
  set_param(sound.mixer_params.params[3], "FX2", TBD_P4_PARAM_TYPE_LEVEL,
            base_cc + 4, TBD_P4_CTRLTYPE_CC, 0);
  set_param(sound.mixer_params.params[4], "T.LEN", TBD_P4_PARAM_TYPE_HIDDEN,
            base_cc + 5, TBD_P4_CTRLTYPE_CC, 1);
}

void tbd_update_track_default_from_p4(uint8_t p4_track_index,
                                      const char *preset_id,
                                      uint8_t rom_bank,
                                      int32_t sample_slice) {
  if (preset_id == nullptr || preset_id[0] == '\0') {
    return;
  }

  for (auto &def : kTbdMidiTrackDefaults) {
    if (def.p4_track_index == p4_track_index) {
      copy_preset_id(def.preset_id, preset_id);
      def.rom_bank = rom_bank;
      def.sample_slice = sample_slice;
    }
  }
  if (p4_track_index <
      sizeof(kTbdStepTrackDefaults) / sizeof(kTbdStepTrackDefaults[0])) {
    auto &def = kTbdStepTrackDefaults[p4_track_index];
    copy_preset_id(def.preset_id, preset_id);
    def.rom_bank = rom_bank;
    def.sample_slice = sample_slice;
  }
}

TBDTrack::TBDTrack() {
  active = TBD_TRACK_TYPE;
  seq_data.init();
  set_step_sound_default(p4_sound, 0);
  static_assert(MEMORY_ALIGN(sizeof(TBDTrack) - sizeof(void *)) <= TBD_TRACK_LEN);
}

void TBDTrack::apply_seq_defaults(uint8_t tracknumber, SeqTrack *seq_track) {
  if (p4_sound.version != TBD_P4_SOUND_DATA_VERSION) {
    set_step_sound_default(p4_sound, tracknumber);
  }
  if (p4_sound.has_preset()) {
    tbd_hydrate_p4_sound(p4_sound);
  }

  if (seq_data.track_length == 0) {
    seq_data.track_length = 16;
  }
  if (seq_data.track_speed == 0xFF) {
    seq_data.track_speed = STEPSEQ_SPEED_1X;
  }

  if (seq_track != nullptr) {
    auto *tbd_track = static_cast<TBDSeqTrack *>(seq_track);
    tbd_track->p4_sound = p4_sound;
  }
}

void TBDTrack::init(uint8_t tracknumber, SeqTrack *seq_track) {
  seq_data.init();
  set_step_sound_default(p4_sound, tracknumber);
  link.speed = STEPSEQ_SPEED_1X;
  link.length = 16;
  apply_seq_defaults(tracknumber, seq_track);
}

uint16_t TBDTrack::calc_latency(uint8_t tracknumber) {
  (void)tracknumber;
  return 2048;
}

void TBDTrack::apply_preset(uint8_t fallback_tracknumber) {
  if (p4_sound.version != TBD_P4_SOUND_DATA_VERSION) {
    set_step_sound_default(p4_sound, fallback_tracknumber);
  }
  tbd_hydrate_p4_sound(p4_sound);
  apply_p4_sound(p4_sound);
}

void TBDTrack::transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                               uint8_t slotnumber) {
  GridTrack::transition_load(tracknumber, seq_track, slotnumber);
  load_seq_data(seq_track);
  apply_seq_defaults(tracknumber, seq_track);
}

void TBDTrack::transition_send(uint8_t tracknumber, uint8_t slotnumber) {
  (void)slotnumber;
  apply_preset(tracknumber);
}

void TBDTrack::load_seq_data(SeqTrack *seq_track) {
  if (seq_track == nullptr) {
    return;
  }

  auto *tbd_track = static_cast<TBDSeqTrack *>(seq_track);
  load_link_data(seq_track);
  memcpy(tbd_track->TBDSeqTrackData::data(), seq_data.data(),
         sizeof(TBDSeqTrackData));
  tbd_track->p4_sound = p4_sound;
  tbd_track->set_length(seq_data.track_length ? seq_data.track_length
                                              : link.length);
  tbd_track->set_speed(seq_data.track_speed == 0xFF ? link.speed
                                                    : seq_data.track_speed);
}

void TBDTrack::load_immediate(uint8_t tracknumber, SeqTrack *seq_track) {
  load_seq_data(seq_track);
  apply_seq_defaults(tracknumber, seq_track);
  apply_preset(tracknumber);
}

bool TBDTrack::store_in_grid(uint8_t column, uint16_t row, SeqTrack *seq_track,
                             uint8_t merge, bool online, Grid *grid) {
  (void)merge;
  (void)online;

  active = TBD_TRACK_TYPE;

  const uint8_t slot = column & 0x0F;
  set_step_sound_default(p4_sound, slot);

  EmptyTrack scratch;
  if (auto *cached = scratch.load_from_mem<TBDTrack>(slot)) {
    memcpy(&p4_sound, &cached->p4_sound, sizeof(p4_sound));
  }

  if (seq_track != nullptr) {
    link.length = seq_track->length;
    link.speed = seq_track->speed;
    auto *tbd_track = static_cast<TBDSeqTrack *>(seq_track);
    p4_sound = tbd_track->p4_sound;
    memcpy(seq_data.data(), tbd_track->TBDSeqTrackData::data(),
           sizeof(TBDSeqTrackData));
    seq_data.track_length = seq_track->length;
    seq_data.track_speed = seq_track->speed;
  }

  apply_seq_defaults(slot, seq_track);

  return write_grid(_this(), _sizeof(), column, row, grid);
}

TBDMidiTrack::TBDMidiTrack() {
  active = TBD_MIDI_TRACK_TYPE;
  seq_data.clear();
  set_midi_sound_default(p4_sound, 0);
  static_assert(MEMORY_ALIGN(sizeof(TBDMidiTrack) - sizeof(void *)) <=
                GRID2_TRACK_LEN);
}

void TBDMidiTrack::apply_seq_defaults(uint8_t tracknumber,
                                      SeqTrack *seq_track) {
  if (p4_sound.version != TBD_P4_SOUND_DATA_VERSION) {
    set_midi_sound_default(p4_sound, tracknumber);
  }
  if (p4_sound.has_preset() || p4_sound.has_macro() || p4_sound.has_machine()) {
    tbd_hydrate_p4_sound(p4_sound);
  }

  if (seq_data.version != MIDI_SEQ_DATA_VERSION) {
    seq_data.clear();
  }
  if (seq_data.length == 0) {
    seq_data.length = link.length ? link.length : 16;
  }
  if (seq_data.speed == 0) {
    seq_data.speed = link.speed;
  }
  seq_data.channel = p4_sound.midi_channel;

  if (seq_track != nullptr) {
    auto *midi_track = static_cast<MidiSeqTrack *>(seq_track);
    midi_track->p4_sound = p4_sound;
    midi_track->seq_data.channel = p4_sound.midi_channel;
    midi_track->set_channel(p4_sound.midi_channel);
    midi_track->set_length(seq_data.length ? seq_data.length : link.length);
    midi_track->set_speed(seq_data.speed ? seq_data.speed : link.speed);
  }
}

void TBDMidiTrack::init(uint8_t tracknumber, SeqTrack *seq_track) {
  seq_data.clear();
  set_midi_sound_default(p4_sound, tracknumber);
  link.speed = SEQ_SPEED_1X;
  link.length = 16;
  apply_seq_defaults(tracknumber, seq_track);
}

uint16_t TBDMidiTrack::calc_latency(uint8_t tracknumber) {
  (void)tracknumber;
  return 2048;
}

void TBDMidiTrack::apply_preset(uint8_t fallback_tracknumber) {
  if (p4_sound.version != TBD_P4_SOUND_DATA_VERSION) {
    set_midi_sound_default(p4_sound, fallback_tracknumber);
  }
  tbd_hydrate_p4_sound(p4_sound);
  apply_p4_sound(p4_sound);
}

void TBDMidiTrack::transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                                   uint8_t slotnumber) {
  GridTrack::transition_load(tracknumber, seq_track, slotnumber);
  load_seq_data(seq_track);
  apply_seq_defaults(tracknumber, seq_track);
}

void TBDMidiTrack::transition_send(uint8_t tracknumber, uint8_t slotnumber) {
  (void)slotnumber;
  apply_preset(tracknumber);
}

void TBDMidiTrack::load_immediate(uint8_t tracknumber, SeqTrack *seq_track) {
  load_seq_data(seq_track);
  apply_seq_defaults(tracknumber, seq_track);
  apply_preset(tracknumber);
}

void TBDMidiTrack::load_seq_data(SeqTrack *seq_track) {
  if (seq_track == nullptr) {
    return;
  }

  auto *midi_track = static_cast<MidiSeqTrack *>(seq_track);
  uint8_t old_mute = midi_track->mute_state;
  midi_track->mute_state = SEQ_MUTE_ON;
  midi_track->notesoff_pending = true;

  if (seq_data.version != MIDI_SEQ_DATA_VERSION) {
    seq_data.clear();
  }
  load_link_data(seq_track);
  midi_track->seq_data = seq_data;
  midi_track->p4_sound = p4_sound;
  midi_track->set_channel(p4_sound.midi_channel);
  midi_track->set_length(seq_data.length ? seq_data.length : link.length);
  midi_track->set_speed(seq_data.speed ? seq_data.speed : link.speed);
  midi_track->mute_state = old_mute;
}

bool TBDMidiTrack::store_in_grid(uint8_t column, uint16_t row,
                                 SeqTrack *seq_track, uint8_t merge,
                                 bool online, Grid *grid) {
  (void)merge;
  (void)online;

  active = TBD_MIDI_TRACK_TYPE;

  const uint8_t slot = column & 0x0F;

  EmptyTrack scratch;
  if (auto *cached = scratch.load_from_mem<TBDMidiTrack>(slot)) {
    memcpy(&p4_sound, &cached->p4_sound, sizeof(p4_sound));
  }

  if (seq_track != nullptr) {
    link.length = seq_track->length;
    link.speed = seq_track->speed;
    auto *midi_track = static_cast<MidiSeqTrack *>(seq_track);
    p4_sound = midi_track->p4_sound;
    seq_data = midi_track->seq_data;
    seq_data.length = seq_track->length;
    seq_data.speed = seq_track->speed;
  }

  apply_seq_defaults(slot, seq_track);

  return write_grid(_this(), _sizeof(), column, row, grid);
}

#endif // PLATFORM_TBD
