/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#pragma once

#include "MCLMemory.h"

#define GRID_ROW_HEADER_LENGTH (1 + 17 + GRID_WIDTH + GRID_WIDTH)

class ATTR_PACKED() GridRowHeader {
 public:
  bool active;
  char name[17];
  uint8_t track_type[GRID_WIDTH];
  uint8_t model[GRID_WIDTH];
  void* _this() { return &active; }
  void update_model(int16_t column, uint8_t model_, uint8_t track_type_);
  bool is_empty();
  void init();

};

