#include "MidiTrackMaterializer.h"

#if !defined(__AVR__)

#include "MidiTrack.h"
#include "../../A4/GridTracks/A4Track.h"
#include "../../MNM/GridTracks/MNMTrack.h"
#ifdef PLATFORM_TBD
#include "../../TBD/TBDTrack.h"
#endif
#include <string.h>

bool midi_track_type_is_storage_family(uint8_t track_type) {
  if (track_type == MIDI_TRACK_TYPE || track_type == A4_MIDI_TRACK_TYPE ||
      track_type == MNM_MIDI_TRACK_TYPE) {
    return true;
  }
#ifdef PLATFORM_TBD
  if (track_type == TBD_MIDI_TRACK_TYPE) {
    return true;
  }
#endif
  return false;
}

DeviceTrack *materialize_midi_storage_track(DeviceTrack *storage,
                                            uint8_t track_type,
                                            const GridLink &link,
                                            const MidiSeqTrackStorage &seq_data,
                                            uint8_t tracknumber) {
  if (storage == nullptr) {
    return nullptr;
  }

  MidiSeqTrackStorage stored_seq = seq_data;
  if (stored_seq.channel >= 16) {
    stored_seq.channel = tracknumber;
  }
  GridLink target_link = link;
  bool target_loads_sound = track_type == MIDI_TRACK_TYPE;

  if (track_type == MIDI_TRACK_TYPE) {
    auto *midi_track =
        static_cast<MidiTrack *>(
            storage->init_materialized_track_type(MIDI_TRACK_TYPE));
    midi_track->link = target_link;
    midi_track->set_load_sound(target_loads_sound);
    midi_track->seq_data = stored_seq;
    return midi_track;
  }

  if (track_type == A4_MIDI_TRACK_TYPE) {
    auto *a4_track =
        static_cast<A4MidiTrack *>(
            storage->init_materialized_track_type(A4_MIDI_TRACK_TYPE));
    memset(&a4_track->sound, 0, sizeof(a4_track->sound));
    a4_track->sound.origPosition = tracknumber;
    a4_track->sound.soundpool = true;
    a4_track->link = target_link;
    a4_track->set_load_sound(target_loads_sound);
    a4_track->seq_data = stored_seq;
    return a4_track;
  }

  if (track_type == MNM_MIDI_TRACK_TYPE) {
    auto *mnm_track = static_cast<MNMMidiTrack *>(
        storage->init_materialized_track_type(MNM_MIDI_TRACK_TYPE));
    mnm_track->machine.init(tracknumber);
    mnm_track->link = target_link;
    mnm_track->set_load_sound(target_loads_sound);
    mnm_track->seq_data = stored_seq;
    return mnm_track;
  }

#ifdef PLATFORM_TBD
  if (track_type == TBD_MIDI_TRACK_TYPE) {
    auto *tbd_midi_track = static_cast<TBDMidiTrack *>(
        storage->init_materialized_track_type(TBD_MIDI_TRACK_TYPE));
    tbd_midi_track->init(tracknumber, nullptr);
    tbd_midi_track->link = target_link;
    tbd_midi_track->set_load_sound(target_loads_sound);
    tbd_midi_track->seq_data = stored_seq;
    tbd_midi_track->p4_sound.midi_channel = stored_seq.channel;
    return tbd_midi_track;
  }
#endif

  return nullptr;
}

#endif // !defined(__AVR__)
