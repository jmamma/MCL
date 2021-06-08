#include "MCL_impl.h"

void GridTrack::load_link_data(SeqTrack *seq_track) {
  seq_track->speed = link.speed;
  seq_track->length = link.length;
}

void GridTrack::transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                                uint8_t slotnumber) {
  uint8_t n = slotnumber;
  if (seq_track == nullptr) { return; }
  USE_LOCK();
  SET_LOCK();
  seq_track->count_down = MidiClock.clock_diff_div192(MidiClock.div192th_counter, (uint32_t) mcl_actions.next_transition * (uint32_t) 12 + (uint32_t) mcl_actions.transition_offsets[n] - 1);
  CLEAR_LOCK();
//  seq_track->mute_until_start = true;
}

bool GridTrack::load_from_grid(uint8_t column, uint16_t row) {

  if (!proj.read_grid(this, sizeof(GridTrack), column, row)) {
    DEBUG_PRINTLN(F("read failed"));
    return false;
  }

  auto tmp = this->active;
  ::new (this) GridTrack;
  this->active = tmp;

  if ((active == EMPTY_TRACK_TYPE) || (active == 255)) {
    init();
  }

  return true;
}

// merge and online are ignored here.
bool GridTrack::store_in_grid(uint8_t column, uint16_t row, SeqTrack *seq_track, uint8_t merge,
                              bool online) {

  DEBUG_PRINT_FN();

  if (!proj.write_grid(this, sizeof(GridTrack), column, row)) {
    DEBUG_PRINTLN(F("write failed"));
    return false;
  }

  return true;
}
