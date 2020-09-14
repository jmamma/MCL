#include "MCL_impl.h"

void MDFXTrack::load_immediate(uint8_t tracknumber) {
  place_fx_in_kit();
}

void MDFXTrack::place_fx_in_kit() {
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

bool MDFXTrack::store_in_grid(uint8_t column, uint16_t row, SeqTrack *seq_track, uint8_t merge,
                              bool online) {
  active = MDFX_TRACK_TYPE;
  bool ret;
  int b = 0;
  DEBUG_PRINT_FN();
  uint32_t len;

  if (column != 255 && online == true) {
    get_fx_from_kit();
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
