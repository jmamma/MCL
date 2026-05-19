#include "MidiTrack.h"

#if !defined(__AVR__)

#include "EmptyTrack.h"
#include <string.h>

namespace {

uint8_t midi_track_valid_speed(uint8_t speed) {
  return speed <= SEQ_SPEED_4X ? speed : SEQ_SPEED_1X;
}

} // namespace

MidiTrack::MidiTrack() {
  active = MIDI_TRACK_TYPE;
  seq_data.clear_storage();
  static_assert(MEMORY_ALIGN(sizeof(MidiTrack) - sizeof(void *)) <=
                GRID2_TRACK_LEN);
}

void MidiTrack::apply_seq_defaults(uint8_t tracknumber, SeqTrack *seq_track) {
  if (seq_data.version != MIDI_SEQ_DATA_VERSION) {
    uint8_t fallback_speed = midi_track_valid_speed(link.speed_value());
    seq_data.clear();
    seq_data.speed = fallback_speed;
    seq_data.channel = tracknumber;
  }
  if (seq_data.length == 0) {
    seq_data.length = link.length ? link.length : 16;
  }
  seq_data.speed = midi_track_valid_speed(seq_data.speed);
  if (seq_data.channel >= 16) {
    seq_data.channel = tracknumber;
  }

  if (seq_track != nullptr) {
    auto *midi_track = static_cast<MidiSeqTrack *>(seq_track);
    midi_track->active = MIDI_TRACK_TYPE;
    midi_track->seq_data.channel = seq_data.channel;
    midi_track->set_channel(seq_data.channel);
    midi_track->set_length(seq_data.length ? seq_data.length : link.length);
    midi_track->set_speed(seq_data.speed);
  }
}

void MidiTrack::init(uint8_t tracknumber, SeqTrack *seq_track) {
  seq_data.clear_storage();
  link.set_speed(SEQ_SPEED_1X);
  link.length = 16;
  seq_data.channel = tracknumber;
  apply_seq_defaults(tracknumber, seq_track);
}

void MidiTrack::transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                                GridSlot slotnumber) {
  GridTrack::transition_load(tracknumber, seq_track, slotnumber);
  load_seq_data(seq_track);
  apply_seq_defaults(tracknumber, seq_track);
}

void MidiTrack::load_immediate(uint8_t tracknumber, SeqTrack *seq_track) {
  load_seq_data(seq_track);
  apply_seq_defaults(tracknumber, seq_track);
}

void MidiTrack::load_immediate_cleared(uint8_t tracknumber,
                                       SeqTrack *seq_track) {
  load_seq_data(seq_track);
  apply_seq_defaults(tracknumber, seq_track);
}

void MidiTrack::load_seq_data(SeqTrack *seq_track) {
  if (seq_track == nullptr) {
    return;
  }

  auto *midi_track = static_cast<MidiSeqTrack *>(seq_track);
  uint8_t old_mute = midi_track->mute_state;
  midi_track->mute_state = SEQ_MUTE_ON;
  midi_track->notesoff_pending = true;

  if (seq_data.version != MIDI_SEQ_DATA_VERSION) {
    uint8_t fallback_speed = midi_track_valid_speed(link.speed_value());
    seq_data.clear();
    seq_data.speed = fallback_speed;
  }

  load_link_data(seq_track);
  midi_track->active = MIDI_TRACK_TYPE;
  midi_track->seq_data = static_cast<const MidiSeqTrackData &>(seq_data);
  midi_track->set_channel(seq_data.channel);
  midi_track->set_length(seq_data.length ? seq_data.length : link.length);
  midi_track->set_speed(midi_track_valid_speed(seq_data.speed));
  midi_track->mute_state = old_mute;

  SeqTrack::load_mod_data(
      seq_track, seq_data.mod(), false,
      storage_version_at_least(SEQ_TRACK_MOD_STORAGE_VERSION));
}

bool MidiTrack::store_in_grid(GridSlot column, GridRow row, SeqTrack *seq_track,
                              uint8_t merge, bool online, Grid *grid) {
  (void)merge;
  (void)online;

  active = MIDI_TRACK_TYPE;

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

  return write_grid(_this(), _sizeof(), column, row, grid);
}

#endif // !defined(__AVR__)
