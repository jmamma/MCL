#include "MCL.h"
#include "GridTrack.h"

uint16_t GridTrack::get_track_size() {
  uint16_t size = 0;
  switch (active) {
  case EMPTY_TRACK_TYPE:
    size = sizeof(GridTrack);
    break;
  case MD_TRACK_TYPE:
    size = sizeof(MDTrack);
    break;
  case A4_TRACK_TYPE:
    size = sizeof(A4Track);
    break;
  case EXT_TRACK_TYPE:
    size = sizeof(ExtTrack);
    break;
  case A4_TRACK_TYPE_270:
    size = sizeof(A4Track_270);
    break;
  case MD_TRACK_TYPE_270:
    size = sizeof(MDTrack_270);
    break;
  case EXT_TRACK_TYPE_270:
    size = sizeof(ExtTrack_270);
    break;
  }
  return size;
}

bool GridTrack::load_from_grid(uint8_t column, uint16_t row, bool full_load) {

  bool ret;

  ret = proj.read_grid(&(this->active), sizeof(GridTrack), column, row);

  if (!ret) {
    DEBUG_PRINTLN("read failed");
    return false;
  }

  if (full_load) {
    ret = proj.read_grid(&(this->active), get_track_size(), column, row);
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
