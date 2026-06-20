#include "GridChainTrack.h"
#include "Grid/MCLActions.h"
#include "Project.h"

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

void GridChainTrack::get_online_data(uint8_t merge) {
  get_chains();
}

bool GridChainTrack::store_in_grid(GridSlot column, GridRow row, SeqTrack *seq_track,
                              uint8_t merge, bool online) {
  active = GRIDCHAIN_TRACK_TYPE;
  bool ret;
  DEBUG_PRINT_FN();

  if (column != 255 && online == true) {
    get_chains();
  }

  ret = proj.write_grid(_this(), _sizeof(), column, row);

  if (!ret) {
    DEBUG_PRINTLN(F("write failed"));
    return false;
  }
  return true;
}
