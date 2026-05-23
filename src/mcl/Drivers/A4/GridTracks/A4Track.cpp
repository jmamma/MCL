#include "A4Track.h"
#include "MCLSeq.h"

#define A4_SOUND_LENGTH 0x19F

uint16_t A4Track::calc_latency(uint8_t tracknumber) {
  uint16_t a4_latency = A4_SOUND_LENGTH;
  return a4_latency;
}

void A4Track::transition_send(uint8_t tracknumber, GridSlot slotnumber) {
    DEBUG_PRINTLN(F("here"));
    DEBUG_PRINTLN(F("send a4 sound"));
    sound.origPosition = tracknumber;
    sound.soundpool = true;
    sound.toSysex();
}

void A4Track::transition_load(uint8_t tracknumber, SeqTrack* seq_track, GridSlot slotnumber) {
  GridTrack::transition_load(tracknumber, seq_track, slotnumber);
  ExtSeqTrack *ext_track = (ExtSeqTrack *) seq_track;
  ext_track->is_generic_midi = false;
  load_seq_data(seq_track);
}

bool A4Track::get_track_from_sysex(uint8_t tracknumber) {
  DEBUG_DUMP("get blocking");
  auto ret = Analog4.getBlockingSoundX(tracknumber);
  DEBUG_DUMP("finished");
  if (ret) {
    sound.fromSysex(Analog4.midi);
  }
  return ret;
}

void A4Track::load_immediate(uint8_t tracknumber, SeqTrack *seq_track) {
  load_seq_data(seq_track);
}

void A4Track::load_seq_data(SeqTrack *seq_track) {
  ExtTrack::load_ext_seq_data(*this, link, seq_data, mod_data, seq_track);
}

bool A4Track::store_in_grid(GridSlot column, GridRow row, SeqTrack *seq_track, uint8_t merge,
                            bool online, Grid *grid) {

  active = A4_TRACK_TYPE;

  bool ret;
  DEBUG_PRINT_FN();
  DEBUG_PRINTLN(F("storing a4 track"));

  ExtSeqTrack *ext_track = (ExtSeqTrack *) seq_track;
  uint8_t tracknumber = column & 0xF;
  SeqTrack::store_mod_data(mod_data, false, tracknumber);

  // [>analog 4 tracks<]
#ifdef EXT_TRACKS
  if (column != 255 && online && get_track_from_sysex(tracknumber)) {
    link.length = seq_track->length;
    link.set_speed(seq_track->speed);
    memcpy(&seq_data, ext_track->data(), sizeof(seq_data));
  }
#endif
  ret = write_grid(_this(), write_size(), column, row, grid);

  if (!ret) {
    return false;
  }
  return true;
}

#if !defined(__AVR__)
bool A4Track::can_materialize_as(uint8_t track_type) {
  if (track_type == A4_MIDI_TRACK_TYPE) {
    return true;
  }
  if (ExtTrack::can_materialize_legacy_ext(active, track_type)) {
    return true;
  }
  return DeviceTrack::can_materialize_as(track_type);
}
#endif

bool A4Track::materialized_storage_range(uint8_t track_type,
                                         uint16_t &source_offset,
                                         uint16_t &target_offset,
                                         uint16_t &len) {
  if (track_type != EXT_TRACK_TYPE) {
    return false;
  }
  source_offset =
      reinterpret_cast<uintptr_t>(&mod_data) -
      reinterpret_cast<uintptr_t>(_this());
  target_offset = ExtTrack::seq_payload_storage_offset();
  len = ExtTrack::seq_payload_storage_size();
  return true;
}

DeviceTrack *A4Track::materialize_as(uint8_t track_type,
                                     uint8_t tracknumber,
                                     SeqTrack *seq_track) {
  (void)seq_track;
#if !defined(__AVR__)
  if (track_type == A4_MIDI_TRACK_TYPE) {
    GridLink old_link = link;
    ExtSeqTrackData old_seq_data;
    SeqTrackModData old_mod_data = mod_data;
    A4Sound old_sound = sound;
    memcpy(&old_seq_data, &seq_data, sizeof(old_seq_data));

    auto *midi_track =
        static_cast<A4MidiTrack *>(
            init_materialized_track_type(A4_MIDI_TRACK_TYPE));
    midi_track->import_legacy(old_link, old_seq_data, old_mod_data, old_sound,
                              tracknumber);
    return midi_track;
  }
  if (ExtTrack::can_materialize_legacy_ext(active, track_type)) {
    return ExtTrack::materialize_legacy_ext(*this, link, seq_data, mod_data,
                                            track_type, tracknumber);
  }
#endif
  return DeviceTrack::materialize_as(track_type, tracknumber, seq_track);
}

