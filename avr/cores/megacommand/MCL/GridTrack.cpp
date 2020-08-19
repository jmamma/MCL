#include "GridTrack.h"
#include "MCL.h"
#include "new.h"

bool GridTrack::load_from_grid(uint8_t column, uint16_t row) {

  if (!proj.read_grid(this, sizeof(GridTrack), column, row)) {
    DEBUG_PRINTLN("read failed");
    return false;
  }

  auto tmp = this->active;
  ::new(this) GridTrack;
  this->active = tmp;

  if ((active == EMPTY_TRACK_TYPE) || (active == 255)) {
    init();
  }

  return true;
}

bool GridTrack::store_in_grid(uint8_t column, uint16_t row) {

  DEBUG_PRINT_FN();

  if (!proj.write_grid(this, sizeof(GridTrack), column, row)) {
    DEBUG_PRINTLN("write failed");
    return false;
  }

  return true;
}
