#include "MCL_impl.h"
void MDLFOTrack::transition_send(uint8_t tracknumber, uint8_t slotnumber) {
  memcpy(&mcl_seq.lfo_tracks[0], &lfo_data, sizeof(LFOSeqTrackData));
}

uint16_t MDLFOTrack::calc_latency(uint8_t tracknumber) { return 0; }

void MDLFOTrack::load_immediate(uint8_t tracknumber, SeqTrack *seq_track) {
  load_link_data(seq_track);
  memcpy(&mcl_seq.lfo_tracks[0], &lfo_data, sizeof(LFOSeqTrackData));
}

void MDLFOTrack::get_lfos() {
  memcpy(&lfo_data, &mcl_seq.lfo_tracks[0], sizeof(LFOSeqTrackData));
}

bool MDLFOTrack::store_in_grid(uint8_t column, uint16_t row,
                               SeqTrack *seq_track, uint8_t merge,
                               bool online) {
  active = MDLFO_TRACK_TYPE;
  bool ret;
  int b = 0;
  DEBUG_PRINT_FN();
  uint32_t len;

  if (column != 255 && online == true) {
    get_lfos();
  }

  len = sizeof(MDLFOTrack);
  DEBUG_PRINTLN(len);

  ret = proj.write_grid((uint8_t *)(this), len, column, row);

  if (!ret) {
    DEBUG_PRINTLN(F("write failed"));
    return false;
  }
  return true;
}
