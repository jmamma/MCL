#include "ExtTrack.h"
#include "MCL.h"

void ExtTrack::load_immediate(uint8_t tracknumber) {
      store_in_mem(tracknumber);
      load_seq_data(tracknumber);
}

bool ExtTrack::get_track_from_sysex(uint8_t tracknumber) {

  active = EXT_TRACK_TYPE;
  return true;
}

bool ExtTrack::load_seq_data(uint8_t tracknumber) {
#ifdef EXT_TRACKS
  if (chain.speed == 0) {
    chain.speed = SEQ_SPEED_2X;
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

bool ExtTrack::store_track_in_grid(uint8_t tracknumber, uint16_t row,
                                   bool online) {
  /*Assign a track to Grid i*/
  /*Extraact track data from received pattern and kit and store in track
   * object*/
  bool ret;

  int b = 0;
  DEBUG_PRINT_FN();
  uint32_t len;

#ifdef EXT_TRACKS
  if (online) {
    get_track_from_sysex(tracknumber);
    chain.length = mcl_seq.ext_tracks[tracknumber].length;
    chain.speed = mcl_seq.ext_tracks[tracknumber].speed;
    memcpy(&seq_data, &mcl_seq.ext_tracks[tracknumber], sizeof(seq_data));
  }
#endif

  ret = proj.write_grid((uint8_t *)this, sizeof(ExtTrack), tracknumber, row);
  if (!ret) {
    DEBUG_PRINTLN("Write failed");
    return false;
  }
  uint8_t model = tracknumber;
  grid_page.row_headers[grid_page.cur_row].update_model(tracknumber, model,
                                                        EXT_TRACK_TYPE);

  return true;
}
