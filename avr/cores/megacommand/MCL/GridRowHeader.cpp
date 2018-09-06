#include "MCL.h"
#include "GridRowHeader.h"

void GridRowHeader::update_model(int16_t column, uint8_t model, uint8_t dev) {
  track_type[column] = model;
  device[column] = dev;
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


void GridRowHeader::init() {
  active = false;
  for (uint8_t x = 0; x < GRID_WIDTH; x++) {
    track_type[x] = 0;
    device[x] = 0;
  }
}

