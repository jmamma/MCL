#include "TBDTrack.h"

#ifdef PLATFORM_TBD

#include "TBD.h"
#include "EmptyTrack.h"
#include "MCLSeq.h"
#include "TbdP4Command.h"
#include "TbdP4Realtime.h"
#include "../Generic/GridTracks/MidiTrackMaterializer.h"
#include <string.h>

namespace {

constexpr uint8_t kDefaultRomBank = 0xFF;
constexpr int32_t kDefaultSampleSlice = -1;
constexpr uint32_t kPresetApplyTimeoutMs = 30000;

uint8_t midi_seq_valid_speed(uint8_t speed) {
  return speed <= SEQ_SPEED_4X ? speed : SEQ_SPEED_1X;
}

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
    {12, 4, "ro-full-def", 2, 2},
    {13, 5, "ro-full-def", 3, 0},
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
    {9, 36, 0, -1, TBD_P4_TRACK_TYPE_DRUM},
    {9, 37, 40, -1, TBD_P4_TRACK_TYPE_DRUM},
    {9, 38, 80, -1, TBD_P4_TRACK_TYPE_DRUM},
    {10, 36, 0, -1, TBD_P4_TRACK_TYPE_DRUM},
    {10, 37, 40, -1, TBD_P4_TRACK_TYPE_DRUM},
    {10, 38, 80, -1, TBD_P4_TRACK_TYPE_DRUM},
    {11, 36, 0, -1, TBD_P4_TRACK_TYPE_DRUM},
    {11, 37, 40, -1, TBD_P4_TRACK_TYPE_DRUM},
    {0, -1, 0, -1, TBD_P4_TRACK_TYPE_MONOSYNTH},
    {1, -1, 0, -1, TBD_P4_TRACK_TYPE_MONOSYNTH},
    {2, -1, 0, -1, TBD_P4_TRACK_TYPE_MONOSYNTH},
    {3, -1, 0, -1, TBD_P4_TRACK_TYPE_MONOSYNTH},
    {4, -1, 0, -1, TBD_P4_TRACK_TYPE_POLYSYNTH},
    {5, -1, 0, -1, TBD_P4_TRACK_TYPE_POLYSYNTH},
    {6, -1, 0, -1, TBD_P4_TRACK_TYPE_POLYSYNTH},
    {7, -1, 0, -1, TBD_P4_TRACK_TYPE_MONOSYNTH},
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

bool p4_sound_has_audio_params(const TbdP4SoundData &sound) {
  for (uint8_t i = 0; i < TBD_P4_AUDIO_PARAM_COUNT; i++) {
    if (sound.audio_params.params[i].type != TBD_P4_PARAM_TYPE_NONE) {
      return true;
    }
  }
  return false;
}

bool p4_sound_has_identity(const TbdP4SoundData &sound) {
  return sound.has_preset() || sound.has_macro() || sound.has_machine();
}

bool p4_sound_has_route_defaults(const TbdP4SoundData &sound) {
  return sound.trig_gate_time_ms != 0 &&
         sound.mixer_params.num_pages != 0 &&
         sound.mixer_params.params[0].type != TBD_P4_PARAM_TYPE_NONE;
}

bool p4_sound_matches_track(const TbdP4SoundData &sound,
                            uint8_t p4_track_index) {
  return sound.version == TBD_P4_SOUND_DATA_VERSION &&
         sound.p4_track_index == p4_track_index;
}

bool p4_sound_usable_for_track(const TbdP4SoundData &sound,
                               uint8_t p4_track_index) {
  return p4_sound_matches_track(sound, p4_track_index) &&
         (p4_sound_has_route_defaults(sound) ||
          p4_sound_has_identity(sound) ||
          p4_sound_has_audio_params(sound));
}

uint8_t p4_sound_detail_score(const TbdP4SoundData &sound) {
  uint8_t score = 0;
  if (p4_sound_has_route_defaults(sound)) score++;
  if (p4_sound_has_identity(sound)) score += 2;
  if (p4_sound_has_audio_params(sound)) score += 4;
  if (sound.has_sendable_params()) score++;
  return score;
}

bool p4_sound_same_identity(const TbdP4SoundData &a,
                            const TbdP4SoundData &b) {
  return strncmp(a.preset_id, b.preset_id, sizeof(a.preset_id)) == 0 &&
         strncmp(a.macro_id, b.macro_id, sizeof(a.macro_id)) == 0 &&
         strncmp(a.machine_id, b.machine_id, sizeof(a.machine_id)) == 0 &&
         a.rom_bank == b.rom_bank &&
         a.sample_slice == b.sample_slice;
}

bool p4_sound_should_replace(const TbdP4SoundData &candidate,
                             const TbdP4SoundData &selected,
                             uint8_t p4_track_index) {
  if (!p4_sound_usable_for_track(candidate, p4_track_index)) {
    return false;
  }
  if (!p4_sound_usable_for_track(selected, p4_track_index)) {
    return true;
  }

  if (p4_sound_detail_score(candidate) >= p4_sound_detail_score(selected)) {
    return true;
  }

  return p4_sound_has_identity(candidate) &&
         !p4_sound_same_identity(candidate, selected);
}

void ensure_sound_default(TbdP4SoundData &sound,
                          const TbdP4SoundData &default_sound) {
  if (!p4_sound_matches_track(sound, default_sound.p4_track_index)) {
    sound = default_sound;
    return;
  }

  if (!p4_sound_has_route_defaults(sound)) {
    tbd_init_p4_sound_runtime_defaults(sound);
  }

  if (p4_sound_has_identity(sound) && !p4_sound_has_audio_params(sound)) {
    tbd_hydrate_p4_sound(sound);
  }

  if (p4_sound_detail_score(default_sound) > p4_sound_detail_score(sound) &&
      (!p4_sound_has_identity(sound) ||
       p4_sound_same_identity(sound, default_sound))) {
    sound = default_sound;
  }
}

TbdP4SoundData g_last_applied_sounds[TBD_P4_SOUND_TRACK_COUNT];
bool g_last_applied_valid[TBD_P4_SOUND_TRACK_COUNT] = {};
TbdP4SoundData g_tbd_midi_runtime_sounds[NUM_EXT_TRACKS];

bool p4_sound_same_command(const TbdP4SoundData &a,
                           const TbdP4SoundData &b) {
  return a.p4_track_index == b.p4_track_index &&
         strncmp(a.preset_id, b.preset_id, sizeof(a.preset_id)) == 0 &&
         strncmp(a.macro_id, b.macro_id, sizeof(a.macro_id)) == 0 &&
         strncmp(a.machine_id, b.machine_id, sizeof(a.machine_id)) == 0 &&
         a.rom_bank == b.rom_bank &&
         a.sample_slice == b.sample_slice;
}

bool p4_sound_same_state(const TbdP4SoundData &a,
                         const TbdP4SoundData &b) {
  return memcmp(&a, &b, sizeof(TbdP4SoundData)) == 0;
}

bool p4_last_applied_sound(uint8_t p4_track_index, TbdP4SoundData *sound) {
  if (p4_track_index >= TBD_P4_SOUND_TRACK_COUNT ||
      !g_last_applied_valid[p4_track_index]) {
    return false;
  }
  if (sound != nullptr) {
    *sound = g_last_applied_sounds[p4_track_index];
  }
  return true;
}

void debug_p4_sound_apply(const char *source, uint8_t mcl_track,
                          GridSlot slotnumber,
                          const TbdP4SoundData &sound) {
#ifdef DEBUGMODE
  DEBUG_PRINT("TBD_P4_APPLY src=");
  DEBUG_PRINT(source == nullptr ? "?" : source);
  DEBUG_PRINT(" mcl=");
  DEBUG_PRINT((unsigned)mcl_track);
  DEBUG_PRINT(" slot=");
  if (slotnumber == 255) {
    DEBUG_PRINT("-");
  } else {
    DEBUG_PRINT((unsigned)slotnumber);
  }
  DEBUG_PRINT(" p4=");
  DEBUG_PRINT((unsigned)sound.p4_track_index);
  DEBUG_PRINT(" ch=");
  DEBUG_PRINT((unsigned)sound.midi_channel);
  DEBUG_PRINT(" preset=");
  DEBUG_PRINT(sound.preset_id[0] ? sound.preset_id : "-");
  DEBUG_PRINT(" macro=");
  DEBUG_PRINT(sound.macro_id[0] ? sound.macro_id : "-");
  DEBUG_PRINT(" machine=");
  DEBUG_PRINT(sound.machine_id[0] ? sound.machine_id : "-");
  DEBUG_PRINT(" rom=");
  DEBUG_PRINT((unsigned)sound.rom_bank);
  DEBUG_PRINT(" slice=");
  DEBUG_PRINT((long)sound.sample_slice);
  DEBUG_PRINT(" cmd=");
  if (sound.has_preset()) {
    DEBUG_PRINT("preset");
  } else if (sound.has_machine()) {
    DEBUG_PRINT("machine");
  } else if (sound.has_macro()) {
    DEBUG_PRINT("macro");
  } else {
    DEBUG_PRINT("state");
  }
  DEBUG_PRINT(" params=");
  DEBUG_PRINTLN((unsigned)sound.has_sendable_params());
#else
  (void)source;
  (void)mcl_track;
  (void)slotnumber;
  (void)sound;
#endif
}

void debug_p4_sound_apply_result(const char *source, uint8_t mcl_track,
                                 uint8_t p4_track_index, bool applied) {
#ifdef DEBUGMODE
  DEBUG_PRINT("TBD_P4_APPLY_RESULT src=");
  DEBUG_PRINT(source == nullptr ? "?" : source);
  DEBUG_PRINT(" mcl=");
  DEBUG_PRINT((unsigned)mcl_track);
  DEBUG_PRINT(" p4=");
  DEBUG_PRINT((unsigned)p4_track_index);
  DEBUG_PRINT(" ok=");
  DEBUG_PRINTLN(applied ? 1 : 0);
#else
  (void)source;
  (void)mcl_track;
  (void)p4_track_index;
  (void)applied;
#endif
}

void debug_p4_sound_apply_skip(const char *source, uint8_t mcl_track,
                               uint8_t p4_track_index,
                               const char *reason) {
#ifdef DEBUGMODE
  DEBUG_PRINT("TBD_P4_APPLY_SKIP src=");
  DEBUG_PRINT(source == nullptr ? "?" : source);
  DEBUG_PRINT(" mcl=");
  DEBUG_PRINT((unsigned)mcl_track);
  DEBUG_PRINT(" p4=");
  DEBUG_PRINT((unsigned)p4_track_index);
  DEBUG_PRINT(" reason=");
  DEBUG_PRINTLN(reason == nullptr ? "?" : reason);
#else
  (void)source;
  (void)mcl_track;
  (void)p4_track_index;
  (void)reason;
#endif
}

void apply_p4_sound(const TbdP4SoundData &sound, const char *source,
                    uint8_t mcl_track, GridSlot slotnumber) {
  debug_p4_sound_apply(source, mcl_track, slotnumber, sound);
  tbd_p4_realtime.set_active_track(sound.p4_track_index);

  TbdP4SoundData last_sound;
  const bool has_last =
      p4_last_applied_sound(sound.p4_track_index, &last_sound);
  if (has_last && p4_sound_same_state(sound, last_sound)) {
    debug_p4_sound_apply_skip(source, mcl_track, sound.p4_track_index,
                              "same-state");
    debug_p4_sound_apply_result(source, mcl_track, sound.p4_track_index, true);
    return;
  }

  const bool command_needed =
      !has_last || !p4_sound_same_command(sound, last_sound);
  bool applied = true;
  bool loaded_preset_command = false;
  if (command_needed) {
    if (sound.has_preset()) {
      applied = tbd_p4_command.load_track_sound_preset(sound.p4_track_index,
                                                       sound.preset_id,
                                                       sound.rom_bank,
                                                       sound.sample_slice,
                                                       kPresetApplyTimeoutMs);
      loaded_preset_command = applied;
    } else {
      if (sound.has_machine()) {
        applied = tbd_p4_command.activate_track_machine(sound.p4_track_index,
                                                        sound.machine_id,
                                                        kPresetApplyTimeoutMs);
      }
      if (applied && sound.has_macro()) {
        applied = tbd_p4_command.load_track_macro_definition(
            sound.p4_track_index, sound.macro_id, kPresetApplyTimeoutMs);
      }
    }
  } else {
    debug_p4_sound_apply_skip(source, mcl_track, sound.p4_track_index,
                              "same-command");
  }
  debug_p4_sound_apply_result(source, mcl_track, sound.p4_track_index, applied);
  if (applied) {
    if (loaded_preset_command) {
      tbd_p4_send_sound_mixer_state(sound);
    } else {
      tbd_p4_send_sound_state(sound);
    }
    tbd_mark_p4_sound_applied(sound);
  }
}

} // namespace