#if !defined(__AVR__)
void A4MidiTrack::import_legacy(const GridLink &old_link,
                                const ExtSeqTrackData &old_seq_data,
                                const SeqTrackModData &old_mod_data,
                                const A4Sound &old_sound,
                                uint8_t tracknumber) {
  sound = old_sound;
  import_legacy_ext_storage(old_link, old_seq_data, old_mod_data, tracknumber);
}

uint16_t A4MidiTrack::calc_latency(uint8_t tracknumber) {
  return A4_SOUND_LENGTH;
}

void A4MidiTrack::transition_send(uint8_t tracknumber, GridSlot slotnumber) {
  DEBUG_PRINTLN(F("here"));
  DEBUG_PRINTLN(F("send a4 sound"));
  sound.origPosition = tracknumber;
  sound.soundpool = true;
  sound.toSysex();
}

void A4MidiTrack::transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                                  GridSlot slotnumber) {
  MidiBackedDeviceTrack::transition_load(tracknumber, seq_track, slotnumber);
}

bool A4MidiTrack::get_track_from_sysex(uint8_t tracknumber) {
  DEBUG_DUMP("get blocking");
  auto ret = Analog4.getBlockingSoundX(tracknumber);
  DEBUG_DUMP("finished");
  if (ret) {
    sound.fromSysex(Analog4.midi);
  }
  return ret;
}

void A4MidiTrack::load_immediate(uint8_t tracknumber, SeqTrack *seq_track) {
  load_seq_data(seq_track);
}

bool A4MidiTrack::store_in_grid(GridSlot column, GridRow row,
                                SeqTrack *seq_track, uint8_t merge,
                                bool online, Grid *grid) {
  (void)merge;
  active = A4_MIDI_TRACK_TYPE;

  DEBUG_PRINT_FN();
  DEBUG_PRINTLN(F("storing a4 midi track"));

  if (column != 255 && online) {
    get_track_from_sysex(column & 0x0F);
  }
  store_midi_seq_data(column, seq_track);

  return write_grid(_this(), get_store_size(), column, row, grid);
}
#endif

// !! Note do not rely on editor code lint errors -- these are for 32bit/64bit x86 sizes!
// Do compile with avr-gcc and observe the error messages

//__SIZE_PROBE<BANK1_MDTEMPO_TRACK_START + 2094 * NUM_EXT_TRACKS> size;
//__SIZE_PROBE<sizeof(PerfTrack)> perftrackdata;
//__SIZE_PROBE<sizeof(MDSeqTrackData)> mdseqtrackdata;
//__SIZE_PROBE<sizeof(MDSeqTrackData)> mdseqtrackdata;
//__SIZE_PROBE<sizeof(a4sound_t)> sza4t;

//__SIZE_PROBE<sizeof(MNMClass)> sz_mnm_class;
//__SIZE_PROBE<sizeof(MDClass)> sz_md_class;
//__SIZE_PROBE<sizeof(A4Class)> sz_a4_class;

//__SIZE_PROBE<sizeof(GridTrack)> szgridtrack;
//__SIZE_PROBE<sizeof(DeviceTrack)> szdevicetrk;
//__SIZE_PROBE<sizeof(A4Track)> sza4trk;
//__SIZE_PROBE<sizeof(EmptyTrack)> szemptytrk;
//__SIZE_PROBE<sizeof(ExtTrack)> szexttrk;
//__SIZE_PROBE<sizeof(MDTrack)> szmdtrk;
//__SIZE_PROBE<sizeof(GridTrack) + sizeof(MDSeqTrackData) + sizeof(MDMachine)> szmdtrk_summed;
//__SIZE_PROBE<sizeof(MDLFOTrack)> szmdlfotrk;
//__SIZE_PROBE<sizeof(MDRouteTrack)> szmdroutetrk;
//__SIZE_PROBE<sizeof(MDFXTrack)> szmdfxtrk;
//__SIZE_PROBE<sizeof(MDTempoTrack)> szmdtempotrk;
//__SIZE_PROBE<AUX_TRACK_LEN> szfx;
//__SIZE_PROBE<sizeof(GridTrack) + sizeof(MDFXData)> szfx_2;

//__SIZE_PROBE<BANK1_MD_TRACKS_START> addr_md;
//__SIZE_PROBE<BANK1_AUX_TRACKS_START> addr_aux;
//__SIZE_PROBE<BANK1_A4_TRACKS_START> addr_a4;
//__SIZE_PROBE<BANK3_FILE_ENTRIES_START> addr_file_start;
//__SIZE_PROBE<BANK3_FILE_ENTRIES_END> addr_end;
//__SIZE_PROBE<COMMSG_SLOTS_END> addr_end;
