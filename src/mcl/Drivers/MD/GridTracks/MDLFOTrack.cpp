#include "MDLFOTrack.h"
#include "MCLSeq.h"
#include "Project.h"

namespace {

bool legacy_lfo_slot_enabled() {
  return proj.version < PROJ_VERSION_TRACK_STORAGE_VERSION;
}

void load_legacy_lfo_data(LegacyLFOSeqTrackData &lfo_data) NOINLINE();
void load_legacy_lfo_data(LegacyLFOSeqTrackData &lfo_data) {
  if (!legacy_lfo_slot_enabled()) {
    return;
  }
  SeqLFOData legacy_data;
  SeqLFOData data;
  lfo_data.store_data(&legacy_data);
  LFOSeqTrack::convert_legacy_data(legacy_data, &data);
  mcl_seq.grid_x_lfo_tracks[0].load_data(data);
}

} // namespace

void MDLFOTrack::transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                                  GridSlot slotnumber) {
  GridTrack::transition_load(tracknumber, seq_track, slotnumber);
  load_legacy_lfo_data(lfo_data);
  load_link_data(seq_track);
}

uint16_t MDLFOTrack::calc_latency(uint8_t tracknumber) { return 0; }

void MDLFOTrack::load_immediate(uint8_t tracknumber, SeqTrack *seq_track) {
  load_legacy_lfo_data(lfo_data);
  DEBUG_PRINTLN("MD LFO LOAD IMMI");
  load_link_data(seq_track);
  DEBUG_PRINTLN("Load speed");
  DEBUG_PRINTLN(lfo_data.speed);
}

void MDLFOTrack::get_lfos() {
  if (!legacy_lfo_slot_enabled()) {
    lfo_data.init();
    return;
  }
  SeqLFOData data;
  mcl_seq.grid_x_lfo_tracks[0].store_legacy_data(&data);
  lfo_data.load_data(data);
  DEBUG_PRINTLN("Get speed");
  DEBUG_PRINTLN(lfo_data.speed);
}

void MDLFOTrack::get_online_data(uint8_t merge) {
  get_lfos();
}
