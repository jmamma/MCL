#include "ExtTrack.h"
#include "Global.h"

void ExtTrack::transition_load(uint8_t tracknumber, SeqTrack* seq_track, uint8_t slotnumber) {
  DEBUG_DUMP(F("transition_load_ext"));
  DEBUG_DUMP((uint16_t) seq_track);
  DEBUG_DUMP(slotnumber);
  DEBUG_DUMP(tracknumber);
  ExtSeqTrack *ext_track = (ExtSeqTrack *) seq_track;
  ext_track->is_generic_midi = true;
  GridTrack::transition_load(tracknumber, seq_track, slotnumber);
  //load_seq_data(seq_track);
}

void ExtTrack::load_immediate(uint8_t tracknumber, SeqTrack *seq_track) {
  DEBUG_PRINTLN("load immediate, ext");
  load_seq_data(seq_track);
}

bool ExtTrack::get_track_from_sysex(uint8_t tracknumber) {
  active = EXT_TRACK_TYPE;
  return true;
}

void ExtTrack::load_seq_data(SeqTrack *seq_track) {
#ifdef EXT_TRACKS
  ExtSeqTrack *ext_track = (ExtSeqTrack *) seq_track;

  uint8_t old_mute = seq_track->mute_state;

  seq_track->mute_state = SEQ_MUTE_ON;

  seq_tx3.txRb->init();
  seq_tx4.txRb->init();

  ext_track->buffer_notesoff();

  memcpy(ext_track->data(), &seq_data, sizeof(seq_data));
  load_link_data(seq_track);
  ext_track->clear_mutes();
  ext_track->pgm_oneshot = 0;
  ext_track->set_length(seq_track->length);
  seq_track->mute_state = old_mute;
#endif
}

bool ExtTrack::store_in_grid(uint8_t column, uint16_t row, SeqTrack *seq_track, uint8_t merge,
                             bool online, Grid *grid) {
  /*Assign a track to Grid i*/
  /*Extraact track data from received pattern and kit and store in track
   * object*/
  bool ret;

  int b = 0;
  if (grid == nullptr) { DEBUG_PRINTLN("grid is nullptr"); }

  ExtSeqTrack *ext_track = (ExtSeqTrack *) seq_track;
  //ext_track->store_mute_state();
#ifdef EXT_TRACKS
  if (online) {
    get_track_from_sysex(column);
    link.length = seq_track->length;
    link.speed = seq_track->speed;
    memcpy(&seq_data, ext_track->data(), sizeof(seq_data));
  }
#endif
  ret = write_grid((uint8_t *)(this), sizeof(ExtTrack), column, row, grid);
  if (!ret) {
    DEBUG_PRINTLN(F("Write failed"));
    return false;
  }

  return true;
}
