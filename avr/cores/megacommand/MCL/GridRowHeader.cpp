#include "MCL.h"
#include "GridRowHeader.h"

void GridRowHeader::update_model(int16_t column, uint8_t model_, uint8_t track_type_) {
  model[column] = model_;
  track_type[column] = track_type_;
}

bool GridRowHeader::write(int16_t row) {
  bool ret;
  uint32_t offset = grid.get_header_offset(row);

  ret = proj.seek_grid(offset);

  if (!ret) {
    DEBUG_PRINT_FN();
    DEBUG_PRINTLN("write row header fail; ");
    return false;
  }

  ret = proj.write_grid((uint8_t *)(this), sizeof(GridRowHeader));

  return ret;
}

bool GridRowHeader::read(int16_t row) {
  bool ret;
  uint32_t offset = grid.get_header_offset(row);

  ret = proj.seek_grid(offset);
  if (!ret) {
    DEBUG_PRINT_FN();
    DEBUG_PRINTLN("read row header fail; ");
    return false;
  }

  ret = proj.read_grid((uint8_t *)(this), sizeof(GridRowHeader));

  return ret;
}

bool GridRowHeader::is_empty() {
  DEBUG_PRINT_FN();
  uint8_t count = 0;
  for (uint8_t x = 0; x < GRID_WIDTH; x++) {
    if (track_type[x] == 0xFF) {
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

