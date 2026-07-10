/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#ifndef PROJECT_H__
#define PROJECT_H__

#include "Grid/Grid.h"
#include "MCLDefines.h"
#include "Grid/MidiDeviceGrid.h"
#include "MCLMemory.h"
#include "MCLSysConfig.h"
#include "GUI/Pages/Project/ProjectPages.h"

#define PROJ_MIN_READABLE_VERSION 3000
#define PROJ_VERSION 3013
#define PRJ_DIR "/Projects"

class ATTR_PACKED() ProjectHeader {
public:
  uint32_t version;
  uint8_t active_grid_pair;
  uint8_t reserved[15];
  uint32_t hash;
  MCLSysConfigData cfg;
};

static_assert(offsetof(ProjectHeader, cfg) == 24,
              "persisted project config offset changed");
static_assert(sizeof(ProjectHeader) == 209,
              "persisted project header layout changed");

class Project : public ProjectHeader {
public:
  File file;

  MidiDeviceGrid grids[NUM_GRIDS];

  void chdir_projects();
  bool project_loaded;
  void setup();
  bool new_project(const char *newprj);
  bool new_project_prompt(const char *parent = nullptr) NOINLINE();
  bool load_project(const char *projectname);
#ifdef MCL_HAS_PROJECT_BACKUP
  bool load_project_version(const char *projectname, uint8_t pair);
#endif
#ifdef MCL_HAS_PROJECT_CONVERSION
  bool convert_project(const char *projectname);
#endif
  bool check_project_version();
  bool new_project_master_file(const char *projectname);
  bool write_header();
  bool build_grid_filename(const char *basename, uint8_t suffix, char *out,
                           size_t out_len) const;
  // Expects SD to already be in the project directory.
  bool project_pair_exists(uint8_t pair, const char *basename);
  bool read_active_grid_pair(const char *projectname, uint8_t *pair);
  bool grid_pair_exists(const char *projectname, uint8_t pair);
#ifdef MCL_HAS_PROJECT_BACKUP
  bool create_backup(const char *projectname, uint8_t *created_pair = nullptr);
  bool delete_backup(const char *projectname, uint8_t pair);
#endif
  bool rename_project_files(const char *from_basename, const char *to_basename);
  bool copy_project(const char *from_project, const char *to_project);
#ifdef MCL_HAS_FILE_MOVE
  bool move_project(const char *from_project, const char *to_project);
#endif
  bool store_config_from_system();

  // Write data — col is logical 0–31, routed to physical grid/col
  bool write_grid(void *data, size_t len, GridSlot col, GridRow row) {
    last_grid_ = col >> 4;
    return grids[last_grid_].write(data, len, col & 0xF, row);
  }
  // Write without seek (uses grid from last seeked call)
  bool write_grid(void *data, size_t len) {
    return grids[last_grid_].write(data, len);
  }

  // Read data — col is logical 0–31, routed to physical grid/col
  bool read_grid(void *data, size_t len, GridSlot col, GridRow row) {
    last_grid_ = col >> 4;
    return grids[last_grid_].read(data, len, col & 0xF, row);
  }
  // Read without seek
  bool read_grid(void *data, size_t len) {
    return grids[last_grid_].read(data, len);
  }

  bool clear_row_grid(GridRow row, GridIndex grid) {
    return grids[grid].clear_row(row);
  }

  bool clear_slot_grid(GridSlot col, GridRow row) {
    GridIndex g = col >> 4;
    return grids[g].clear_slot(col & 0xF, row);
  }

  bool write_grid_row_header(GridRowHeader *row_header, GridRow row,
                             GridIndex grid) {
    return grids[grid].write_row_header(row_header, row);
  }

  bool read_grid_row_header(GridRowHeader *row_header, GridRow row,
                            GridIndex grid) {
    return grids[grid].read_row_header(row_header, row);
  }

  bool sync_grid(GridIndex grid) { return grids[grid].sync(); }
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
  void draw_upgrade_progress(GridIndex grid, GridRow row);
  bool read_header();
#ifndef __AVR__
  bool load_project_impl(const char *projectname, uint8_t requested_pair,
                         bool use_requested_pair,
                         bool allow_headerless_requested_pair = false);
#else
  bool load_project_impl(const char *projectname, uint8_t requested_pair,
                         bool use_requested_pair);
#endif
#if defined(MCL_HAS_PROJECT_BACKUP) && !defined(__AVR__)
  bool preflight_project_version(const char *projectname, uint8_t pair,
                                 bool *allow_headerless_pair);
#endif
#ifdef MCL_HAS_PROJECT_CONVERSION
  bool migrate_track_storage_versions(const char *basename,
                                      uint8_t *active_pair);
#endif
  bool copy_grid_pair(const char *from_project, const char *from_basename,
                      const char *to_project, const char *to_basename,
                      uint8_t source_pair, uint8_t dest_pair);
  bool split_project_path(const char *projectname, const char **basename) const;
  bool project_file_name(const char *basename, char *out, size_t out_len) const;
  uint8_t project_pair_file_mask(uint8_t pair, const char *basename);
#ifdef MCL_HAS_PROJECT_CONVERSION
  bool migrate_legacy_md_aux_slots(GridRow row, GridRowHeader *grid_x_header,
                                   Grid *dst_grids, bool *skip_grid_x0);
  bool migrate_grid_track_storage_versions(GridIndex grid, Grid *dst_grids);
#endif
  GridIndex last_grid_;
};

extern Project proj;

#endif /* PROJECT_H__ */
