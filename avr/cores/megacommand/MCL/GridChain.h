#pragma once

#include "MCLMemory.h"
#include <string.h>

#define CHAIN_MANUAL 1
#define CHAIN_AUTO 2
#define CHAIN_QUEUE 3

class GridChain {
public:
  GridChain() { init(); };
  uint8_t pos;
  uint8_t num_of_links;
  uint8_t mode;

  uint8_t lengths[NUM_LINKS];
  uint8_t rows[NUM_LINKS];

  bool is_mode_queue() { return (mode == CHAIN_QUEUE && num_of_links); }

  void init() {
    pos = 0;
    mode = CHAIN_MANUAL;
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

  uint8_t get_row() { return rows[pos]; }

  void inc() {
    if (!is_mode_queue())
      return;
    pos++;
    if (pos == num_of_links) {
      pos = 0;
    }
  }

  uint8_t get_length() { return lengths[pos]; }

  void reset() { pos = num_of_links <= 1 ? 0 : 1; }
};
