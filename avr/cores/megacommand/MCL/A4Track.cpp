#include "A4Track.h"
#include "MCL.h"
#include "MCLSeq.h"
//#include "MCLSd.h"

void A4Track::load_seq_data(uint8_t tracknumber) {
#ifdef EXT_TRACKS
  if (seq_data.resolution == 0) { seq_data.resolution = 1; }
  if (active == EMPTY_TRACK_TYPE) {
    mcl_seq.ext_tracks[tracknumber].clear_track();
  } else {
    mcl_seq.ext_tracks[tracknumber].buffer_notesoff();
    memcpy(&mcl_seq.ext_tracks[tracknumber], &seq_data, sizeof(seq_data));
  }
#endif
}

bool A4Track::get_track_from_sysex(uint8_t tracknumber, uint8_t column) {

  active = A4_TRACK_TYPE;
}
bool A4Track::place_track_in_sysex(uint8_t tracknumber, uint8_t column,
                                   A4Sound *analogfour_sound) {
  if (active == A4_TRACK_TYPE) {
    memcpy(analogfour_sound, &sound, sizeof(A4Sound));
    load_seq_data(tracknumber);
    return true;
  } else {
    return false;
  }
}
bool A4Track::load_track_from_grid(uint8_t column, uint8_t row, int m) {
  bool ret;
  int32_t offset = grid.get_slot_offset(column, row);
  ret = proj.file.seekSet(offset);
  if (!ret) {
    return false;
  }
  if (m > 0) {
    ret = mcl_sd.read_data((uint8_t *)(this), m, &proj.file);
  } else {
    ret = mcl_sd.read_data((uint8_t *)(this), A4_TRACK_LEN, &proj.file);
  }
  if (!ret) {
    return false;
  }
  if (active == EMPTY_TRACK_TYPE) {
  seq_data.length = 16;
  }

  return true;
}
bool A4Track::store_track_in_grid(uint8_t column, uint8_t row, uint8_t track,
                                  bool online) {
  /*Assign a track to Grid i*/
  /*Extraact track data from received pattern and kit and store in track
   * object*/
  active = A4_TRACK_TYPE;

  bool ret;
  int32_t offset = grid.get_slot_offset(column, row);
  ret = proj.file.seekSet(offset);
  if (!ret) {
    return false;
  }

  /*analog 4 tracks*/
#ifdef EXT_TRACKS
  if (online) {
    if (Analog4.connected) {
      if (track != 255) {
        get_track_from_sysex(track - 16, column - 16);
      }
    }
    memcpy(&seq_data, &mcl_seq.ext_tracks[track - 16], sizeof(seq_data));
  }
#endif
  ret = mcl_sd.write_data((uint8_t *)this, A4_TRACK_LEN, &proj.file);
  if (!ret) {
    return false;
  }
  grid_page.row_headers[grid_page.cur_row].update_model(column, column,
                                                        A4_TRACK_TYPE);
  return true;
}