TbdP4SoundData *tbd_midi_runtime_sound(uint8_t track) {
  if (track >= NUM_EXT_TRACKS) {
    return nullptr;
  }
  return &g_tbd_midi_runtime_sounds[track];
}

const TbdP4SoundData *tbd_midi_runtime_sound_const(uint8_t track) {
  return tbd_midi_runtime_sound(track);
}

void tbd_mark_p4_sound_applied(const TbdP4SoundData &sound) {
  if (sound.p4_track_index >= TBD_P4_SOUND_TRACK_COUNT) {
    return;
  }
  g_last_applied_sounds[sound.p4_track_index] = sound;
  g_last_applied_valid[sound.p4_track_index] = true;
}

const TbdTrackDefault &tbd_track_default_for_slot(uint8_t slot) {
  if (slot >= sizeof(kTbdMidiTrackDefaults) / sizeof(kTbdMidiTrackDefaults[0])) {
    slot = 0;
  }
  return kTbdMidiTrackDefaults[slot];
}

void tbd_set_step_sound_default(TbdP4SoundData &sound, uint8_t slot) {
  set_step_sound_default(sound, slot);
}

void tbd_set_midi_sound_default(TbdP4SoundData &sound, uint8_t slot) {
  set_midi_sound_default(sound, slot);
}

