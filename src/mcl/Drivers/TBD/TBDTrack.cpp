#include "TBDTrack.h"

#ifdef PLATFORM_TBD

#include "EmptyTrack.h"
#include "ExtSeqTrack.h"
#include "TbdP4Command.h"
#include "TbdP4Realtime.h"
#include <string.h>

namespace {

constexpr uint8_t kTbdTrackDataVersion = 1;
constexpr uint8_t kDefaultRomBank = 0xFF;
constexpr int32_t kDefaultSampleSlice = -1;
constexpr uint32_t kPresetApplyTimeoutMs = 3000;

TbdTrackDefault kTbdTrackDefaults[] = {
    {8, 0, "td3-all-def", kDefaultRomBank, kDefaultSampleSlice},
    {9, 1, "td3-all-def", kDefaultRomBank, kDefaultSampleSlice},
    {10, 2, "mo-all-def", kDefaultRomBank, kDefaultSampleSlice},
    {11, 3, "wtosc-all-def", kDefaultRomBank, kDefaultSampleSlice},
    {12, 4, "ro-all-def", kDefaultRomBank, kDefaultSampleSlice},
    {13, 5, "ro-all-def", kDefaultRomBank, kDefaultSampleSlice},
};

void copy_preset_id(char *dst, const char *src) {
  if (src == nullptr) {
    dst[0] = '\0';
    return;
  }
  strncpy(dst, src, TBD_PRESET_ID_LEN);
  dst[TBD_PRESET_ID_LEN - 1] = '\0';
}

} // namespace

const TbdTrackDefault &tbd_track_default_for_slot(uint8_t slot) {
  if (slot >= sizeof(kTbdTrackDefaults) / sizeof(kTbdTrackDefaults[0])) {
    slot = 0;
  }
  return kTbdTrackDefaults[slot];
}

void tbd_update_track_default_from_p4(uint8_t p4_track_index,
                                      const char *preset_id,
                                      uint8_t rom_bank,
                                      int32_t sample_slice) {
  if (preset_id == nullptr || preset_id[0] == '\0') {
    return;
  }

  for (auto &def : kTbdTrackDefaults) {
    if (def.p4_track_index == p4_track_index) {
      copy_preset_id(def.preset_id, preset_id);
      def.rom_bank = rom_bank;
      def.sample_slice = sample_slice;
      return;
    }
  }
}

void TBDTrackData::clear() {
  version = kTbdTrackDataVersion;
  p4_track_index = 8;
  midi_channel = 0;
  rom_bank = kDefaultRomBank;
  sample_slice = kDefaultSampleSlice;
  preset_id[0] = '\0';
}

void TBDTrackData::set_default(uint8_t slot) {
  const auto &def = tbd_track_default_for_slot(slot);
  version = kTbdTrackDataVersion;
  p4_track_index = def.p4_track_index;
  midi_channel = def.midi_channel;
  rom_bank = def.rom_bank;
  sample_slice = def.sample_slice;
  copy_preset_id(preset_id, def.preset_id);
}

TBDTrack::TBDTrack() {
  active = TBD_TRACK_TYPE;
  p4_preset.set_default(0);
  static_assert(sizeof(TBDTrack) <= GRID2_TRACK_LEN);
}

void TBDTrack::apply_seq_defaults(uint8_t tracknumber, SeqTrack *seq_track) {
  if (p4_preset.version != kTbdTrackDataVersion || !p4_preset.has_preset()) {
    p4_preset.set_default(tracknumber);
  }

  seq_data.channel = p4_preset.midi_channel;
  if (seq_track != nullptr) {
    auto *ext_track = (ExtSeqTrack *)seq_track;
    ext_track->channel = p4_preset.midi_channel;
  }
}

void TBDTrack::init(uint8_t tracknumber, SeqTrack *seq_track) {
  ExtTrack::init(tracknumber, seq_track);
  p4_preset.set_default(tracknumber);
  apply_seq_defaults(tracknumber, seq_track);
}

uint16_t TBDTrack::calc_latency(uint8_t tracknumber) {
  (void)tracknumber;
  return 2048;
}

void TBDTrack::apply_preset(uint8_t fallback_tracknumber) {
  if (p4_preset.version != kTbdTrackDataVersion || !p4_preset.has_preset()) {
    p4_preset.set_default(fallback_tracknumber);
  }

  tbd_p4_realtime.set_active_track(p4_preset.p4_track_index);
  tbd_p4_command.load_track_sound_preset(p4_preset.p4_track_index,
                                         p4_preset.preset_id,
                                         p4_preset.rom_bank,
                                         p4_preset.sample_slice,
                                         kPresetApplyTimeoutMs);
}

void TBDTrack::transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                               uint8_t slotnumber) {
  transition_load_device(tracknumber, seq_track, slotnumber);
  apply_seq_defaults(tracknumber, seq_track);
}

void TBDTrack::transition_send(uint8_t tracknumber, uint8_t slotnumber) {
  (void)slotnumber;
  apply_preset(tracknumber);
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
  p4_preset.set_default(slot);

  EmptyTrack scratch;
  if (auto *cached = scratch.load_from_mem<TBDTrack>(slot)) {
    memcpy(&p4_preset, &cached->p4_preset, sizeof(p4_preset));
  }

  apply_seq_defaults(slot, seq_track);

  if (seq_track != nullptr) {
    link.length = seq_track->length;
    link.speed = seq_track->speed;
    auto *ext_track = (ExtSeqTrack *)seq_track;
    memcpy(&seq_data, ext_track->data(), sizeof(seq_data));
  }

  return write_grid(_this(), _sizeof(), column, row, grid);
}

#endif // PLATFORM_TBD
