#include "AUXTrack.h"
#include "../Drivers/MD/MD.h"
#include "MDTrack.h"

void AUXTrack::update_link_from_pattern(uint8_t merge) {
  if (merge == SAVE_MD_PATTERN_IMPORT) {
    link.length = MD.pattern.patternLength;
    link.set_speed(SEQ_SPEED_1X + MD.pattern.doubleTempo);
  }
}

bool AUXTrack::store_in_grid(GridSlot column, GridRow row,
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
