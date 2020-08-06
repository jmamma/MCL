#include "A4Track.h"
#include "MCL.h"
#include "MCLSeq.h"
//#include "MCLSd.h"

void A4Track::load_seq_data(int tracknumber) {
#ifdef EXT_TRACKS
  if (seq_data.speed == 0) {
    seq_data.speed = EXT_SPEED_2X;
  }
  if (active == EMPTY_TRACK_TYPE) {
    mcl_seq.ext_tracks[tracknumber].clear_track();
  } else {
    mcl_seq.ext_tracks[tracknumber].buffer_notesoff();
    memcpy(&mcl_seq.ext_tracks[tracknumber], &seq_data, sizeof(seq_data));
  }
#endif
  return true;
}

bool A4Track::get_track_from_sysex(int tracknumber, uint8_t column) {

  active = A4_TRACK_TYPE;
}

bool A4Track::store_track_in_grid(int32_t column, int32_t row, int track,
                                  bool online) {
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
#ifdef EXT_TRACKS
  if (online) {
    if (Analog4.connected) {
      if (track != 255) {
        get_track_from_sysex(track - 16, column - 16);
      }
    }
    memcpy(&seq_data, &mcl_seq.ext_tracks[track - 16], sizeof(seq_data));

    chain.length = seq_data.legnth;
    chain.speed = seq_data.speed;
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
