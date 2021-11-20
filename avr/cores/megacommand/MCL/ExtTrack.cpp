#include "MCL_impl.h"

void ExtTrack::transition_load(uint8_t tracknumber, SeqTrack* seq_track, uint8_t slotnumber) {
  DEBUG_DUMP("transition_load_ext");
  DEBUG_DUMP((uint16_t) seq_track);
  DEBUG_DUMP(slotnumber);
  DEBUG_DUMP(tracknumber);
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
  ExtSeqTrack *ext_track = (ExtSeqTrack *) seq_track;
  ext_track->buffer_notesoff();
  memcpy(ext_track->data(), &seq_data, sizeof(seq_data));
  load_link_data(seq_track);
#endif
  return true;
}

bool ExtTrack::store_in_grid(uint8_t column, uint16_t row, SeqTrack *seq_track, uint8_t merge,
                             bool online) {
  /*Assign a track to Grid i*/
  /*Extraact track data from received pattern and kit and store in track
   * object*/
  bool ret;

  int b = 0;
  DEBUG_PRINT_FN();
  uint32_t len;

  ExtSeqTrack *ext_track = (ExtSeqTrack *) seq_track;

#ifdef EXT_TRACKS
  if (online) {
    get_track_from_sysex(column);
    link.length = seq_track->length;
    link.speed = seq_track->speed;
    memcpy(&seq_data, ext_track->data(), sizeof(seq_data));
  }
#endif

  ret = proj.write_grid((uint8_t *)this, sizeof(ExtTrack), column, row);
  if (!ret) {
    DEBUG_PRINTLN(F("Write failed"));
    return false;
  }

  return true;
}
