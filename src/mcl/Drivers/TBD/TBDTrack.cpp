#include "TBDTrack.h"

#ifdef PLATFORM_TBD

#include "EmptyTrack.h"
#include "ExtSeqTrack.h"
#include "TbdP4Command.h"
#include "TbdP4Realtime.h"
#include <string.h>

namespace {

constexpr uint8_t kDefaultRomBank = 0xFF;
constexpr int32_t kDefaultSampleSlice = -1;
constexpr uint32_t kPresetApplyTimeoutMs = 3000;

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

void set_sound_from_default(TbdP4SoundData &sound, const TbdTrackDefault &def) {
  sound.clear();
  sound.p4_track_index = def.p4_track_index;
  sound.midi_channel = def.midi_channel;
  sound.rom_bank = def.rom_bank;
  sound.sample_slice = def.sample_slice;
  copy_fixed_string(sound.preset_id, sizeof(sound.preset_id), def.preset_id);
}

void set_step_sound_default(TbdP4SoundData &sound, uint8_t slot) {
  if (slot >= sizeof(kTbdStepTrackDefaults) / sizeof(kTbdStepTrackDefaults[0])) {
    slot = 0;
  }
  set_sound_from_default(sound, kTbdStepTrackDefaults[slot]);
}

void set_midi_sound_default(TbdP4SoundData &sound, uint8_t slot) {
  const auto &def = tbd_track_default_for_slot(slot);
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
  set_midi_sound_default(p4_sound, 0);
  static_assert(sizeof(TBDMidiTrack) <= GRID2_TRACK_LEN);
}

void TBDMidiTrack::apply_seq_defaults(uint8_t tracknumber,
                                      SeqTrack *seq_track) {
  if (p4_sound.version != TBD_P4_SOUND_DATA_VERSION || !p4_sound.has_preset()) {
    set_midi_sound_default(p4_sound, tracknumber);
  }

  seq_data.channel = p4_sound.midi_channel;
  if (seq_track != nullptr) {
    auto *ext_track = static_cast<ExtSeqTrack *>(seq_track);
    ext_track->channel = p4_sound.midi_channel;
  }
}

void TBDMidiTrack::init(uint8_t tracknumber, SeqTrack *seq_track) {
  ExtTrack::init(tracknumber, seq_track);
  set_midi_sound_default(p4_sound, tracknumber);
  apply_seq_defaults(tracknumber, seq_track);
}

uint16_t TBDMidiTrack::calc_latency(uint8_t tracknumber) {
  (void)tracknumber;
  return 2048;
}

void TBDMidiTrack::apply_preset(uint8_t fallback_tracknumber) {
  if (p4_sound.version != TBD_P4_SOUND_DATA_VERSION || !p4_sound.has_preset()) {
    set_midi_sound_default(p4_sound, fallback_tracknumber);
  }
  apply_p4_sound(p4_sound);
}

void TBDMidiTrack::transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                                   uint8_t slotnumber) {
  transition_load_device(tracknumber, seq_track, slotnumber);
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

bool TBDMidiTrack::store_in_grid(uint8_t column, uint16_t row,
                                 SeqTrack *seq_track, uint8_t merge,
                                 bool online, Grid *grid) {
  (void)merge;
  (void)online;

  active = TBD_MIDI_TRACK_TYPE;

  const uint8_t slot = column & 0x0F;
  set_midi_sound_default(p4_sound, slot);

  EmptyTrack scratch;
  if (auto *cached = scratch.load_from_mem<TBDMidiTrack>(slot)) {
    memcpy(&p4_sound, &cached->p4_sound, sizeof(p4_sound));
  }

  apply_seq_defaults(slot, seq_track);

  if (seq_track != nullptr) {
    link.length = seq_track->length;
    link.speed = seq_track->speed;
    auto *ext_track = static_cast<ExtSeqTrack *>(seq_track);
    memcpy(&seq_data, ext_track->data(), sizeof(seq_data));
  }

  return write_grid(_this(), _sizeof(), column, row, grid);
}

#endif // PLATFORM_TBD
