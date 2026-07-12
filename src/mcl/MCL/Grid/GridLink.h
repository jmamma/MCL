#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "MCLMemory.h"
#include "Sequencer/SeqTrack.h"

class ATTR_PACKED() GridLink {

public:
  GridRow row;
  uint8_t loops;
  uint8_t length;
  uint8_t speed;
  GridLink(uint8_t active_ = 0, GridRow row_ = 0, GridColumn col_ = 0,
            uint8_t loops_ = 0) {}
  //Store link data in link array.
  void store_in_mem(uint8_t tracknumber, GridLink *link_array) {
     memcpy(&link_array[tracknumber], this, sizeof(GridLink));
  }
  uint8_t speed_value() const { return speed & 0x7F; }
  void set_speed(uint8_t speed_) {
    speed = (speed & 0x80) | (speed_ & 0x7F);
  }
  void init(GridRow row_, uint8_t loops_ = 0, uint8_t length_ = 16, uint8_t speed_ = SEQ_SPEED_1X) {
  row = row_;
  loops = loops_;
  length = length_;
  speed = speed_;
  }
};

static_assert(sizeof(GridLink) == 4, "GridLink storage layout changed");