void tbd_ensure_step_sound_default(TbdP4SoundData &sound, uint8_t slot) {
  TbdP4SoundData default_sound;
  set_step_sound_default(default_sound, slot);
  ensure_sound_default(sound, default_sound);
}

void tbd_ensure_midi_sound_default(TbdP4SoundData &sound, uint8_t slot) {
  TbdP4SoundData default_sound;
  set_midi_sound_default(default_sound, slot);
  ensure_sound_default(sound, default_sound);
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
  const int16_t level_default = sound.p4_track_index == 15 ? 1 : 64;
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
  tbd_p4_set_track_length(sound, TBD_P4_DEFAULT_TRACK_LENGTH);
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
  static_assert(MEMORY_ALIGN(sizeof(TBDTrack) - sizeof(void *)) <= TBD_TRACK_LEN);
}

uint16_t TBDTrack::grid_slot_label(GridSlotLabelContext ctx) {
  char label[3];
  label[0] = 'T';
  label[1] = 'B';
  label[2] = '\0';
  EmptyTrack scratch;
  if (auto *track = scratch.load_from_grid<TBDTrack>(ctx.slot, ctx.row)) {
    if (tbd_p4_copy_sound_label(track->p4_sound, label, 3, 2) &&
        label[1] == '\0') {
      label[1] = ' ';
      label[2] = '\0';
    }
  }
  return make_grid_slot_label(label[0], label[1]);
}

