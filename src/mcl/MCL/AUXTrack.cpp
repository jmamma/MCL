#include "AUXTrack.h"

bool AUXTrack::store_in_grid(uint8_t column, uint16_t row,
                              SeqTrack *seq_track, uint8_t merge,
                              bool online, Grid *grid) {
  if (column != 255 && online) {
    get_online_data(merge);
  }
  if (!write_grid(_this(), get_track_size(), column, row, grid)) {
    DEBUG_PRINTLN(F("write failed"));
    return false;
  }
  return true;
}
