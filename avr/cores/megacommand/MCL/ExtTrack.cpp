#include "MCL_impl.h"

void ExtTrack::transition_load(uint8_t tracknumber, SeqTrack* seq_track, uint8_t slotnumber) {
  ExtSeqTrack *ext_track = (ExtSeqTrack *) seq_track;
  ext_track->buffer_notesoff();
  GridTrack::transition_load(tracknumber, seq_track, slotnumber);
  load_seq_data(seq_track);
}

void ExtTrack::load_immediate(uint8_t tracknumber, SeqTrack *seq_track) {
  store_in_mem(tracknumber);
  load_seq_data(seq_track);
}

bool ExtTrack::get_track_from_sysex(uint8_t tracknumber) {
  active = EXT_TRACK_TYPE;
  return true;
}

bool ExtTrack::load_seq_data(SeqTrack *seq_track) {
#ifdef EXT_TRACKS
  if (chain.speed == 0) {
    chain.speed = SEQ_SPEED_2X;
  }
  ExtSeqTrack *ext_track = (ExtSeqTrack *) seq_track;
  ext_track->buffer_notesoff();
  memcpy(ext_track, &seq_data, sizeof(seq_data));
  ext_track->speed = chain.speed;
  ext_track->length = chain.length;
#endif
  return true;
}

bool ExtTrack::store_in_grid(uint8_t tracknumber, uint16_t row, uint8_t merge,
                             bool online) {
  /*Assign a track to Grid i*/
  /*Extraact track data from received pattern and kit and store in track
   * object*/
  bool ret;

  int b = 0;
  DEBUG_PRINT_FN();
  uint32_t len;

#ifdef EXT_TRACKS
  if (online) {
    get_track_from_sysex(tracknumber);
    chain.length = mcl_seq.ext_tracks[tracknumber].length;
    chain.speed = mcl_seq.ext_tracks[tracknumber].speed;
    memcpy(&seq_data, &mcl_seq.ext_tracks[tracknumber], sizeof(seq_data));
  }
#endif

  ret = proj.write_grid((uint8_t *)this, sizeof(ExtTrack), tracknumber, row);
  if (!ret) {
    DEBUG_PRINTLN("Write failed");
    return false;
  }

  return true;
}
