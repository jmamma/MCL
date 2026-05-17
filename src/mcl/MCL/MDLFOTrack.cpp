#include "MDLFOTrack.h"
#include "MCLSeq.h"

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

void MDLFOTrack::get_online_data(uint8_t merge) {
  get_lfos();
}
