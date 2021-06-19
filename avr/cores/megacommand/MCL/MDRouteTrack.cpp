#include "MCL_impl.h"
void MDRouteTrack::transition_send(uint8_t tracknumber, uint8_t slotnumber) {
  send_data();
}

uint16_t MDRouteTrack::calc_latency(uint8_t tracknumber) {
  bool send = false;
  return send_routes(send);
}

uint16_t MDRouteTrack::send_routes(bool send) {
  uint16_t bytes = 0;
  bytes += MD.setTrackRoutings(routing, send);
  return bytes;
}

void MDRouteTrack::load_immediate(uint8_t tracknumber, SeqTrack *seq_track) {
  load_link_data(seq_track);
  send_data();
}

void MDRouteTrack::send_data() {
  memcpy(mcl_cfg.routing, routing, sizeof(routing));
  mcl_cfg.poly_mask = poly_mask;
  send_routes();
}

void MDRouteTrack::get_routes() {
  memcpy(routing, mcl_cfg.routing, sizeof(routing));
  poly_mask = mcl_cfg.poly_mask;
}

bool MDRouteTrack::store_in_grid(uint8_t column, uint16_t row,
                                 SeqTrack *seq_track, uint8_t merge,
                                 bool online) {
  active = MDROUTE_TRACK_TYPE;
  bool ret;
  int b = 0;
  DEBUG_PRINT_FN();
  uint32_t len;

  if (column != 255 && online == true) {
    get_routes();
    if (merge == SAVE_MD) {
      link.length = MD.pattern.patternLength;
      link.speed = SEQ_SPEED_1X + MD.pattern.doubleTempo;
    }
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
