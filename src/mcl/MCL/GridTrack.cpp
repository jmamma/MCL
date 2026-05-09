#include "GridTrack.h"
#include "Project.h"
#include "MCLActions.h"
#include "platform.h"
#if !defined(__AVR__)
#include "../Drivers/Generic/Sequencer/StepSeqDefines.h"
#endif

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

#if !defined(__AVR__)
uint8_t GridTrack::transition_countdown_resolution() {
  return STEPSEQ_LEGACY_SEQ_INTERPOLATION;
}
#endif

void GridTrack::transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                                uint8_t slotnumber) {
  uint8_t n = slotnumber;
  if (seq_track == nullptr) { return; }
  const uint32_t now = MidiClock.div192th_counter;
  uint32_t target =
      MidiClock.div16th_to_div192(mcl_actions.next_transition,
                                  mcl_actions.transition_offsets[n]);
  if (target > 0) {
    target--;
  }
  uint32_t count_down = MidiClock.clock_diff_div192(now, target);
  if (target < now && (now - target) < 4096u) {
    count_down = 1;
  }
#if !defined(__AVR__)
  const uint8_t countdown_resolution = transition_countdown_resolution();
  if (countdown_resolution > 0 &&
      MidiClock.clock_interpolation > countdown_resolution) {
    const uint8_t divider = MidiClock.clock_interpolation / countdown_resolution;
    if (divider > 1) {
      count_down = (count_down + divider - 1) / divider;
    }
  }
#endif
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
