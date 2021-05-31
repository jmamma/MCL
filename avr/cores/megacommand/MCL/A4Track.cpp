#include "MCL_impl.h"
//#include "MCLSd.h"

#define A4_SOUND_LENGTH 0x19F

uint16_t A4Track::calc_latency(uint8_t tracknumber) {
  uint16_t a4_latency = A4_SOUND_LENGTH;
  return a4_latency;
}

void A4Track::transition_send(uint8_t tracknumber, uint8_t slotnumber) {
    DEBUG_PRINTLN(F("here"));
    DEBUG_PRINTLN(F("send a4 sound"));
    sound.origPosition = tracknumber;
    sound.soundpool = true;
    sound.toSysex();
}

void A4Track::transition_load(uint8_t tracknumber, SeqTrack* seq_track, uint8_t slotnumber) {
  uint8_t n = slotnumber;
  ExtTrack::transition_load(tracknumber, seq_track, slotnumber);
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
  store_in_mem(tracknumber);
  load_seq_data(seq_track);
}

bool A4Track::store_in_grid(uint8_t column, uint16_t row, SeqTrack *seq_track, uint8_t merge,
                            bool online) {

  active = A4_TRACK_TYPE;

  bool ret;
  int b = 0;
  DEBUG_PRINT_FN();
  DEBUG_PRINTLN(F("storing a4 track"));
  uint32_t len;

  ExtSeqTrack *ext_track = (ExtSeqTrack *) seq_track;

  // [>analog 4 tracks<]
#ifdef EXT_TRACKS
  if (online && get_track_from_sysex(column)) {
    link.length = seq_track->length;
    link.speed = seq_track->speed;
    memcpy(&seq_data, ext_track->data(), sizeof(seq_data));
  }
#endif
  ret = proj.write_grid((uint8_t *)this, sizeof(A4Track), column, row);
  if (!ret) {
    return false;
  }
  return true;
}

// !! Note do not rely on editor code lint errors -- these are for 32bit/64bit x86 sizes!
// Do compile with avr-gcc and observe the error messages

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
//__SIZE_PROBE<BANK1_FILE_ENTRIES_START> addr_file_start;
//__SIZE_PROBE<BANK1_FILE_ENTRIES_END> addr_end;
