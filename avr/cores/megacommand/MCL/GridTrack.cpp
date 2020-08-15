#include "MCL.h"
#include "GridTrack.h"

uint16_t GridTrack::get_track_size() { return sizeof(GridTrack); }

bool GridTrack::load_from_grid(uint8_t column, uint16_t row) {

  bool ret;

  ret = proj.read_grid(&(this->active), sizeof(GridTrack), column, row);
  if (!ret) {
    DEBUG_PRINTLN("read failed");
    return false;
  }

  uint32_t len = get_track_size();

  if (len > sizeof(GridTrack)) {
    ret = proj.read_grid(&(this->active), len, column, row);
    if (!ret) {
      DEBUG_PRINTLN("read failed");
      return false;
    }
  }
  if ((active == EMPTY_TRACK_TYPE) || (active == 255)) {
    init();
  }

  return true;
}

bool GridTrack::store_in_grid(uint8_t column, uint16_t row) {

  DEBUG_PRINT_FN();
  bool ret;

  uint32_t len = get_track_size();
  ret = proj.write_grid(&(this->active), len, column, row);
  if (!ret) {
    DEBUG_PRINTLN("write failed");
    return false;
  }

  return true;
}
