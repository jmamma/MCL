#include "MCL_impl.h"

void MDFXTrack::transition_send(uint8_t tracknumber, uint8_t slotnumber) {
    send_fx();
}

uint16_t MDFXTrack::calc_latency(uint8_t tracknumber) {
  bool send = false;
  return send_fx(send);
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

void MDFXTrack::place_fx_in_kit() {
  DEBUG_PRINTLN("place");
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
}

void MDFXTrack::get_fx_from_kit_extra(KitExtra *kit_extra) {
  memcpy(&reverb, &kit_extra->reverb, sizeof(reverb));
  memcpy(&delay, &kit_extra->delay, sizeof(delay));
  memcpy(&eq, &kit_extra->eq, sizeof(eq));
  memcpy(&dynamics, &kit_extra->dynamics, sizeof(dynamics));
  enable_reverb = true;
  enable_delay = true;
  enable_eq = true;
  enable_dynamics = true;
}

void MDFXTrack::get_fx_from_kit() {
  memcpy(&reverb, &MD.kit.reverb, sizeof(reverb));
  memcpy(&delay, &MD.kit.delay, sizeof(delay));
  memcpy(&eq, &MD.kit.eq, sizeof(eq));
  memcpy(&dynamics, &MD.kit.dynamics, sizeof(dynamics));
  enable_reverb = true;
  enable_delay = true;
  enable_eq = true;
  enable_dynamics = true;
}

bool MDFXTrack::store_in_grid(uint8_t column, uint16_t row, SeqTrack *seq_track,
                              uint8_t merge, bool online) {
  active = MDFX_TRACK_TYPE;
  bool ret;
  int b = 0;
  DEBUG_PRINT_FN();
  uint32_t len;

  if (column != 255 && online == true) {
    get_fx_from_kit();
    if (merge == SAVE_MD) {
        link.length = MD.pattern.patternLength;
        link.speed = SEQ_SPEED_1X + MD.pattern.doubleTempo;
    }
  }

  len = sizeof(MDFXTrack);
  DEBUG_PRINTLN(len);

  ret = proj.write_grid((uint8_t *)(this), len, column, row);

  if (!ret) {
    DEBUG_PRINTLN(F("write failed"));
    return false;
  }
  return true;
}
