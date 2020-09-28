#include "MCL_impl.h"
void MDRouteTrack::transition_send(uint8_t tracknumber, uint8_t slotnumber) {
  send_routes();
}
void MDRouteTrack::transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                                uint8_t slotnumber) {
}

uint16_t MDRouteTrack::calc_latency(uint8_t tracknumber) {
  bool send = false;
  return send_routes(send);
}

uint16_t MDRouteTrack::send_routes(bool send) {
  uint16_t bytes = 0;
  for (uint8_t a = 0; a < NUM_MD_TRACKS; a++) {
    bytes += MD.setTrackRouting(a, routing[a], send);
  }
  return bytes;
}

void MDRouteTrack::load_immediate(uint8_t tracknumber, SeqTrack *seq_track) {
  memcpy(mcl_cfg.routing, routing, sizeof(routing));
  send_routes();
}

void MDRouteTrack::get_routes() {
  memcpy(routing, mcl_cfg.routing, sizeof(routing));
}

bool MDRouteTrack::store_in_grid(uint8_t column, uint16_t row, SeqTrack *seq_track,
                              uint8_t merge, bool online) {
  active = MDROUTE_TRACK_TYPE;
  bool ret;
  int b = 0;
  DEBUG_PRINT_FN();
  uint32_t len;

  if (column != 255 && online == true) {
    get_routes();
  }

  len = sizeof(MDRouteTrack);
  DEBUG_PRINTLN(len);

  ret = proj.write_grid((uint8_t *)(this), len, column, row);

  if (!ret) {
    DEBUG_PRINTLN(F("write failed"));
    return false;
  }
  return true;
}
