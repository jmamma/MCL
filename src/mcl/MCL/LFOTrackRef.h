#pragma once

#include "LFOSeqTrack.h"
#include <inttypes.h>

class LFOTrackRef {
public:
  static LFOSeqTrack &current_track();
  static bool select_track(uint8_t track);

  static uint8_t track_count(uint8_t device_slot);

  static uint8_t target_count(uint8_t device_slot);
  static uint8_t param_count(uint8_t device_slot, uint8_t dest);
  static bool get_param(uint8_t device_slot, uint8_t dest, uint8_t param,
                        uint8_t *value);

  static void set_key_repeat(uint8_t enabled);
  static void sync_panel(const LFOSeqTrack &track);
  static bool supports_trig_port(uint8_t port);
};
