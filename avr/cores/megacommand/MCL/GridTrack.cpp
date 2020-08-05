#include "MCL.h"
#include "GridTrack.h"

uint16_t GridTrack::get_track_size() {
  switch (active) {
  case EMPTY_TRACK_TYPE:
    return sizeof(GridTrack);
    break;
  case MD_TRACK_TYPE:
    return sizeof(MDTrack);
    break;
  case A4_TRACK_TYPE:
    return sizeof(A4Track);
    break;
  case EXT_TRACK_TYPE:
    return sizeof(ExtTrack);
    break;
  case A4_TRACK_TYPE_270:
    return sizeof(A4Track_270);
    break;
  case MD_TRACK_TYPE_270:
    return sizeof(MDTrack_270);
    break;
  case EXT_TRACK_TYPE_270:
    return sizeof(ExtTrack_270);
    break;
  }
  return 0;
}

bool GridTrack::load_track_from_grid(uint8_t column, uint16_t row) {

  bool ret;
  uint32_t offset = grid.get_slot_offset(column, row);

  ret = proj.file.seekSet(offset);
  if (!ret) {
    DEBUG_PRINTLN("Seek failed");
    return false;
  }

  uint32_t len = (sizeof(GridTrack));

  DEBUG_PRINTLN("reading grid track");

  ret = mcl_sd.read_data((uint8_t *)this, len, &proj.file);
  if (!ret) {
    DEBUG_PRINTLN("read failed");
    return false;
  }

  len = get_track_size() - len;
  ret = mcl_sd.read_data((uint8_t *)this, len, &proj.file);
  if (!ret) {
    DEBUG_PRINTLN("read failed");
    return false;
  }

  return true;
}

bool GridTrack::store_track_in_grid(uint8_t column, uint16_t row) {

  DEBUG_PRINT_FN();
  bool ret;
  uint32_t offset = grid.get_slot_offset(column, row);

  ret = proj.file.seekSet(offset);
  if (!ret) {
    DEBUG_PRINTLN("seek failed");
    return false;
  }

  uint32_t len = get_track_size();
  ret = mcl_sd.write_data((uint8_t *)(this), len, &proj.file);
  if (!ret) {
    DEBUG_PRINTLN("write failed");
    return false;
  }

  return true;
}
