#pragma once

#include "MCLMemory.h"
#include <string.h>

class GridChain {
public:
  GridChain() { init(); };
  uint8_t pos;
  uint8_t num_of_links;

  uint8_t lengths[NUM_LINKS];
  uint8_t rows[NUM_LINKS];

  void init() {
    pos = 0;
    num_of_links = 0;
    memset(rows, 255, sizeof(rows));
  }

  bool add(uint8_t row, uint8_t length) {
    if (num_of_links == NUM_LINKS) {
      num_of_links = 0;
    }
    rows[num_of_links] = row;
    lengths[num_of_links] = length;

    num_of_links++;
    return true;
  }

  uint8_t get() {
   return rows[pos];
  }

  uint8_t inc() {
    pos++;
    if (pos == num_of_links) {
      pos = 0;
    }
  }

  uint8_t get_length() {
    return lengths[pos];
  }

  void reset() { pos = 0; }
};
