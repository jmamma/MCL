#include "GridTrack.h"
#include "MCL.h"
#include "new.h"

bool GridTrack::init_track_type(uint8_t track_type) {
   ::new(this) GridTrack;
   return true;
}

bool GridTrack::load_from_grid(uint8_t column, uint16_t row) {

  bool ret;

  ret = proj.read_grid(this, sizeof(GridTrack), column, row);
  if (!ret) {
    DEBUG_PRINTLN("read failed");
    return false;
  }

 if (!init_track_type(active)) return false;

  uint32_t len = get_track_size();

  DEBUG_PRINTLN(len);

  if (len > sizeof(GridTrack)) {
    ret = proj.read_grid(this, len, column, row);
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
  ret = proj.write_grid(this, len, column, row);
  if (!ret) {
    DEBUG_PRINTLN("write failed");
    return false;
  }

  return true;
}
