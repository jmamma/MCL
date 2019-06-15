#include "ExtTrack.h"
#include "MCL.h"

bool ExtTrack::get_track_from_sysex(int tracknumber, uint8_t column) {

  memcpy(&seq_data, &mcl_seq.ext_tracks[tracknumber],
           sizeof(seq_data));
  active = EXT_TRACK_TYPE;
  return true;
}
bool ExtTrack::place_track_in_sysex(int tracknumber, uint8_t column) {

  memcpy(&mcl_seq.ext_tracks[tracknumber], &seq_data,
           sizeof(seq_data));
  return true;
}
bool ExtTrack::load_track_from_grid(int32_t column, int32_t row, int m) {
  bool ret;
  int b = 0;

  int32_t offset = grid.get_slot_offset(column, row);

  int32_t len;
  ret = proj.file.seekSet(offset);
  if (!ret) {
    DEBUG_PRINT_FN();
    DEBUG_PRINTLN("Seek failed");
    return false;
  }
  if (m > 0) {
    ret = mcl_sd.read_data((uint8_t *)(this), m, &proj.file);
  } else {
    ret = mcl_sd.read_data((uint8_t *)(this), sizeof(ExtTrack), &proj.file);
  }

  if (!ret) {
    DEBUG_PRINT_FN();
    DEBUG_PRINTLN("Read failed");
    return false;
  }
  return true;
}
bool ExtTrack::store_track_in_grid(int track, int32_t column, int32_t row, bool online) {
  /*Assign a track to Grid i*/
  /*Extraact track data from received pattern and kit and store in track
   * object*/
  bool ret;

  int b = 0;
  DEBUG_PRINT_FN();
  int32_t len;

  int32_t offset = (column + (row * (int32_t)GRID_WIDTH)) * (int32_t)GRID_SLOT_BYTES;

  ret = proj.file.seekSet(offset);
  if (!ret) {
    DEBUG_PRINTLN("Seek failed");
    return false;
  }

  if (online) { get_track_from_sysex(track - 16, column - 16); }
  ret = mcl_sd.write_data((uint8_t *)this, sizeof(ExtTrack), &proj.file);
  if (!ret) {
    DEBUG_PRINTLN("Write failed");
    return false;
  }
  uint8_t model = column;
  grid_page.row_headers[grid_page.cur_row].update_model(column, model, EXT_TRACK_TYPE);

  return true;
}
