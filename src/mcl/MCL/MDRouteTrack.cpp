#include "MDRouteTrack.h"
#include "MCLSysConfig.h"
#include "MDTrack.h"
#include "MD.h"
#include "MCLActions.h"

void MDRouteTrack::transition_send(uint8_t tracknumber, uint8_t slotnumber) {
}

void MDRouteTrack::transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                              uint8_t slotnumber) {
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

bool MDRouteTrack::store_in_grid(uint8_t column, uint16_t row,
                                 SeqTrack *seq_track, uint8_t merge,
                                 bool online, Grid *grid) {
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
  ret = write_grid((uint8_t *)(this), len, column, row, grid);

  if (!ret) {
    DEBUG_PRINTLN(F("write failed"));
    return false;
  }
  return true;
}
