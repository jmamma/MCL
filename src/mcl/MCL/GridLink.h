#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "SeqTrack.h"

class ATTR_PACKED() GridLink {

public:
  static constexpr uint8_t SPEED_MASK = 0x7F;
  static constexpr uint8_t SEQ_ONLY_FLAG = 0x80;

  uint8_t row;
  uint8_t loops;
  uint8_t length;
  uint8_t speed;
  GridLink(uint8_t active_ = 0, uint8_t row_ = 0, uint8_t col_ = 0,
            uint8_t loops_ = 0) {}
  //Store link data in link array.
  void store_in_mem(uint8_t tracknumber, GridLink *link_array) {
     memcpy(&link_array[tracknumber], this, sizeof(GridLink));
  }
  uint8_t speed_value() const { return speed & SPEED_MASK; }
  void set_speed(uint8_t speed_) {
    speed = (speed & SEQ_ONLY_FLAG) | (speed_ & SPEED_MASK);
  }
  bool load_sound() const { return (speed & SEQ_ONLY_FLAG) == 0; }
  void set_load_sound(bool enabled) {
    if (enabled) {
      speed &= SPEED_MASK;
    } else {
      speed |= SEQ_ONLY_FLAG;
    }
  }
  void init(uint8_t row_, uint8_t loops_ = 0, uint8_t length_ = 16, uint8_t speed_ = SEQ_SPEED_1X) {
  row = row_;
  loops = loops_;
  length = length_;
  speed = speed_ & SPEED_MASK;
  }
};

static_assert(sizeof(GridLink) == 4, "GridLink storage layout changed");
