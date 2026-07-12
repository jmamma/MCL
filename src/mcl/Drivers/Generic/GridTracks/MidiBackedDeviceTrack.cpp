#include "MidiBackedDeviceTrack.h"

#if !defined(__AVR__)

#include "MidiTrack.h"
#include "MidiTrackMaterializer.h"
#include "Sequencer/PtcVoiceRouter.h"
#include "../Sequencer/StepSeqDefines.h"
#include <string.h>

namespace {

uint8_t midi_device_track_valid_speed(uint8_t speed) {
  return speed <= SEQ_SPEED_4X ? speed : SEQ_SPEED_1X;
}

} // namespace

MidiBackedDeviceTrack::MidiBackedDeviceTrack() {}

void MidiBackedDeviceTrack::apply_seq_defaults(uint8_t tracknumber,
                                               SeqTrack *seq_track) {
  MidiSeqTrackStorage &seq_data = midi_seq_storage();
  if (seq_data.version != MIDI_SEQ_DATA_VERSION) {
    uint8_t fallback_speed = midi_device_track_valid_speed(link.speed_value());
    seq_data.clear();
    seq_data.speed = fallback_speed;
    seq_data.channel = tracknumber;
  }
  if (seq_data.length == 0) {
    seq_data.length = link.length ? link.length : 16;
  }
  seq_data.speed = midi_device_track_valid_speed(seq_data.speed);
  if (seq_data.channel >= PTC_EXT_ROUTE_CHANNEL_END) {
    seq_data.channel = tracknumber;
  }

  if (seq_track != nullptr) {
    auto *midi_track = static_cast<MidiSeqTrack *>(seq_track);
    midi_track->active = active;
    midi_track->seq_data.channel = seq_data.channel;
    midi_track->set_channel(seq_data.channel);
    midi_track->set_length(seq_data.length ? seq_data.length : link.length);
    midi_track->set_speed(seq_data.speed);
  }
}

void MidiBackedDeviceTrack::init(uint8_t tracknumber, SeqTrack *seq_track) {
  MidiSeqTrackStorage &seq_data = midi_seq_storage();
  seq_data.clear_storage();
  load_fade.init();
  link.set_speed(SEQ_SPEED_1X);
  link.length = 16;
  seq_data.channel = tracknumber;
  apply_seq_defaults(tracknumber, seq_track);
}

uint8_t MidiBackedDeviceTrack::transition_countdown_resolution() {
  return STEPSEQ_SEQ_INTERPOLATION;
}

void MidiBackedDeviceTrack::transition_load(uint8_t tracknumber,
                                            SeqTrack *seq_track,
                                            GridSlot slotnumber) {
  GridTrack::transition_load(tracknumber, seq_track, slotnumber);
  if (seq_track == nullptr || seq_track->count_down == 0) {
    load_seq_data(seq_track);
    apply_seq_defaults(tracknumber, seq_track);
    return;
  }
  static_cast<MidiSeqTrack *>(seq_track)->defer_cache_load(active, tracknumber);
}

void MidiBackedDeviceTrack::load_immediate(uint8_t tracknumber,
                                           SeqTrack *seq_track) {
  load_seq_data(seq_track);
  apply_seq_defaults(tracknumber, seq_track);
}

void MidiBackedDeviceTrack::load_immediate_cleared(uint8_t tracknumber,
                                                   SeqTrack *seq_track) {
  load_seq_data(seq_track);
  apply_seq_defaults(tracknumber, seq_track);
}

