#pragma once

#include "platform.h"
#include <inttypes.h>

#define TRACK_LOAD_FADE_FLAG_ENABLED 0x01
#define TRACK_LOAD_FADE_FLAG_OUT 0x02
#define TRACK_LOAD_FADE_TARGET_DEFAULT 0xFF
#define TRACK_LOAD_FADE_DEFAULT_DURATION_Q12 12

class ATTR_PACKED() TrackLoadFadeData {
public:
  uint8_t flags;
  uint8_t target;
  uint16_t duration_q12;
  uint8_t amount;
  int8_t curve;

  void init() {
    flags = 0;
    target = TRACK_LOAD_FADE_TARGET_DEFAULT;
    duration_q12 = TRACK_LOAD_FADE_DEFAULT_DURATION_Q12;
    amount = 127;
    curve = 0;
  }

  bool enabled() const {
    return (flags & TRACK_LOAD_FADE_FLAG_ENABLED) && duration_q12 > 0 &&
           amount > 0;
  }

  bool fade_out() const { return flags & TRACK_LOAD_FADE_FLAG_OUT; }
};

static_assert(sizeof(TrackLoadFadeData) == 6,
              "TrackLoadFadeData storage size changed");
