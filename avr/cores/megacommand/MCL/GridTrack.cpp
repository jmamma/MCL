#include "MCL.h"
#include "GridTrack.h"


bool GridTrack::load_track_from_grid(int32_t column, int32_t row) {

  bool ret;
  int b = 0;

//  DEBUG_PRINT_FN();
  int32_t offset = grid.get_slot_offset(column, row);

  int32_t len;

  ret = proj.file.seekSet(offset);
  if (!ret) {
    DEBUG_PRINTLN("Seek failed");
    return false;
  }

  len = (sizeof(GridTrack));

  // len = (sizeof(GridTrack)  - (LOCK_AMOUNT * 3));

  ret = mcl_sd.read_data((uint8_t *)this, len, &proj.file);

  if (!ret) {
    DEBUG_PRINTLN("read failed");
    return false;
  }
  return true;
}

bool GridTrack::store_track_in_grid(int32_t column, int32_t row) {
  /*Assign a track to Grid i*/
  /*Extraact track data from received pattern and kit and store in track
   * object*/

  bool ret;
  int b = 0;
  DEBUG_PRINT_FN();
  int32_t len;

  int32_t offset = grid.get_slot_offset(column, row);

  ret = proj.file.seekSet(offset);
  if (!ret) {
    DEBUG_PRINTLN("seek failed");
    return false;
  }

  len = sizeof(GridTrack);
  ret = mcl_sd.write_data((uint8_t *)(this), len, &proj.file);
  if (!ret) {
    DEBUG_PRINTLN("write failed");
    return false;
  }

  return true;
}
