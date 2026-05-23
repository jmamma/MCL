#include "ExtTrack.h"
#include "global.h"
#include "MCLSeq.h"
#if !defined(__AVR__)
#include "MidiTrack.h"
#include "MidiTrackMaterializer.h"
#endif
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

#if !defined(__AVR__)
bool ExtTrack::can_materialize_as(uint8_t track_type) {
  if (can_materialize_legacy_ext(active, track_type)) {
    return true;
  }
  return DeviceTrack::can_materialize_as(track_type);
}

bool ExtTrack::can_materialize_legacy_ext(uint8_t active,
                                          uint8_t track_type) {
  if (track_type == EXT_TRACK_TYPE && active != EXT_TRACK_TYPE &&
      is_legacy_ext_sequence_type(active)) {
    return true;
  }
  return midi_track_type_is_storage_family(track_type) &&
         is_legacy_ext_sequence_type(active);
}
#endif

DeviceTrack *ExtTrack::materialize_as(uint8_t track_type,
                                      uint8_t tracknumber,
                                      SeqTrack *seq_track) {
  (void)seq_track;
#if !defined(__AVR__)
  if (can_materialize_legacy_ext(active, track_type)) {
    return materialize_legacy_ext(*this, link, seq_data, mod_data, track_type,
                                  tracknumber);
  }
#endif
  return DeviceTrack::materialize_as(track_type, tracknumber, seq_track);
}

void ExtTrack::load_ext_seq_data(DeviceTrack &track, GridLink &link,
                                 ExtSeqTrackData &seq_data,
                                 SeqTrackModData &mod_data,
                                 SeqTrack *seq_track) {
#ifdef EXT_TRACKS
  ExtSeqTrack *ext_track = (ExtSeqTrack *) seq_track;

  uint8_t old_mute = seq_track->mute_state;

  seq_track->mute_state = SEQ_MUTE_ON;
  ext_track->notesoff_pending = true;

  uint8_t *dest = ext_track->data();
  memcpy(dest, &seq_data, sizeof(seq_data));
  track.load_link_data(seq_track);
  ext_track->clear_mutes();
  ext_track->pgm_oneshot = 0;
  ext_track->set_length(seq_track->length);
  seq_track->mute_state = old_mute;

  SeqTrack::load_mod_data(
      seq_track, mod_data, false,
      track.storage_version_at_least(SEQ_TRACK_MOD_STORAGE_VERSION));
#endif
}

void ExtTrack::load_seq_data(SeqTrack *seq_track) {
  load_ext_seq_data(*this, link, seq_data, mod_data, seq_track);
}

#if !defined(__AVR__)
DeviceTrack *ExtTrack::materialize_legacy_ext(DeviceTrack &track,
                                              GridLink &link,
                                              ExtSeqTrackData &seq_data,
                                              SeqTrackModData &mod_data,
                                              uint8_t track_type,
                                              uint8_t tracknumber) {
  if (track_type == EXT_TRACK_TYPE && track.active != EXT_TRACK_TYPE &&
      is_legacy_ext_sequence_type(track.active)) {
    GridLink old_link = link;
    ExtSeqTrackData old_seq_data;
    SeqTrackModData old_mod_data = mod_data;
    memcpy(&old_seq_data, &seq_data, sizeof(old_seq_data));

    auto *ext_track = static_cast<ExtTrack *>(
        track.init_materialized_track_type(EXT_TRACK_TYPE));
    ext_track->link = old_link;
    ext_track->mod_data = old_mod_data;
    memcpy(&ext_track->seq_data, &old_seq_data, sizeof(old_seq_data));
    return ext_track;
  }

  if (midi_track_type_is_storage_family(track_type) &&
      is_legacy_ext_sequence_type(track.active)) {
    GridLink old_link = link;
    ExtSeqTrackData old_seq_data;
    SeqTrackModData old_mod_data = mod_data;
    memcpy(&old_seq_data, &seq_data, sizeof(old_seq_data));

    MidiSeqTrackStorage midi_seq_data;
    midi_seq_data.clear_storage();
    midi_seq_data.mod() = old_mod_data;
    midi_seq_data.import_legacy_ext(old_seq_data, old_link);
    midi_seq_data.channel =
        old_seq_data.channel < 16 ? old_seq_data.channel : tracknumber;
    return materialize_midi_storage_track(&track, track_type, old_link,
                                          midi_seq_data, tracknumber);
  }
  return nullptr;
}
#endif

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
