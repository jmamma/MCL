#include "A4Track.h"
#include "MCL.h"
#include "MCLSeq.h"
//#include "MCLSd.h"

bool A4Track::get_track_from_sysex(int tracknumber, uint8_t column) {

        m_memcpy(&seq_data,&mcl_seq.ext_tracks[tracknumber], sizeof(seq_data));
  active = A4_TRACK_TYPE;
}
bool A4Track::place_track_in_sysex(int tracknumber, uint8_t column,
                                  A4Sound *analogfour_sound) {
  if (active == A4_TRACK_TYPE) {
    m_memcpy(analogfour_sound, &sound, sizeof(A4Sound));
m_memcpy(&mcl_seq.ext_tracks[tracknumber],&seq_data, sizeof(seq_data));
    return true;
  } else {
    return false;
  }
}
bool A4Track::load_track_from_grid(int32_t column, int32_t row, int m) {
  bool ret;
  int b = 0;
  DEBUG_PRINT_FN();
  int32_t offset = grid.get_slot_offset(column, row);
  int32_t len;
  ret = proj.file.seekSet(offset);
  if (!ret) {
    DEBUG_PRINTLN("Seek failed");
    return false;
  }
  if (Analog4.connected) {
    if (m > 0) {
      ret = mcl_sd.read_data((uint8_t *)(this), m, &proj.file);
    } else {
      ret = mcl_sd.read_data((uint8_t *)(this), sizeof(A4Track), &proj.file);
    }
    if (!ret) {
      DEBUG_PRINTLN("Write failed");
      return false;
    }
    return true;
  }
  return false;
}
bool A4Track::store_track_in_grid(int track, int32_t column, int32_t row) {
  /*Assign a track to Grid i*/
  /*Extraact track data from received pattern and kit and store in track
   * object*/
  active = A4_TRACK_TYPE;

  bool ret;
  int b = 0;
  DEBUG_PRINT_FN();
  DEBUG_PRINTLN("storing a4 track");
  int32_t len;
  int32_t offset = grid.get_slot_offset(column, row);
  ret = proj.file.seekSet(offset);
  if (!ret) {
    DEBUG_PRINTLN("Seek failed");
    return false;
  }

  /*analog 4 tracks*/
  if (Analog4.connected) {
    get_track_from_sysex(track - 16, column - 16);
    ret = mcl_sd.write_data((uint8_t *)this, sizeof(A4Track), &proj.file);
    if (!ret) {
      return false;
    }
   uint8_t model = track - 16 + 1;
    grid_page.row_headers[grid_page.cur_row].update_model(column, model, DEVICE_A4);
    return true;
  }
}
