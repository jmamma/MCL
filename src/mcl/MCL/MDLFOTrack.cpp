#include "MDLFOTrack.h"
#include "MCLSeq.h"

void MDLFOTrack::transition_send(uint8_t tracknumber, uint8_t slotnumber) {}

void MDLFOTrack::transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                                  uint8_t slotnumber) {
  GridTrack::transition_load(tracknumber, seq_track, slotnumber);
  memcpy(mcl_seq.lfo_tracks[0].data(), lfo_data.data(),
         sizeof(LFOSeqTrackData));
  load_link_data(seq_track);
}

uint16_t MDLFOTrack::calc_latency(uint8_t tracknumber) { return 0; }

void MDLFOTrack::load_immediate(uint8_t tracknumber, SeqTrack *seq_track) {
  memcpy(mcl_seq.lfo_tracks[0].data(), lfo_data.data(),
         sizeof(LFOSeqTrackData));
  DEBUG_PRINTLN("MD LFO LOAD IMMI");
  load_link_data(seq_track);
  DEBUG_PRINTLN("Load speed");
  DEBUG_PRINTLN(lfo_data.speed);
}

void MDLFOTrack::get_lfos() {
  memcpy(lfo_data.data(), mcl_seq.lfo_tracks[0].data(),
         sizeof(LFOSeqTrackData));
  DEBUG_PRINTLN("Get speed");
  DEBUG_PRINTLN(lfo_data.speed);
}

bool MDLFOTrack::store_in_grid(uint8_t column, uint16_t row,
                               SeqTrack *seq_track, uint8_t merge,
                               bool online, Grid *grid) {
  active = MDLFO_TRACK_TYPE;
  bool ret;
  int b = 0;
  DEBUG_PRINT_FN();
  uint32_t len;
  DEBUG_PRINTLN("MDLFO");
  if (column != 255 && online == true) {
    DEBUG_PRINTLN("storing online");
    get_lfos();
  }

  len = sizeof(MDLFOTrack);
  DEBUG_PRINTLN(len);
  ret = write_grid((uint8_t *)(this), len, column, row, grid);

  if (!ret) {
    DEBUG_PRINTLN(F("write failed"));
    return false;
  }
  return true;
}