void TBDTrack::apply_seq_defaults(uint8_t tracknumber, SeqTrack *seq_track) {
  tbd_ensure_step_sound_default(p4_sound, tracknumber);

  if (seq_data.track_length == 0) {
    seq_data.track_length = 16;
  }
  tbd_p4_set_track_length(p4_sound, seq_data.track_length);
  if (seq_data.track_speed == 0xFF) {
    seq_data.track_speed = STEPSEQ_SPEED_1X;
  }

  if (seq_track != nullptr) {
    auto *tbd_track = static_cast<TBDSeqTrack *>(seq_track);
    tbd_track->p4_sound = p4_sound;
  }
}

void TBDTrack::init(uint8_t tracknumber, SeqTrack *seq_track) {
  seq_data.init_storage();
  set_step_sound_default(p4_sound, tracknumber);
  link.set_speed(STEPSEQ_SPEED_1X);
  link.length = 16;
  apply_seq_defaults(tracknumber, seq_track);
}

uint16_t TBDTrack::calc_latency(uint8_t tracknumber) {
  (void)tracknumber;
  return 0;
}

void TBDTrack::apply_preset(uint8_t fallback_tracknumber, const char *source,
                            GridSlot slotnumber) {
  tbd_ensure_step_sound_default(p4_sound, fallback_tracknumber);
  apply_p4_sound(p4_sound, source, fallback_tracknumber, slotnumber);
}

