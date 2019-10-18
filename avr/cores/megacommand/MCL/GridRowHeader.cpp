#include "MCL.h"
#include "GridRowHeader.h"

void GridRowHeader::update_model(int16_t column, uint8_t model_, uint8_t track_type_) {
  model[column] = model_;
  track_type[column] = track_type_;
}

bool GridRowHeader::write(int16_t row) {
  bool ret;
  int32_t offset = grid.get_header_offset(row);

  ret = proj.file.seekSet(offset);
  if (!ret) {
    DEBUG_PRINT_FN();
    DEBUG_PRINTLN("write row header fail; ");
    return false;
  }

  ret = mcl_sd.write_data((uint8_t *)(this), sizeof(GridRowHeader), &proj.file);

  return ret;
}

bool GridRowHeader::read(int16_t row) {
  bool ret;
  int32_t offset = grid.get_header_offset(row);

  ret = proj.file.seekSet(offset);
  if (!ret) {
    DEBUG_PRINT_FN();
    DEBUG_PRINTLN("read row header fail; ");
    return false;
  }

  ret = mcl_sd.read_data((uint8_t *)(this), sizeof(GridRowHeader), &proj.file);

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
    track_type[x] = 0;
    model[x] = 0;
  }
}

