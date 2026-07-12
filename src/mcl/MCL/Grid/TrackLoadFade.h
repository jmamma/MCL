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
  uint8_t reserved[2];

  void init() {
    flags = 0;
    target = TRACK_LOAD_FADE_TARGET_DEFAULT;
    duration_q12 = TRACK_LOAD_FADE_DEFAULT_DURATION_Q12;
    amount = 127;
    curve = 0;
    reserved[0] = 0;
    reserved[1] = 0;
  }

  bool enabled() const {
    return (flags & TRACK_LOAD_FADE_FLAG_ENABLED) && duration_q12 > 0 &&
           amount > 0;
  }

  bool fade_out() const { return flags & TRACK_LOAD_FADE_FLAG_OUT; }

  uint16_t elapsed_q12() const {
    return (uint16_t)reserved[0] | ((uint16_t)reserved[1] << 8);
  }

  void set_elapsed_q12(uint16_t elapsed) {
    reserved[0] = (uint8_t)(elapsed & 0xFF);
    reserved[1] = (uint8_t)(elapsed >> 8);
  }
};

static_assert(sizeof(TrackLoadFadeData) == 8,
              "TrackLoadFadeData storage size changed");