void TBDTrack::transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                               GridSlot slotnumber) {
  GridTrack::transition_load(tracknumber, seq_track, slotnumber);
  load_seq_data(seq_track);
  apply_seq_defaults(tracknumber, seq_track);
}

void TBDTrack::transition_send(uint8_t tracknumber, GridSlot slotnumber) {
  apply_preset(tracknumber, "step.transition_send", slotnumber);
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
  tbd_track->set_speed(seq_data.track_speed == 0xFF ? link.speed_value()
                                                    : seq_data.track_speed);

  SeqTrack::load_mod_data(seq_track, seq_data.mod(), true);
}

void TBDTrack::load_immediate(uint8_t tracknumber, SeqTrack *seq_track) {
  load_seq_data(seq_track);
  apply_seq_defaults(tracknumber, seq_track);
  apply_preset(tracknumber, "step.load_immediate", 255);
}

void TBDTrack::load_immediate_cleared(uint8_t tracknumber,
                                      SeqTrack *seq_track) {
  load_seq_data(seq_track);
  apply_seq_defaults(tracknumber, seq_track);
}

bool TBDTrack::store_in_grid(GridSlot column, GridRow row, SeqTrack *seq_track,
                             uint8_t merge, bool online, Grid *grid) {
  (void)merge;
  (void)online;

  active = TBD_TRACK_TYPE;

  const GridColumn slot = column & 0x0F;
  SeqTrack::store_mod_data(seq_data.mod(), true, slot);
  set_step_sound_default(p4_sound, slot);
  const uint8_t p4_track_index = p4_sound.p4_track_index;

  EmptyTrack scratch;
  if (auto *cached = scratch.load_from_mem<TBDTrack>(slot)) {
    if (p4_sound_should_replace(cached->p4_sound, p4_sound,
                                p4_track_index)) {
      memcpy(&p4_sound, &cached->p4_sound, sizeof(p4_sound));
    }
  }

  if (seq_track != nullptr) {
    link.length = seq_track->length;
    link.set_speed(seq_track->speed);
    auto *tbd_track = static_cast<TBDSeqTrack *>(seq_track);
    if (p4_sound_should_replace(tbd_track->p4_sound, p4_sound,
                                p4_track_index)) {
      p4_sound = tbd_track->p4_sound;
    }
    memcpy(seq_data.data(), tbd_track->TBDSeqTrackData::data(),
           sizeof(TBDSeqTrackData));
    seq_data.track_length = seq_track->length;
    seq_data.track_speed = seq_track->speed;
  }

  apply_seq_defaults(slot, seq_track);

  return write_grid(_this(), get_store_size(), column, row, grid);
}

TBDMidiTrack::TBDMidiTrack() {
  active = TBD_MIDI_TRACK_TYPE;
  static_assert(MEMORY_ALIGN(sizeof(TBDMidiTrack) - sizeof(void *)) <=
                GRID2_TRACK_LEN);
}

