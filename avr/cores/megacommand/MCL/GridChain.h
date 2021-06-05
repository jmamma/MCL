#pragma once

#include "MCLMemory.h"
#include <string.h>

class GridChain {
public:
  GridChain() { init(); };
  uint8_t pos;
  uint8_t length;

  uint8_t lengths[NUM_LINKS];
  uint8_t rows[NUM_LINKS];

  void init() {
    pos = 0;
    length = 0;
    memset(rows, 255, sizeof(rows));
  }

  bool add(uint8_t row) {
    if (length == NUM_LINKS) {
      length = 0;
    }
    rows[length] = row;
    length++;
    return true;
  }

  uint8_t get_next() {
    if (pos == length) {
      pos = 0;
    }
    return rows[pos++];
  }

  void reset() { pos = 0; }
};


