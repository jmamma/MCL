#include "MDRouteTrack.h"
#include "MCLSysConfig.h"
#include "MDTrack.h"
#include "MD.h"
#include "MCLActions.h"

void LegacyMDRouteTrack::transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                                         GridSlot slotnumber) {
  GridTrack::transition_load(tracknumber, seq_track, slotnumber);
  if (mcl_actions.send_machine[slotnumber]) {
    load_routes();
    // Send routes regardless.
    send_routes();
  }
}

uint16_t LegacyMDRouteTrack::calc_latency(uint8_t tracknumber) {
  bool send = false;
  return send_routes(send);
}

uint16_t LegacyMDRouteTrack::send_routes(bool send) {
  uint16_t bytes = 0;
  bytes += MD.setTrackRoutings(routing, send);

  return bytes;
}

void LegacyMDRouteTrack::load_immediate(uint8_t tracknumber, SeqTrack *seq_track) {
  load_link_data(seq_track);
  load_routes();
  send_routes();
}


void LegacyMDRouteTrack::load_routes() {
  memcpy(mcl_cfg.routing, routing, sizeof(routing));
  mcl_cfg.poly_mask = poly_mask;
}

void LegacyMDRouteTrack::get_routes() {
  memcpy(routing, mcl_cfg.routing, sizeof(routing));
  poly_mask = mcl_cfg.poly_mask;
}

void LegacyMDRouteTrack::get_online_data(uint8_t merge) {
  get_routes();
  update_link_from_pattern(merge);
}

void MDRouteTrack::clear_ptc_groups() {
  memset(ptc_group, PTC_GROUP_OFF, sizeof(ptc_group));
}

void MDRouteTrack::load_ptc_groups() {
  ptc_groups.load(ptc_group);
  ptc_groups.store(mcl_cfg.ptc_group);
}

void MDRouteTrack::load_legacy_poly_mask(uint16_t poly_mask,
                                         uint8_t poly_channel) {
  PtcGroups groups;
  groups.load_legacy_poly_mask(poly_mask, poly_channel);
  groups.store(ptc_group);
}

uint16_t MDRouteTrack::legacy_poly_mask() const {
  PtcGroups groups;
  groups.load(ptc_group);
  return groups.legacy_poly_mask();
}

void MDRouteTrack::transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                                   GridSlot slotnumber) {
  GridTrack::transition_load(tracknumber, seq_track, slotnumber);
  if (mcl_actions.send_machine[slotnumber]) {
    load_routes();
    // Send routes regardless.
    send_routes();
  } else {
    load_ptc_groups();
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
  load_ptc_groups();
}

void MDRouteTrack::get_routes() {
  memcpy(routing, mcl_cfg.routing, sizeof(routing));
  ptc_groups.store(ptc_group);
  ptc_groups.store(mcl_cfg.ptc_group);
}

void MDRouteTrack::get_online_data(uint8_t merge) {
  get_routes();
  update_link_from_pattern(merge);
}
