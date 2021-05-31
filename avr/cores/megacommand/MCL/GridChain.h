#pragma once

#include <string.h>
#include "MCLMemory.h"

class GridChain {

public:
  uint8_t lengths[NUM_SLOTS];
  uint8_t dst[NUM_SLOTS][NUM_LINKS];

  GridChain() { init(); }
  //Store link data in link array.
  void init() {
    memset(lengths, 0, sizeof(lengths));
    memset(dst, 255, sizeof(dst));
  }

  bool add_link(uint8_t slot, uint8_t row) {
     uint8_t len = lengths[slot];
     if (len == NUM_LINKS) {
       return false;
     }
     dst[slot][len] = row;
     lengths[slot] += 1;
  }

};

