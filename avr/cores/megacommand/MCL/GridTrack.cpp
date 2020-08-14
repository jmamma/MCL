#include "MCL.h"
#include "GridTrack.h"

uint16_t GridTrack::get_track_size() { return sizeof(GridTrack); }

bool GridTrack::load_from_grid(uint8_t column, uint16_t row) {

  bool ret;

  uint32_t len = (sizeof(GridTrack));

  DEBUG_PRINTLN("reading grid track");

  ret = proj.read_grid((uint8_t *)this, len, column, row);
  if (!ret) {
    DEBUG_PRINTLN("read failed");
    return false;
  }

  len = get_track_size() - len;
  if (len > 0) {
    ret = proj.read_grid((uint8_t *)(this), len);
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
  ret = proj.write_grid((uint8_t *)(this), len, column, row);
  if (!ret) {
    DEBUG_PRINTLN("write failed");
    return false;
  }

  return true;
}