uint16_t TBDMidiTrack::grid_slot_label(GridSlotLabelContext ctx) {
  char label[3];
  label[0] = 'T';
  label[1] = 'M';
  label[2] = '\0';
  EmptyTrack scratch;
  if (auto *track = scratch.load_from_grid<TBDMidiTrack>(ctx.slot, ctx.row)) {
    if (tbd_p4_copy_sound_label(track->p4_sound, label, 3, 2) &&
        label[1] == '\0') {
      label[1] = ' ';
      label[2] = '\0';
    }
  }
  return make_grid_slot_label(label[0], label[1]);
}

void TBDMidiTrack::apply_seq_defaults(uint8_t tracknumber,
                                      SeqTrack *seq_track) {
  tbd_ensure_midi_sound_default(p4_sound, tracknumber);

  if (seq_data.version != MIDI_SEQ_DATA_VERSION) {
    uint8_t fallback_speed = midi_seq_valid_speed(link.speed_value());
    seq_data.clear();
    seq_data.speed = fallback_speed;
  }
  if (seq_data.length == 0) {
    seq_data.length = link.length ? link.length : 16;
  }
  seq_data.speed = midi_seq_valid_speed(seq_data.speed);
  seq_data.channel = p4_sound.midi_channel;
  tbd_p4_set_track_length(p4_sound, seq_data.length);

  if (seq_track != nullptr) {
    auto *midi_track = static_cast<MidiSeqTrack *>(seq_track);
    if (TbdP4SoundData *runtime_sound = tbd_midi_runtime_sound(tracknumber)) {
      *runtime_sound = p4_sound;
    }
    midi_track->active = TBD_MIDI_TRACK_TYPE;
    midi_track->seq_data.channel = p4_sound.midi_channel;
    midi_track->set_channel(p4_sound.midi_channel);
    midi_track->set_length(seq_data.length ? seq_data.length : link.length);
    midi_track->set_speed(seq_data.speed);
  }
}

void TBDMidiTrack::init(uint8_t tracknumber, SeqTrack *seq_track) {
  seq_data.clear_storage();
  set_midi_sound_default(p4_sound, tracknumber);
  link.set_speed(SEQ_SPEED_1X);
  link.length = 16;
  apply_seq_defaults(tracknumber, seq_track);
}

uint16_t TBDMidiTrack::calc_latency(uint8_t tracknumber) {
  (void)tracknumber;
  return 0;
}

void TBDMidiTrack::apply_preset(uint8_t fallback_tracknumber,
                                const char *source, GridSlot slotnumber) {
  tbd_ensure_midi_sound_default(p4_sound, fallback_tracknumber);
  apply_p4_sound(p4_sound, source, fallback_tracknumber, slotnumber);
}

void TBDMidiTrack::transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                                   GridSlot slotnumber) {
  GridTrack::transition_load(tracknumber, seq_track, slotnumber);
  if (seq_track == nullptr || seq_track->count_down == 0) {
    load_seq_data(seq_track);
    apply_seq_defaults(tracknumber, seq_track);
    return;
  }
  static_cast<MidiSeqTrack *>(seq_track)->defer_cache_load(active, tracknumber);
}

void TBDMidiTrack::transition_send(uint8_t tracknumber, GridSlot slotnumber) {
  apply_preset(tracknumber, "midi.transition_send", slotnumber);
}

void TBDMidiTrack::load_immediate(uint8_t tracknumber, SeqTrack *seq_track) {
  load_seq_data(seq_track);
  apply_seq_defaults(tracknumber, seq_track);
  apply_preset(tracknumber, "midi.load_immediate", 255);
}

