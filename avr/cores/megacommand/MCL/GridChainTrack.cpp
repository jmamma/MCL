#include "MCL_impl.h"


void GridChainTrack::load_immediate(uint8_t tracknumber, SeqTrack *seq_track) {
  load_link_data(seq_track);
  get_chains();
}

void GridChainTrack::place_chains() {
  memcpy(&mcl_actions.chains, &chains, sizeof(GridChain));
}


void GridChainTrack::get_chains() {
  memcpy(&chains, &mcl_actions.chains, sizeof(GridChain));
}

bool GridChainTrack::store_in_grid(uint8_t column, uint16_t row, SeqTrack *seq_track,
                              uint8_t merge, bool online) {
  active = GRIDCHAIN_TRACK_TYPE;
  bool ret;
  int b = 0;
  DEBUG_PRINT_FN();
  uint32_t len;

  if (column != 255 && online == true) {
    get_chains();
  }

  len = sizeof(GridChainTrack);
  DEBUG_PRINTLN(len);

  ret = proj.write_grid((uint8_t *)(this), len, column, row);

  if (!ret) {
    DEBUG_PRINTLN(F("write failed"));
    return false;
  }
  return true;
}
