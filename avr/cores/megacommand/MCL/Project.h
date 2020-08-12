/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef PROJECT_H__
#define PROJECT_H__

#include "Grid.h"
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
                  uint8_t grid = 255) {
    if (grid == 255) {
      grid = grid_select;
    }
    bool ret = grids[grid].write(data, len, col, row);
    return ret;
  }
  // Write without seek.
  bool write_grid(void *data, size_t len,
                 uint8_t grid = 255) {
    if (grid == 255) {
      grid = grid_select;
    }
    bool ret = grids[grid].write(data, len);
    return ret;
  }

  // Read data from a specific grid
  bool read_grid(void *data, size_t len, uint8_t col, uint16_t row,
                 uint8_t grid = 255) {
    if (grid == 255) {
      grid = grid_select;
    }
    bool ret = grids[grid].read(data, len, col, row);
    return ret;
  }
  // Read without seek.
  bool read_grid(void *data, size_t len,
                 uint8_t grid = 255) {
    if (grid == 255) {
      grid = grid_select;
    }
    bool ret = grids[grid].read(data, len);
    return ret;
  }


  bool write_grid_row_header(GridRowHeader *row_header, uint16_t row,
                             uint8_t grid_select = 255) {
    if (grid == 255) {
      grid = grid_select;
    }
    bool ret = grids[grid].write_row_header(row_header, row);
    return ret;
  }

  bool read_grid_row_header(GridRowHeader *row_header, uint16_t row,
                            uint8_t grid_select = 255) {
    if (grid == 255) {
      grid = grid_select;
    }
    bool ret = grids[grid].read_row_header(row_header, row);
    return ret;
  }

  bool sync_grid(uint8_t grid) { return grids[grid].sync(); }
  bool sync_grid() { return sync_grid(grid_select); }
};

extern Project proj;

#endif /* PROJECT_H__ */
