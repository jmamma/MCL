#include "GridTrack.h"
#include "MCL.h"
#include "new.h"

bool GridTrack::load_from_grid(uint8_t column, uint16_t row, bool data) {

  bool ret;

  ret = proj.read_grid(this, sizeof(GridTrack), column, row);
  if (!ret) {
    DEBUG_PRINTLN("read failed");
    return false;
  }

  switch (active) {
  case A4_TRACK_TYPE_270:
  case MD_TRACK_TYPE_270:
  case EXT_TRACK_TYPE_270:
    if (!data) {
      // no space for track upgrade
      return false;
    } else {
      // TODO upgrade right here
      return true;
    }
    break;
  case EMPTY_TRACK_TYPE:
    ::new(this) EmptyTrack;
    break;
  case MD_TRACK_TYPE:
    ::new(this) MDTrack;
    break;
  case A4_TRACK_TYPE:
    ::new(this) A4Track;
    break;
  case EXT_TRACK_TYPE:
    ::new(this) ExtTrack;
    break;
  default:
    // unrecognized track type
    return false;
  }

  if (!data) return true;

  uint32_t len = get_track_size();

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

bool GridTrack::store_in_grid(uint8_t column, uint16_t row, bool data) {

  DEBUG_PRINT_FN();
  bool ret;

  uint32_t len = data ? get_track_size() : sizeof(GridTrack);
  ret = proj.write_grid(this, len, column, row);
  if (!ret) {
    DEBUG_PRINTLN("write failed");
    return false;
  }

  return true;
}
