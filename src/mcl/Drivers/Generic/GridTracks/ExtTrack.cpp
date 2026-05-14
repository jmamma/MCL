#include "ExtTrack.h"
#include "Global.h"
#include "MCLSeq.h"
#ifdef PLATFORM_TBD
#include "../../TBD/TBDTrack.h"
#endif

namespace {
#if !defined(__AVR__)
bool is_legacy_ext_sequence_type(uint8_t track_type) {
  return track_type == EXT_TRACK_TYPE ||
         track_type == A4_TRACK_TYPE ||
         track_type == MNM_TRACK_TYPE;
}
#endif

} // namespace

void ExtTrack::transition_load(uint8_t tracknumber, SeqTrack* seq_track, GridSlot slotnumber) {
  DEBUG_DUMP(F("transition_load_ext"));
  DEBUG_DUMP((uint16_t) seq_track);
  DEBUG_DUMP(slotnumber);
  DEBUG_DUMP(tracknumber);
  ExtSeqTrack *ext_track = (ExtSeqTrack *) seq_track;
  ext_track->is_generic_midi = true;
  ext_track->cache_loaded = false;
  GridTrack::transition_load(tracknumber, seq_track, slotnumber);
  //load_seq_data(seq_track);
}

void ExtTrack::transition_load_device(uint8_t tracknumber, SeqTrack *seq_track, GridSlot slotnumber) {
  GridTrack::transition_load(tracknumber, seq_track, slotnumber);
  ExtSeqTrack *ext_track = (ExtSeqTrack *) seq_track;
  ext_track->is_generic_midi = false;
  load_seq_data(seq_track);
}

void ExtTrack::load_immediate(uint8_t tracknumber, SeqTrack *seq_track) {
  DEBUG_PRINTLN("load immediate, ext");
  load_seq_data(seq_track);
}

bool ExtTrack::get_track_from_sysex(uint8_t tracknumber) {
  active = EXT_TRACK_TYPE;
  return true;
}

#ifdef PLATFORM_TBD
bool ExtTrack::can_materialize_as(uint8_t track_type) {
  if (track_type == TBD_MIDI_TRACK_TYPE &&
      is_legacy_ext_sequence_type(active)) {
    return true;
  }
  return DeviceTrack::can_materialize_as(track_type);
}
#endif

DeviceTrack *ExtTrack::materialize_as(uint8_t track_type,
                                      uint8_t tracknumber,
                                      SeqTrack *seq_track) {
  (void)seq_track;
#if !defined(__AVR__)
  if (track_type == EXT_TRACK_TYPE && active != EXT_TRACK_TYPE &&
      is_legacy_ext_sequence_type(active)) {
    GridLink old_link = link;
    ExtSeqTrackData old_seq_data;
    SeqTrackModData old_mod_data = mod_data;
    memcpy(&old_seq_data, &seq_data, sizeof(old_seq_data));

    auto *ext_track = static_cast<ExtTrack *>(init_track_type(EXT_TRACK_TYPE));
    ext_track->link = old_link;
    ext_track->mod_data = old_mod_data;
    memcpy(&ext_track->seq_data, &old_seq_data, sizeof(old_seq_data));
    return ext_track;
  }
#endif

#ifdef PLATFORM_TBD
  if (track_type == TBD_MIDI_TRACK_TYPE &&
      is_legacy_ext_sequence_type(active)) {
    GridLink old_link = link;
    ExtSeqTrackData old_seq_data;
    SeqTrackModData old_mod_data = mod_data;
    memcpy(&old_seq_data, &seq_data, sizeof(old_seq_data));

    auto *midi_track =
        static_cast<TBDMidiTrack *>(init_track_type(TBD_MIDI_TRACK_TYPE));
    midi_track->init(tracknumber, nullptr);
    midi_track->link = old_link;
    midi_track->seq_data.mod() = old_mod_data;
    midi_track->seq_data.import_legacy_ext(old_seq_data, old_link);
    midi_track->p4_sound.midi_channel = old_seq_data.channel;
    midi_track->seq_data.channel = old_seq_data.channel;
    return midi_track;
  }
#endif
  return DeviceTrack::materialize_as(track_type, tracknumber, seq_track);
}

void ExtTrack::load_seq_data(SeqTrack *seq_track) {
#ifdef EXT_TRACKS
  ExtSeqTrack *ext_track = (ExtSeqTrack *) seq_track;

  uint8_t old_mute = seq_track->mute_state;

  seq_track->mute_state = SEQ_MUTE_ON;
  ext_track->notesoff_pending = true;

  uint8_t *dest = ext_track->data();
  memcpy(dest, &seq_data, sizeof(seq_data));
  load_link_data(seq_track);
  ext_track->clear_mutes();
  ext_track->pgm_oneshot = 0;
  ext_track->set_length(seq_track->length);
  seq_track->mute_state = old_mute;

  SeqTrack::load_mod_data(
      seq_track, mod_data, false,
      storage_version_at_least(SEQ_TRACK_MOD_STORAGE_VERSION));
#endif
}

bool ExtTrack::store_in_grid(GridSlot column, GridRow row, SeqTrack *seq_track, uint8_t merge,
                             bool online, Grid *grid) {
  /*Assign a track to Grid i*/
  /*Extraact track data from received pattern and kit and store in track
  * object*/
  bool ret;

  if (grid == nullptr) { DEBUG_PRINTLN("grid is nullptr"); }

  ExtSeqTrack *ext_track = (ExtSeqTrack *) seq_track;
  uint8_t tracknumber = column & 0x0F;
  SeqTrack::store_mod_data(mod_data, false, tracknumber);
  //ext_track->store_mute_state();
#ifdef EXT_TRACKS
  if (online) {
    get_track_from_sysex(column);
    link.length = seq_track->length;
    link.set_speed(seq_track->speed);
    uint8_t *src = ext_track->data();
    memcpy(&seq_data, src, sizeof(seq_data));
  }
#endif
  ret = write_grid(_this(), _sizeof(), column, row, grid);
  if (!ret) {
    DEBUG_PRINTLN(F("Write failed"));
    return false;
  }

  return true;
}
