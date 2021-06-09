#include "MCL_impl.h"

void GridRowHeader::update_model(int16_t column, uint8_t model_, uint8_t track_type_) {
  model[column] = model_;
  track_type[column] = track_type_;
}

bool GridRowHeader::is_empty() {
  DEBUG_PRINT_FN();
  uint8_t count = 0;
  for (uint8_t x = 0; x < GRID_WIDTH; x++) {
    if (track_type[x] == EMPTY_TRACK_TYPE) {
    count++;
    }
    DEBUG_DUMP(track_type[x]);
  }
  return (count == GRID_WIDTH);
}

void GridRowHeader::init() {
  active = false;
  for (uint8_t x = 0; x < GRID_WIDTH; x++) {
    track_type[x] = EMPTY_TRACK_TYPE;
    model[x] = 0;
  }
}

