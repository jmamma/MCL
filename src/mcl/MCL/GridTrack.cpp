#include "GridTrack.h"
#include "Project.h"
#include "MCLActions.h"
#include "platform.h"

bool GridTrack::write_grid(void *data, size_t len, uint8_t column, uint16_t row, Grid *grid) {
  void *payload = data == nullptr ? _this() : data;
  if (grid == nullptr) {
    return proj.write_grid(payload, len, column, row);
  }
  else {
    return grid->write(payload, len, column, row);
  }
}

void GridTrack::load_link_data(SeqTrack *seq_track) {
  seq_track->speed = link.speed;
  seq_track->length = link.length;
}

void GridTrack::transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                                uint8_t slotnumber) {
  uint8_t n = slotnumber;
  if (seq_track == nullptr) { return; }
  const uint32_t now = MidiClock.div192th_counter;
  const uint32_t target = (uint32_t)mcl_actions.next_transition * 12u +
                          (uint32_t)mcl_actions.transition_offsets[n] - 1u;
  uint32_t count_down = MidiClock.clock_diff_div192(now, target);
  if (target < now && (now - target) < 4096u) {
    count_down = 1;
  }
  USE_LOCK();
  SET_LOCK();
  seq_track->count_down = count_down;
  CLEAR_LOCK();

}
bool GridTrack::load_from_grid_512(uint8_t column, uint16_t row, Grid *grid) {

  if (grid) {
    if (!grid->read(_this(), 512, column, row)) {
      DEBUG_PRINTLN(F("read failed"));
      return false;
    }
  }
  else {
    if (!proj.read_grid(_this(), 512, column, row)) {
      DEBUG_PRINTLN(F("read failed"));
      return false;
    }
  }
  auto tmp = this->active;
  ::new (this) GridTrack;
  this->active = tmp;

  if ((active == 255)) {
    init();
  }

  return true;
}

bool GridTrack::load_from_grid(uint8_t column, uint16_t row) {

  if (!proj.read_grid(_this(), _sizeof(), column, row)) {
    DEBUG_PRINTLN(F("read failed"));
    return false;
  }

  auto tmp = this->active;
  ::new (this) GridTrack;
  this->active = tmp;

  if ((active == 255)) {
    init();
  }

  return true;
}

// merge and online are ignored here.
bool GridTrack::store_in_grid(uint8_t column, uint16_t row, SeqTrack *seq_track, uint8_t merge,
                              bool online, Grid *grid) {

  DEBUG_PRINT_FN();

  if (!write_grid(_this(), _sizeof(), column, row, grid)) {
    DEBUG_PRINTLN(F("write failed"));
    return false;
  }

  return true;
}
