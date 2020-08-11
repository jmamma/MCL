/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef PROJECT_H__
#define PROJECT_H__
#include "MCLMemory.h"
#include "MCLSysConfig.h"
#include "ProjectPages.h"
#define PROJ_VERSION 2025

class ProjectHeader {
public:
  uint32_t version;
  uint8_t reserved[16];
  uint32_t hash;
  MCLSysConfigData cfg;
};

class Project : public ProjectHeader {
public:
  File file;
  uint8_t grid_select;
  Grid grids[NUM_GRIDS];

  bool project_loaded = false;
  void setup();
  bool new_project();
  bool load_project(const char *projectname);
  bool check_project_version();
  bool new_project(const char *projectname);
  bool write_header();

  void uint8_t select_grid(uint8_t i) { grid_select = i; }
  // Write data to a specific grid
  bool write_grid(void *data, size_t len, uint8_t col, uint16_t row,
                  uint8_t grid) {
    if (grid == 255) {
      grid = grid_select;
    }
    bool ret = grids[grid].write(data, len, col, row);
    return ret;
  }
  // Write data to current grid;
  bool write_grid(void *data, size_t len, uint8_t col, uint16_t row) {
    write_grid(data, len, col, row, grid_select);
  }

  // Read data from a specific grid
  bool read_grid(void *data, size_t len, uint8_t col, uint16_t row,
                 uint8_t grid) {
    if (grid == 255) {
      grid = grid_select;
    }
    bool ret = grids[grid].read(data, len, col, row);
    return ret;
  }
  // Read data from current grid;
  bool read_grid(void *data, size_t len, uint8_t col, uint16_t row) {
    read_grid(data, len, col, row, grid_select);
  }

  bool sync_grid(uint8_t grid) { return grids[grid].sync(); }
  bool sync_grid() { return sink_grid(grid_select); }
};

extern Project proj;

#endif /* PROJECT_H__ */