void MidiBackedDeviceTrack::load_seq_data(SeqTrack *seq_track) {
  if (seq_track == nullptr) {
    return;
  }

  auto *midi_track = static_cast<MidiSeqTrack *>(seq_track);
  MidiSeqTrackStorage &seq_data = midi_seq_storage();
  uint8_t old_mute = midi_track->mute_state;
  midi_track->mute_state = SEQ_MUTE_ON;
  midi_track->notesoff_pending = true;

  if (seq_data.version != MIDI_SEQ_DATA_VERSION) {
    uint8_t fallback_speed = midi_device_track_valid_speed(link.speed_value());
    seq_data.clear();
    seq_data.speed = fallback_speed;
    seq_data.channel = midi_track->track_number;
  }
  if (seq_data.length == 0) {
    seq_data.length = link.length ? link.length : 16;
  }
  seq_data.speed = midi_device_track_valid_speed(seq_data.speed);
  if (seq_data.channel >= PTC_EXT_ROUTE_CHANNEL_END) {
    seq_data.channel = midi_track->track_number;
  }

  load_link_data(seq_track);
  midi_track->active = active;
  midi_track->seq_data = static_cast<const MidiSeqTrackData &>(seq_data);
  midi_track->set_channel(seq_data.channel);
  midi_track->set_length(seq_data.length ? seq_data.length : link.length);
  midi_track->set_speed(midi_device_track_valid_speed(seq_data.speed));
  midi_track->mute_state = old_mute;

  SeqTrack::load_mod_data(seq_track, seq_data.mod(), false);
}

void MidiBackedDeviceTrack::on_copy(GridColumn s_col, GridColumn d_col,
                                    bool destination_same) {
  midi_seq_storage().mod().remap_lfo_track_destinations(
      s_col, d_col, destination_same, NUM_GRID_Y_LFO_TRACKS);
}

void MidiBackedDeviceTrack::import_legacy_ext_storage(
    const GridLink &old_link, const ExtSeqTrackData &old_seq_data,
    const SeqTrackModData &old_mod_data, uint8_t tracknumber) {
  MidiSeqTrackStorage &seq_data = midi_seq_storage();
  link = old_link;
  seq_data.clear_storage();
  seq_data.mod() = old_mod_data;
  seq_data.import_legacy_ext(old_seq_data, old_link);
  seq_data.channel = old_seq_data.channel < PTC_EXT_ROUTE_CHANNEL_END
                         ? old_seq_data.channel
                         : tracknumber;
}

bool MidiBackedDeviceTrack::can_materialize_as(uint8_t track_type) {
  if (midi_track_type_is_storage_family(track_type)) {
    return true;
  }
  return DeviceTrack::can_materialize_as(track_type);
}

bool MidiBackedDeviceTrack::materialized_storage_range(
    uint8_t track_type, uint16_t &source_offset, uint16_t &target_offset,
    uint16_t &len) {
  if (track_type != MIDI_TRACK_TYPE) {
    return false;
  }
  source_offset =
      reinterpret_cast<uintptr_t>(&midi_seq_storage()) -
      reinterpret_cast<uintptr_t>(_this());
  target_offset = GridTrack::STORAGE_HEADER_SIZE;
  len = sizeof(MidiSeqTrackStorage);
  return true;
}

DeviceTrack *MidiBackedDeviceTrack::materialize_as(uint8_t track_type,
                                                   uint8_t tracknumber,
                                                   SeqTrack *seq_track) {
  (void)seq_track;
  if (active == track_type) {
    return this;
  }

  GridLink old_link = link;
  MidiSeqTrackStorage old_seq_data = midi_seq_storage();
  TrackLoadFadeData old_load_fade = load_fade;
  if (old_seq_data.channel >= PTC_EXT_ROUTE_CHANNEL_END) {
    old_seq_data.channel = tracknumber;
  }

  if (midi_track_type_is_storage_family(track_type)) {
    return materialize_midi_storage_track(this, track_type, old_link,
                                          old_seq_data, &old_load_fade,
                                          tracknumber);
  }

  return DeviceTrack::materialize_as(track_type, tracknumber, seq_track);
}

void MidiBackedDeviceTrack::store_midi_seq_data(GridSlot column,
                                                SeqTrack *seq_track) {
  MidiSeqTrackStorage &seq_data = midi_seq_storage();
  const GridColumn slot = column & 0x0F;
  SeqTrack::store_mod_data(seq_data.mod(), false, slot);

  if (seq_track != nullptr) {
    link.length = seq_track->length;
    link.set_speed(seq_track->speed);
    auto *midi_track = static_cast<MidiSeqTrack *>(seq_track);
    static_cast<MidiSeqTrackData &>(seq_data) = midi_track->seq_data;
    seq_data.channel = midi_track->channel();
    seq_data.length = seq_track->length;
    seq_data.speed = seq_track->speed;
  }

  apply_seq_defaults(slot, seq_track);
}

#endif // !defined(__AVR__)
