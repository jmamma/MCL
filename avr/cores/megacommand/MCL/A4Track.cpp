#include "A4Track.h"
#include "MCL.h"
#include "MCLSeq.h"
//#include "MCLSd.h"

void A4Track::load_seq_data(int tracknumber) {
  mcl_seq.ext_tracks[tracknumber].buffer_notesoff();
  memcpy(&mcl_seq.ext_tracks[tracknumber], &seq_data, sizeof(seq_data));
}

bool A4Track::get_track_from_sysex(int tracknumber, uint8_t column) {

  memcpy(&seq_data, &mcl_seq.ext_tracks[tracknumber], sizeof(seq_data));
  active = A4_TRACK_TYPE;
}
bool A4Track::place_track_in_sysex(int tracknumber, uint8_t column,
                                   A4Sound *analogfour_sound) {
  if (active == A4_TRACK_TYPE) {
    memcpy(analogfour_sound, &sound, sizeof(A4Sound));
    load_seq_data(tracknumber);
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
bool A4Track::store_track_in_grid(int32_t column, int32_t row, int track) {
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
    if (track != 255) {
      get_track_from_sysex(track - 16, column - 16);
    }
  }
  ret = mcl_sd.write_data((uint8_t *)this, sizeof(A4Track), &proj.file);
  if (!ret) {
    return false;
  }
  grid_page.row_headers[grid_page.cur_row].update_model(column, column,
                                                        A4_TRACK_TYPE);
  return true;
}

bool A4Track::store_in_mem(uint8_t column, uint32_t region) {

  uint32_t pos =
      region + A4_TRACK_LEN * (uint32_t)(column - 16);

  volatile uint8_t *ptr;

  ptr = reinterpret_cast<uint8_t *>(pos);
  memcpy_bank1(ptr, this, sizeof(A4Track));
  return true;
}

bool A4Track::load_from_mem(uint8_t column, uint32_t region) {
  uint32_t mdlen =
      sizeof(GridTrack) + sizeof(MDSeqTrackData) + sizeof(MDMachine);

  uint32_t pos =
      region + mdlen * 16 + sizeof(A4Track) * (uint32_t)(column - 16);

  volatile uint8_t *ptr;

  ptr = reinterpret_cast<uint8_t *>(pos);

  memcpy_bank1(this, ptr, sizeof(A4Track));
  return true;
}
