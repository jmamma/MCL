/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef PROJECT_H__
#define PROJECT_H__

#include "Grid.h"
#include "MidiDeviceGrid.h"
#include "MCLMemory.h"
#include "MCLSysConfig.h"
#include "ProjectPages.h"

#define PROJ_MIN_READABLE_VERSION 3000
#define PROJ_VERSION_TRACK_STORAGE_VERSION 3001
#define PROJ_VERSION 3002
#define PRJ_DIR "/Projects"

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

  MidiDeviceGrid grids[NUM_GRIDS];

  void chdir_projects();
  bool project_loaded = false;
  void setup();
  bool new_project(const char *newprj);
  bool new_project_prompt();
  bool load_project(const char *projectname);
  bool convert_project(const char *projectname);
  bool check_project_version(uint16_t min_version = PROJ_MIN_READABLE_VERSION);
  bool migrate_grid_track_storage_versions(uint8_t grid);
  bool new_project_master_file(const char *projectname);
  bool write_header();

  // Write data — col is logical 0–31, routed to physical grid/col
  bool write_grid(void *data, size_t len, uint8_t col, uint16_t row) {
    last_grid_ = col >> 4;
    return grids[last_grid_].write(data, len, col & 0xF, row);
  }
  // Write without seek (uses grid from last seeked call)
  bool write_grid(void *data, size_t len) {
    return grids[last_grid_].write(data, len);
  }

  // Read data — col is logical 0–31, routed to physical grid/col
  bool read_grid(void *data, size_t len, uint8_t col, uint16_t row) {
    last_grid_ = col >> 4;
    return grids[last_grid_].read(data, len, col & 0xF, row);
  }
  // Read without seek
  bool read_grid(void *data, size_t len) {
    return grids[last_grid_].read(data, len);
  }

  bool clear_row_grid(uint16_t row, uint8_t grid) {
    return grids[grid].clear_row(row);
  }

  bool clear_slot_grid(uint8_t col, uint16_t row) {
    uint8_t g = col >> 4;
    return grids[g].clear_slot(col & 0xF, row);
  }

  bool write_grid_row_header(GridRowHeader *row_header, uint16_t row,
                             uint8_t grid) {
    return grids[grid].write_row_header(row_header, row);
  }

  bool read_grid_row_header(GridRowHeader *row_header, uint16_t row,
                            uint8_t grid) {
    DEBUG_PRINT_FN();
    DEBUG_DUMP(grid);
    return grids[grid].read_row_header(row_header, row);
  }

  bool sync_grid(uint8_t grid) { return grids[grid].sync(); }
  bool sync_grid() { sync_grid(0); return sync_grid(1); }
  bool close_project() {
    bool ret = true;
    // Close main file
    if (!file.close()) {
        ret = false;
    }
    // Close all grid files
    for (uint8_t i = 0; i < NUM_GRIDS; i++) {
        if (!grids[i].close_file()) {
            ret = false;
        }
    }
    return ret;
  }

private:
  void draw_wait_popup(const char *message);
  uint8_t last_grid_ = 0;
};

extern Project proj;

#endif /* PROJECT_H__ */
