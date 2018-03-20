#include "A4Track.h"

bool A4Track::getTrack_from_sysex(int tracknumber, uint8_t column) {

        m_memcpy(&seq_data,&mcl_seq.ext_tracks[tracknumber].seq_data, sizeof(seq_data));
  active = A4_TRACK_TYPE;
}
bool A4Track::placeTrack_in_sysex(int tracknumber, uint8_t column,
                                  A4Sound *analogfour_sound) {
  if (active == A4_TRACK_TYPE) {
    m_memcpy(analogfour_sound, &sound, sizeof(A4Sound));
m_memcpy(&mcl_seq.ext_tracks[tracknumber].seq_data,&seq_data, sizeof(seq_data));
    return true;
  } else {
    return false;
  }
}
bool A4Track::load_track_from_grid(int32_t column, int32_t row, int m) {
  bool ret;
  int b = 0;
  DEBUG_PRINT_FN();
  int32_t offset =
      (int32_t)GRID_SLOT_BYTES +
      (column + (row * (int32_t)GRID_WIDTH)) * (int32_t)GRID_SLOT_BYTES;
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
  bool ret;
  int b = 0;
  DEBUG_PRINT_FN();
  DEBUG_PRINTLN("storing a4 track");
  int32_t len;
  int32_t offset =
      (int32_t)GRID_SLOT_BYTES +
      (column + (row * (int32_t)GRID_WIDTH)) * (int32_t)GRID_SLOT_BYTES;
  ret = proj.file.seekSet(offset);
  if (!ret) {
    DEBUG_PRINTLN("Seek failed");
    return false;
  }

  /*analog 4 tracks*/
  if (Analog4.connected) {
    getTrack_from_sysex(track - 16, column - 16);
    ret = mcl_sd.write_data((uint8_t *)this, sizeof(A4Track), &proj.file);
    if (!ret) {
      return false;
    }
    return true;
  }
}
