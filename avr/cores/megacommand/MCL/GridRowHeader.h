/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef GRIDROW_H__
#define GRIDROW_H__

#include "MCLMemory.h"
#include "Project.h"

class GridRowHeader {
 public:
  bool active;
  char name[17];
  uint8_t track_type[GRID_WIDTH];
  uint8_t model[GRID_WIDTH];

  void update_model(int16_t column, uint8_t model_, uint8_t track_type_);
  bool is_empty();
  void init();
  bool read(uint16_t row, uint8_t grid = 255) {
   return proj.read_grid_row_header(this, row, grid);
  }
  bool write(uint16_t row, uint8_t grid = 255) {
   return proj.write_grid_row_header(this, row, grid);
  }

};

#endif /* GRIDROW_H__ */
