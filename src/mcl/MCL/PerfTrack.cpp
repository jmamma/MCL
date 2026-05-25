#include "PerfTrack.h"
#include "PerfSeqTrack.h"
#include "CommonPages.h"
#include "MCLActions.h"
#include "MDTrack.h"

void PerfTrack::transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                                  GridSlot slotnumber) {
  DEBUG_PRINTLN("transition send");
  if (seq_track) {
    seq_track->cache_loaded = false;
  }
  GridTrack::transition_load(tracknumber, seq_track, slotnumber);
  if (mcl_actions.send_machine[slotnumber]) {
    load_perf(false, seq_track);
  }
}

uint16_t PerfTrack::calc_latency(uint8_t tracknumber) {
  return load_mute_set == 255 ? 0 : 32 * 3 * 4; // Worst case estimate, 32 parameters, 3 bytes each, 4 perf controllers.
}

void PerfTrack::get_perf() {
  memcpy(scenes, PerfData::scenes, sizeof(PerfScene) * NUM_SCENES);
  memcpy(mute_sets,mixer_page.mute_sets, sizeof(mute_sets) + sizeof(perf_locks));
  version = PERF_TRACK_STORAGE_VERSION_CLEAN_LAYOUT;
  load_mute_set = mixer_page.load_mute_set;
  load_type_mask = 0;
  uint8_t bit = 1;
  uint8_t load_bit = 0x10;
  for (uint8_t n = 0; n < 4; n++, bit <<= 1, load_bit <<= 1) {
    PerfEncoder *e = perf_page.perf_encoders[n];
    PerfData *d = &e->perf_data;
    encs[n].src = d->src;
    encs[n].param = d->param;
    encs[n].min = d->min;
    encs[n].active_scene_a = e->active_scene_a;
    encs[n].active_scene_b = e->active_scene_b;
    encs[n].cur = e->cur;
    memcpy(encs[n].name,e->name, PERF_NAME_LENGTH);

    if (mixer_page.load_types[n][0]) {
      load_type_mask |= bit;
    }
    if (mixer_page.load_types[n][1]) {
      load_type_mask |= load_bit;
    }

  }
  DEBUG_PRINTLN("get perf");
  DEBUG_PRINTLN(sizeof(scenes));
}


void PerfTrack::load_perf(bool immediate, SeqTrack *seq_track) {
  DEBUG_PRINTLN("load perf");
  DEBUG_PRINTLN( sizeof(scenes));
  mixer_page.load_mute_set = load_mute_set < 4 ? load_mute_set : 255;

  uint8_t bit = 1;
  uint8_t load_bit = 0x10;
  for (uint8_t n = 0; n < 4; n++, bit <<= 1, load_bit <<= 1) {
    PerfEncoder *e = perf_page.perf_encoders[n];
    PerfData *d = &e->perf_data;
    d->src = encs[n].src;
    d->param = encs[n].param;
    d->min = encs[n].min;
    e->active_scene_a = encs[n].active_scene_a;
    e->active_scene_b = encs[n].active_scene_b;
    e->cur = encs[n].cur;
    e->old = encs[n].cur;
    memcpy(e->name,encs[n].name, PERF_NAME_LENGTH);

    mixer_page.load_types[n][0] = (load_type_mask & bit) != 0;
    mixer_page.load_types[n][1] = (load_type_mask & load_bit) != 0;

  }
 memcpy(PerfData::scenes, scenes, sizeof(PerfScene) * NUM_SCENES);

 memcpy(mixer_page.mute_sets, mute_sets, sizeof(mute_sets) + sizeof(perf_locks));
 uint8_t mute_set = mixer_page.load_mute_set;
 if (mute_set < 4) {
   mixer_page.switch_mute_set(mute_set, immediate, mixer_page.load_types[mute_set]); //Mute change is applied outside of sequencer runtime.
   if (!immediate) {
     PerfSeqTrack *p = (PerfSeqTrack*) seq_track;
     memcpy(p->perf_locks, &perf_locks[mute_set],4); //Perf change is pre-empted at sequencer runtime.
   }
 }
}

void PerfTrack::load_immediate(uint8_t tracknumber, SeqTrack *seq_track) {
  DEBUG_PRINTLN("load immediate");
  load_link_data(seq_track);
  load_perf(true, seq_track);
}

void PerfTrack::get_online_data(uint8_t merge) {
  get_perf();
  update_link_from_pattern(merge);
}
