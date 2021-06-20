#pragma once

#include "MCLMemory.h"
#include <string.h>

#define CHAIN_MANUAL 1
#define CHAIN_AUTO 2
#define CHAIN_QUEUE 3

class GridChain {
public:
  GridChain() { init(); };
  uint8_t w;
  uint8_t r;
  uint8_t num_of_links;
  uint8_t mode;

  uint8_t lengths[NUM_LINKS];
  uint8_t rows[NUM_LINKS];

  bool is_mode_queue() { return (mode == CHAIN_QUEUE && num_of_links); }

  void init() {
    r = 0;
    w = 0;
    mode = CHAIN_MANUAL;
    num_of_links = 0;
    memset(rows, 255, sizeof(rows));
  }

  bool add(uint8_t row, uint8_t length) {
    rows[w] = row;
    lengths[w] = length;

    if (num_of_links < NUM_LINKS) {
      num_of_links++;
    }
    w++;
    if (w == NUM_LINKS) {
      w = 0;
    }
    return true;
  }

  void set_pos(uint8_t pos) { r = pos; }
  uint8_t get_row() { return rows[r]; }

  void inc() {
    if (!is_mode_queue())
      return;
    r++;
    if (r == num_of_links) {
      r = 0;
    }
  }

  uint8_t get_length() { return lengths[r]; }

  void reset() { r = num_of_links <= 1 ? 0 : 1; }
};
