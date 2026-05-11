#include "MDLFOTrack.h"
#include "MCLSeq.h"

void MDLFOTrack::transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                                  uint8_t slotnumber) {
  GridTrack::transition_load(tracknumber, seq_track, slotnumber);
  SeqLFOData data;
  lfo_data.store_data(&data);
  mcl_seq.grid_x_lfo_tracks[0].load_data(data);
  load_link_data(seq_track);
}

uint16_t MDLFOTrack::calc_latency(uint8_t tracknumber) { return 0; }

void MDLFOTrack::load_immediate(uint8_t tracknumber, SeqTrack *seq_track) {
  SeqLFOData data;
  lfo_data.store_data(&data);
  mcl_seq.grid_x_lfo_tracks[0].load_data(data);
  DEBUG_PRINTLN("MD LFO LOAD IMMI");
  load_link_data(seq_track);
  DEBUG_PRINTLN("Load speed");
  DEBUG_PRINTLN(lfo_data.speed);
}

void MDLFOTrack::get_lfos() {
  SeqLFOData data;
  mcl_seq.grid_x_lfo_tracks[0].store_data(&data);
  lfo_data.load_data(data);
  DEBUG_PRINTLN("Get speed");
  DEBUG_PRINTLN(lfo_data.speed);
}

void MDLFOTrack::get_online_data(uint8_t merge) {
  get_lfos();
}
