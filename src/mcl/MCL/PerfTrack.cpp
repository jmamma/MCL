#include "MCL_impl.h"

void PerfTrack::transition_send(uint8_t tracknumber, uint8_t slotnumber) {
  DEBUG_PRINTLN("transition send");
}

void PerfTrack::transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                                  uint8_t slotnumber) {
  DEBUG_PRINTLN("transition send");
  GridTrack::transition_load(tracknumber, seq_track, slotnumber);
  if (mcl_actions.send_machine[slotnumber]) {
    load_perf(false, seq_track);
  }
}

uint16_t PerfTrack::calc_latency(uint8_t tracknumber) {
  uint8_t load_mute_set = 255;

  for (uint8_t n = 0; n < 4; n++) {
    if ((mute_sets[1].mutes[n] & 0b1000000000000000) == 0) {
      load_mute_set = n;
    }
  }

  return load_mute_set == 255 ? 0 : 32 * 3 * 4; // Worst case estimate, 32 parameters, 3 bytes each, 4 perf controllers.
}

void PerfTrack::get_perf() {
  memcpy(scenes, PerfData::scenes, sizeof(PerfScene) * NUM_SCENES);
  memcpy(mute_sets,mixer_page.mute_sets, sizeof(mute_sets) + sizeof(perf_locks));
  for (uint8_t n = 0; n < 4; n++) {
    PerfEncoder *e = perf_page.perf_encoders[n];
    PerfData *d = &e->perf_data;
    encs[n].src = d->src;
    encs[n].param = d->param;
    encs[n].min = d->min;
    encs[n].active_scene_a = e->active_scene_a;
    encs[n].active_scene_b = e->active_scene_b;
    encs[n].cur = e->cur;
    memcpy(encs[n].name,e->name, PERF_NAME_LENGTH);

    if (!mixer_page.load_types[n][0]) {
      CLEAR_BIT16(mute_sets[1].mutes[n],13);
    }
    if (!mixer_page.load_types[n][1]) {
      CLEAR_BIT16(mute_sets[1].mutes[n],14);
    }

  }
  DEBUG_PRINTLN("get perf");
  DEBUG_PRINTLN(sizeof(scenes));
 //Encode the load_mute_set value into the high bit of the corresponding ext mutes.
  if (mixer_page.load_mute_set < 4) {
    mute_sets[1].mutes[mixer_page.load_mute_set] &= 0b0111111111111111;
  }
}


void PerfTrack::load_perf(bool immediate, SeqTrack *seq_track) {
  DEBUG_PRINTLN("load perf");
  DEBUG_PRINTLN( sizeof(scenes));
  mixer_page.load_mute_set = 255;

  for (uint8_t n = 0; n < 4; n++) {
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

    if ((mute_sets[1].mutes[n] & 0b1000000000000000) == 0) {
      mixer_page.load_mute_set = n;
    }
    mixer_page.load_types[n][0] = mute_sets[1].mutes[n] & 0b0010000000000000;
    mixer_page.load_types[n][1] = mute_sets[1].mutes[n] & 0b0100000000000000;
    mute_sets[1].mutes[n] |= 0b1110000000000000;

  }
 memcpy(PerfData::scenes, scenes, sizeof(PerfScene) * NUM_SCENES);

 memcpy(mixer_page.mute_sets, mute_sets, sizeof(mute_sets) + sizeof(perf_locks));
 if (mixer_page.load_mute_set < 4) {
   mixer_page.switch_mute_set(mixer_page.load_mute_set, immediate, mixer_page.load_types[mixer_page.load_mute_set]); //Mute change is applied outside of sequencer runtime.
   if (!immediate) {
     PerfSeqTrack *p = (PerfSeqTrack*) seq_track;
     memcpy(p->perf_locks, &perf_locks[mixer_page.load_mute_set],4); //Perf change is pre-empted at sequencer runtime.
   }
 }
}

void PerfTrack::load_immediate(uint8_t tracknumber, SeqTrack *seq_track) {
  DEBUG_PRINTLN("load immediate");
  load_link_data(seq_track);
  load_perf(true, seq_track);
}

bool PerfTrack::store_in_grid(uint8_t column, uint16_t row,
                                 SeqTrack *seq_track, uint8_t merge,
                                 bool online, Grid *grid) {
  active = PERF_TRACK_TYPE;
  bool ret;
  int b = 0;
  DEBUG_PRINT_FN();
  uint32_t len;

  if (column != 255 && online == true) {
    get_perf();
    if (merge == SAVE_MD) {
      link.length = MD.pattern.patternLength;
      link.speed = SEQ_SPEED_1X + MD.pattern.doubleTempo;
    }
  }

  len = sizeof(PerfTrack);
  DEBUG_PRINTLN(len);

  ret = write_grid((uint8_t *)(this), len, column, row, grid);

  if (!ret) {
    DEBUG_PRINTLN(F("write failed"));
    return false;
  }
  return true;
}
