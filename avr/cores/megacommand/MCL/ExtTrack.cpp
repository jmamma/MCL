#include "ExtTrack.h"
#include "MCL.h"

void ExtTrack::load_immediate(uint8_t tracknumber) {
      store_in_mem(tracknumber);
      load_seq_data(tracknumber);
}

bool ExtTrack::get_track_from_sysex(int tracknumber, uint8_t column) {

  active = EXT_TRACK_TYPE;
  return true;
}

bool ExtTrack::load_seq_data(int tracknumber) {
#ifdef EXT_TRACKS
  if (seq_data.speed == 0) {
    seq_data.speed = EXT_SPEED_2X;
  }
  if (active == EMPTY_TRACK_TYPE) {
    mcl_seq.ext_tracks[tracknumber].clear_track();
  } else {
    mcl_seq.ext_tracks[tracknumber].buffer_notesoff();
    memcpy(&mcl_seq.ext_tracks[tracknumber], &seq_data, sizeof(seq_data));
    mcl_seq.ext_tracks[tracknumber].speed = chain.speed;
    mcl_seq.ext_tracks[tracknumber].length = chain.length;
}
#endif
  return true;
}

bool ExtTrack::store_track_in_grid(int track, int32_t column, int32_t row,
                                   bool online) {
  /*Assign a track to Grid i*/
  /*Extraact track data from received pattern and kit and store in track
   * object*/
  bool ret;

  int b = 0;
  DEBUG_PRINT_FN();
  uint32_t len;

  ret = proj.file.seekSet(offset);
  if (!ret) {
    DEBUG_PRINTLN("Seek failed");
    return false;
  }
#ifdef EXT_TRACKS
  if (online) {
    get_track_from_sysex(track - 16, column - 16);
    chain.length = seq_data.legnth;
    chain.speed = seq_data.speed;
    memcpy(&seq_data, &mcl_seq.ext_tracks[track - 16], sizeof(seq_data));
  }
#endif

  ret = proj.write_grid((uint8_t *)this, sizeof(ExtTrack), col, row);
  if (!ret) {
    DEBUG_PRINTLN("Write failed");
    return false;
  }
  uint8_t model = column;
  grid_page.row_headers[grid_page.cur_row].update_model(column, model,
                                                        EXT_TRACK_TYPE);

  return true;
}
