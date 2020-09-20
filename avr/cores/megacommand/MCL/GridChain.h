#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "SeqTrack.h"

class GridChain_270 {

public:
  uint8_t row;
  uint8_t loops;

  GridChain_270(uint8_t active_ = 0, uint8_t row_ = 0, uint8_t col_ = 0,
            uint8_t loops_ = 0) {}

};


class GridChain {

public:
  uint8_t row;
  uint8_t loops;
  uint8_t length;
  uint8_t speed;
  GridChain(uint8_t active_ = 0, uint8_t row_ = 0, uint8_t col_ = 0,
            uint8_t loops_ = 0) {}
  //Store chain data in chain array.
  void store_in_mem(uint8_t tracknumber, GridChain *chain_array) {
     memcpy(&chain_array[tracknumber], this, sizeof(GridChain));
  }
  void init(uint8_t row) {
  row = row;
  loops = 0;
  length = 16;
  speed = SEQ_SPEED_1X;
  }
};

