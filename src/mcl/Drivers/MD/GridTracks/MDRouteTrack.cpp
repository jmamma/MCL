#include "MDRouteTrack.h"
#include "MCLSysConfig.h"
#include "MDTrack.h"
#include "MD.h"
#include "MCLActions.h"

void MDRouteTrack::transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                                  GridSlot slotnumber) {
  GridTrack::transition_load(tracknumber, seq_track, slotnumber);
   if (mcl_actions.send_machine[slotnumber]) {
    load_routes();
    //Send routes regardless
    send_routes();
  }
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
  load_routes();
  send_routes();
}


void MDRouteTrack::load_routes() {
  memcpy(mcl_cfg.routing, routing, sizeof(routing));
  mcl_cfg.poly_mask = poly_mask;
}

void MDRouteTrack::get_routes() {
  memcpy(routing, mcl_cfg.routing, sizeof(routing));
  poly_mask = mcl_cfg.poly_mask;
}

void MDRouteTrack::get_online_data(uint8_t merge) {
  get_routes();
  update_link_from_pattern(merge);
}
