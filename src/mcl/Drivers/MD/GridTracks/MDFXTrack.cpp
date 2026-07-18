#include "MDFXTrack.h"
#include "MD.h"

void MDFXTrack::paste_track(uint8_t src_track, uint8_t dest_track,
                          SeqTrack *seq_track) {
  load_link_data(seq_track);
  send_fx(true);
}

void MDFXTrack::transition_load(uint8_t tracknumber, SeqTrack *seq_track,
                                GridSlot slotnumber) {
  GridTrack::transition_load(tracknumber, seq_track, slotnumber);
  // load_seq_data(seq_track);
}

bool MDFXTrack::transition_cache(uint8_t tracknumber, GridSlot slotnumber) {
  bool send = true;
  MD.assignFXParamsBulk(reverb, send);
  return true;
}

uint16_t MDFXTrack::calc_latency(uint8_t tracknumber) {
  bool send = false;
  return MD.assignFXParamsBulk(reverb, send);
}

uint16_t MDFXTrack::send_fx(bool send) {
  uint16_t bytes = 0;

  bytes += MD.sendFXParamsBulk(reverb, send);
  /*
    if (enable_reverb) {
      bytes += MD.setReverbParams(reverb, send);
    }
    if (enable_delay) {
      bytes += MD.setEchoParams(delay, send);
    }
    if (enable_eq) {
      bytes += MD.setEQParams(eq, send);
    }
    if (enable_dynamics) {
      bytes += MD.setCompressorParams(dynamics, send);
    }
  */
  if (send) { place_fx_in_kit(); }
  return bytes;
}

void MDFXTrack::load_immediate(uint8_t tracknumber, SeqTrack *seq_track) {
  load_link_data(seq_track);
  place_fx_in_kit();
}

void MDFXTrack::load_immediate_cleared(uint8_t tracknumber, SeqTrack *seq_track) {
  load_link_data(seq_track);
}

void MDFXTrack::place_fx_in_kit() {
  DEBUG_PRINTLN("place");
  memcpy(MD.kit.reverb, reverb, sizeof(reverb) * 4);
  memcpy(MD.kit.fx_orig, reverb, sizeof(reverb) * 4);
#if !defined(__AVR__)
  memcpy(MD.kit.userBusFx, userBusFx, sizeof(userBusFx));
  memcpy(MD.kit.userPostFx, userPostFx, sizeof(userPostFx));
  memcpy(MD.kit.userBusFxOrig, userBusFx, sizeof(userBusFx));
  memcpy(MD.kit.userPostFxOrig, userPostFx, sizeof(userPostFx));
#endif
  /*
  if (enable_reverb) {
    memcpy(&MD.kit.reverb, &reverb, sizeof(reverb));
  }
  if (enable_delay) {
    memcpy(&MD.kit.delay, &delay, sizeof(delay));
  }
  if (enable_eq) {
    memcpy(&MD.kit.eq, &eq, sizeof(eq));
  }
  if (enable_dynamics) {
    memcpy(&MD.kit.dynamics, &dynamics, sizeof(dynamics));
  }
  */
}

void MDFXTrack::get_fx_from_kit_extra(KitExtra *kit_extra) {
  memcpy(&reverb, &kit_extra->reverb, 32);
#if !defined(__AVR__)
  memcpy(userBusFx, kit_extra->userBusFx, sizeof(userBusFx));
  memcpy(userPostFx, kit_extra->userPostFx, sizeof(userPostFx));
#endif
  /*
  enable_reverb = true;
  enable_delay = true;
  enable_eq = true;
  enable_dynamics = true;
  */
}

void MDFXTrack::get_fx_from_kit() {
  memcpy(&reverb, &MD.kit.reverb, 32);
#if !defined(__AVR__)
  memcpy(userBusFx, MD.kit.userBusFx, sizeof(userBusFx));
  memcpy(userPostFx, MD.kit.userPostFx, sizeof(userPostFx));
#endif
  /*
  enable_reverb = true;
  enable_delay = true;
  enable_eq = true;
  enable_dynamics = true;
  */
}

void MDFXTrack::get_online_data(uint8_t merge) {
  get_fx_from_kit();
  update_link_from_pattern(merge);
}
