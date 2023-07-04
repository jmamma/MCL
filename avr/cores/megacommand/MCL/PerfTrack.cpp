#include "MCL_impl.h"

void PerfTrack::transition_send(uint8_t tracknumber, uint8_t slotnumber) {
  load_perf();
}

uint16_t PerfTrack::calc_latency(uint8_t tracknumber) {
  return 0;
}
void PerfTrack::get_perf() {

  for (uint8_t n = 0; n < 4; n++) {
    PerfEncoder *e = perf_page.perf_encoders[n];
    PerfData *d = &e->perf_data;
    encs[n].src = d->src;
    encs[n].param = d->param;
    encs[n].min = d->min;
    encs[n].active_scene_a = e->active_scene_a;
    encs[n].active_scene_b = e->active_scene_b;
    encs[n].cur = e->cur;
  }
  DEBUG_PRINTLN("get perf");
  DEBUG_PRINTLN(sizeof(scenes));
  memcpy(&scenes[0], &PerfData::scenes[0], sizeof(scenes));
}


void PerfTrack::load_perf() {

  for (uint8_t n = 0; n < 4; n++) {
    PerfEncoder *e = perf_page.perf_encoders[n];
    PerfData *d = &e->perf_data;
    d->src = encs[n].src;
    d->param = encs[n].param;
    d->min = encs[n].min;
    e->active_scene_a = encs[n].active_scene_a;
    e->active_scene_b = encs[n].active_scene_b;
    e->cur = encs[n].cur;
  }

  DEBUG_PRINTLN("load perf");
  DEBUG_PRINTLN( sizeof(scenes));
  memcpy(&PerfData::scenes[0], &scenes[0], sizeof(scenes));
}

void PerfTrack::load_immediate(uint8_t tracknumber, SeqTrack *seq_track) {
  load_link_data(seq_track);
  load_perf();
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