void TBDMidiTrack::load_immediate_cleared(uint8_t tracknumber,
                                          SeqTrack *seq_track) {
  load_seq_data(seq_track);
  apply_seq_defaults(tracknumber, seq_track);
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
    uint8_t fallback_speed = midi_seq_valid_speed(link.speed_value());
    seq_data.clear();
    seq_data.speed = fallback_speed;
  }
  if (seq_data.length == 0) {
    seq_data.length = link.length ? link.length : 16;
  }
  seq_data.speed = midi_seq_valid_speed(seq_data.speed);
  if (seq_data.channel >= 16) {
    seq_data.channel = midi_track->track_number;
  }
  load_link_data(seq_track);
  midi_track->active = TBD_MIDI_TRACK_TYPE;
  midi_track->seq_data = static_cast<const MidiSeqTrackData &>(seq_data);
  if (TbdP4SoundData *runtime_sound =
          tbd_midi_runtime_sound(midi_track->track_number)) {
    *runtime_sound = p4_sound;
  }
  midi_track->set_channel(p4_sound.midi_channel);
  midi_track->set_length(seq_data.length ? seq_data.length : link.length);
  midi_track->set_speed(midi_seq_valid_speed(seq_data.speed));
  midi_track->mute_state = old_mute;

  SeqTrack::load_mod_data(seq_track, seq_data.mod(), false);
}

bool TBDMidiTrack::can_materialize_as(uint8_t track_type) {
  if (midi_track_type_is_storage_family(track_type)) {
    return true;
  }
  return DeviceTrack::can_materialize_as(track_type);
}

bool TBDMidiTrack::materialized_storage_range(uint8_t track_type,
                                              uint16_t &source_offset,
                                              uint16_t &target_offset,
                                              uint16_t &len) {
  if (track_type != MIDI_TRACK_TYPE) {
    return false;
  }
  source_offset =
      reinterpret_cast<uintptr_t>(&seq_data) -
      reinterpret_cast<uintptr_t>(_this());
  target_offset = DEVICE_TRACK_LEN;
  len = sizeof(MidiSeqTrackStorage);
  return true;
}

DeviceTrack *TBDMidiTrack::materialize_as(uint8_t track_type,
                                          uint8_t tracknumber,
                                          SeqTrack *seq_track) {
  (void)seq_track;
  if (active == track_type) {
    return this;
  }
  if (midi_track_type_is_storage_family(track_type)) {
    return materialize_midi_storage_track(this, track_type, link, seq_data,
                                          tracknumber);
  }
  return DeviceTrack::materialize_as(track_type, tracknumber, seq_track);
}

bool TBDMidiTrack::store_in_grid(GridSlot column, GridRow row,
                                 SeqTrack *seq_track, uint8_t merge,
                                 bool online, Grid *grid) {
  (void)merge;
  (void)online;

  active = TBD_MIDI_TRACK_TYPE;

  const GridColumn slot = column & 0x0F;
  SeqTrack::store_mod_data(seq_data.mod(), false, slot);
  set_midi_sound_default(p4_sound, slot);
  const uint8_t p4_track_index = p4_sound.p4_track_index;

  EmptyTrack scratch;
  if (auto *cached = scratch.load_from_mem<TBDMidiTrack>(slot)) {
    if (p4_sound_should_replace(cached->p4_sound, p4_sound,
                                p4_track_index)) {
      memcpy(&p4_sound, &cached->p4_sound, sizeof(p4_sound));
    }
  }

  if (seq_track != nullptr) {
    link.length = seq_track->length;
    link.set_speed(seq_track->speed);
    auto *midi_track = static_cast<MidiSeqTrack *>(seq_track);
    TbdP4SoundData *runtime_sound =
        tbd_midi_runtime_sound(midi_track->track_number);
    if (runtime_sound != nullptr &&
        p4_sound_should_replace(*runtime_sound, p4_sound, p4_track_index)) {
      p4_sound = *runtime_sound;
    }
    static_cast<MidiSeqTrackData &>(seq_data) = midi_track->seq_data;
    seq_data.length = seq_track->length;
    seq_data.speed = seq_track->speed;
  }

  apply_seq_defaults(slot, seq_track);

  return write_grid(_this(), get_store_size(), column, row, grid);
}

#endif // PLATFORM_TBD
