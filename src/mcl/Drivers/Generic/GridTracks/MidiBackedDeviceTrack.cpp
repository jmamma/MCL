#include "MidiBackedDeviceTrack.h"

#if !defined(__AVR__)

#include "MidiTrack.h"
#ifdef PLATFORM_TBD
#include "../../TBD/TBDTrack.h"
#endif
#include <string.h>

namespace {

uint8_t midi_device_track_valid_speed(uint8_t speed) {
  return speed <= SEQ_SPEED_4X ? speed : SEQ_SPEED_1X;
}

} // namespace

MidiBackedDeviceTrack::MidiBackedDeviceTrack() {
  seq_data.clear_storage();
}

void MidiBackedDeviceTrack::apply_seq_defaults(uint8_t tracknumber,
                                               SeqTrack *seq_track) {
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
  if (seq_data.channel >= 16) {
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
  seq_data.clear_storage();
  link.set_speed(SEQ_SPEED_1X);
  link.length = 16;
  seq_data.channel = tracknumber;
  apply_seq_defaults(tracknumber, seq_track);
}

void MidiBackedDeviceTrack::transition_load(uint8_t tracknumber,
                                            SeqTrack *seq_track,
                                            GridSlot slotnumber) {
  GridTrack::transition_load(tracknumber, seq_track, slotnumber);
  load_seq_data(seq_track);
  apply_seq_defaults(tracknumber, seq_track);
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
  uint8_t old_mute = midi_track->mute_state;
  midi_track->mute_state = SEQ_MUTE_ON;
  midi_track->notesoff_pending = true;

  if (seq_data.version != MIDI_SEQ_DATA_VERSION) {
    uint8_t fallback_speed = midi_device_track_valid_speed(link.speed_value());
    seq_data.clear();
    seq_data.speed = fallback_speed;
  }

  load_link_data(seq_track);
  midi_track->active = active;
  midi_track->seq_data = static_cast<const MidiSeqTrackData &>(seq_data);
  midi_track->set_channel(seq_data.channel);
  midi_track->set_length(seq_data.length ? seq_data.length : link.length);
  midi_track->set_speed(midi_device_track_valid_speed(seq_data.speed));
  midi_track->mute_state = old_mute;

  SeqTrack::load_mod_data(
      seq_track, seq_data.mod(), false,
      storage_version_at_least(SEQ_TRACK_MOD_STORAGE_VERSION));
}

void MidiBackedDeviceTrack::import_legacy_ext_storage(
    const GridLink &old_link, const ExtSeqTrackData &old_seq_data,
    const SeqTrackModData &old_mod_data, uint8_t tracknumber) {
  link = old_link;
  seq_data.clear_storage();
  seq_data.mod() = old_mod_data;
  seq_data.import_legacy_ext(old_seq_data, old_link);
  seq_data.channel = old_seq_data.channel < 16 ? old_seq_data.channel : tracknumber;
}

bool MidiBackedDeviceTrack::can_materialize_as(uint8_t track_type) {
  if (track_type == MIDI_TRACK_TYPE) {
    return true;
  }
#ifdef PLATFORM_TBD
  if (track_type == TBD_MIDI_TRACK_TYPE) {
    return true;
  }
#endif
  return DeviceTrack::can_materialize_as(track_type);
}

DeviceTrack *MidiBackedDeviceTrack::materialize_as(uint8_t track_type,
                                                   uint8_t tracknumber,
                                                   SeqTrack *seq_track) {
  (void)seq_track;
  if (active == track_type) {
    return this;
  }

  GridLink old_link = link;
  MidiSeqTrackStorage old_seq_data = seq_data;
  if (old_seq_data.channel >= 16) {
    old_seq_data.channel = tracknumber;
  }

  if (track_type == MIDI_TRACK_TYPE) {
    auto *midi_track =
        static_cast<MidiTrack *>(init_track_type(MIDI_TRACK_TYPE));
    midi_track->link = old_link;
    midi_track->seq_data = old_seq_data;
    return midi_track;
  }

#ifdef PLATFORM_TBD
  if (track_type == TBD_MIDI_TRACK_TYPE) {
    auto *tbd_midi_track =
        static_cast<TBDMidiTrack *>(init_track_type(TBD_MIDI_TRACK_TYPE));
    tbd_midi_track->init(tracknumber, nullptr);
    tbd_midi_track->link = old_link;
    tbd_midi_track->seq_data = old_seq_data;
    tbd_midi_track->p4_sound.midi_channel = old_seq_data.channel;
    return tbd_midi_track;
  }
#endif

  return DeviceTrack::materialize_as(track_type, tracknumber, seq_track);
}

void MidiBackedDeviceTrack::store_midi_seq_data(GridSlot column,
                                                SeqTrack *seq_track) {
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
