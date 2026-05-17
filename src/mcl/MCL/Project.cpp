#include "Project.h"
#include "MCLSd.h"
#include "MCLGUI.h"
#include "GridPages.h"
#include "oled.h"

#include "MDTrack.h"
#include "ExtTrack.h"
#include "A4Track.h"
#include "MNMTrack.h"
#include "MDFXTrack.h"
#include "MDLFOTrack.h"
#include "MDRouteTrack.h"
#include "EmptyTrack.h"

void Project::draw_wait_popup(const char *message) {
  mcl_gui.draw_infobox("PLEASE WAIT", message);
  oled_display.display();
}

void Project::draw_upgrade_progress(GridIndex grid, GridRow row) {
#ifdef OLED_DISPLAY
  uint8_t progress = grid * (GRID_LENGTH / NUM_GRIDS) + row / NUM_GRIDS;
  mcl_gui.draw_progress_bar(progress, GRID_LENGTH, false, 31, 21);
#endif
}

void Project::setup() {}

bool Project::new_project(const char *newprj) {
  // Create parent project directory
  //
  chdir_projects();
  DEBUG_PRINTLN(newprj);
  DEBUG_PRINTLN(strlen(newprj));
  // Create project directory
  if (!SD.mkdir(newprj, true) || !SD.chdir(newprj)) {
    gfx.alert("ERROR", "DIR");
    return false;
  }


  char proj_filename[PRJ_NAME_LEN  + 5] = {'\0'};
  strcpy(proj_filename, newprj);
  strcat(proj_filename, ".mcl");

  draw_wait_popup("CREATING PROJECT");

  DEBUG_PRINTLN(proj_filename);
  if (SD.exists(proj_filename)) {
    gfx.alert("ERROR", "PROJECT EXISTS");
    return false;
  }

  // Initialise Grid Files.
  //

  char grid_filename[PRJ_NAME_LEN  + 5] = {'\0'};
  strcpy(grid_filename, newprj);
  uint8_t l = strlen(grid_filename);


  for (uint8_t i = 0; i < NUM_GRIDS; i++) {
    grid_filename[l] = '.';
    grid_filename[l + 1] = i + '0';
    grid_filename[l + 2] = '\0';
    if (!SD.exists(grid_filename)) {
      if (!grids[i].new_grid(grid_filename)) {
        gfx.alert("ERROR", "SD ERROR");
        return false;
      }
    }
  }
  // Initialiase Project Master File.
  //
  bool ret = proj.new_project_master_file(proj_filename);
  return ret;
}

bool Project::new_project_prompt() {
  char newprj[PRJ_NAME_LEN];

  char my_string[PRJ_NAME_LEN] = "project___";

  my_string[7] = (mcl_cfg.number_projects % 1000) / 100 + '0';
  my_string[7 + 1] = (mcl_cfg.number_projects % 100) / 10 + '0';
  my_string[7 + 2] = (mcl_cfg.number_projects % 10) + '0';

  strncpy(newprj, my_string, PRJ_NAME_LEN);
again:
  if (mcl_gui.wait_for_input(newprj, "New Project:", PRJ_NAME_LEN)) {

    if (!new_project(newprj)) {
      goto again;
    }
    if (proj.load_project(newprj)) {
      grid_page.reload_slot_models = false;
      DEBUG_PRINTLN("project loaded, setting page to grid");
      mcl.setPage(GRID_PAGE);
      return true;
    } else {
      gfx.alert("ERROR", "SD ERROR");
      goto again;
    }
  }
  if (proj.project_loaded) {
    mcl.setPage(GRID_PAGE);
    return true;
  }
  return false;
}

void Project::chdir_projects() {
  char path[64];
  const char *c_project_root = mcl_sd.full_path(PRJ_DIR, path, sizeof(path));
  SD.mkdir(c_project_root, true);
  SD.chdir(c_project_root);
}

#define OLD_PROJ_VERSION 2025

bool Project::convert_project(const char *projectname) { return true; }

bool Project::load_project(const char *projectname) {

  bool ret;

  DEBUG_PRINT_FN();
  DEBUG_PRINTLN(F("Loading project"));
  DEBUG_PRINTLN(projectname);
  file.close();
  project_loaded = false;

  size_t path_len = strlen(projectname);
  if (path_len == 0 || path_len >= PRJ_PATH_LEN) {
    DEBUG_PRINTLN("bad path len");
    return false;
  }

  const char *project_basename = strrchr(projectname, '/');
  project_basename = project_basename == nullptr ? projectname
                                                 : project_basename + 1;
  size_t name_len = strlen(project_basename);
  if (name_len == 0 || name_len > PRJ_NAME_LEN) {
    DEBUG_PRINTLN("bad name len");
    return false;
  }

  char proj_filename[PRJ_NAME_LEN  + 5] = {'\0'};
  strncpy(proj_filename, project_basename, PRJ_NAME_LEN);
  strcat(proj_filename, ".mcl");

  char grid_name[PRJ_NAME_LEN  + 5] = {'\0'};
  strcpy(grid_name, project_basename);

  // Open project parent
  chdir_projects();

  // Open project directory.
  if (!SD.exists(projectname)) {
    DEBUG_PRINTLN("dir does not exist");
    return false;
  }

  if (!SD.chdir(projectname)) {
    DEBUG_PRINTLN("could not enter project dir");
    return false;
  }

  if (!SD.exists(proj_filename)) {
    DEBUG_DUMP(proj_filename);
    DEBUG_PRINTLN(F("does not exist"));
    return false;
  }

  ret = file.open(proj_filename, O_RDWR);
  if (!ret) {
    file.close();
    DEBUG_PRINTLN(F("Could not open project file"));
    return false;
  }
  ret = check_project_version();

  if (!ret) {
    DEBUG_PRINTLN(F("Project version incompatible"));
    file.close();
    return false;
  }

  bool migrate_track_storage = version < PROJ_VERSION_TRACK_STORAGE_VERSION;
  bool migrate_route_tracks = version < PROJ_VERSION_ROUTE_TRACK_TYPE;
  if (migrate_track_storage || migrate_route_tracks) {
    draw_wait_popup("UPGRADING PROJECT");
  }

  for (uint8_t i = 0; i < NUM_GRIDS; i++) {
    grids[i].close_file();

    grid_name[name_len] = '.';
    grid_name[name_len + 1] = i + '0';
    grid_name[name_len + 2] = '\0';
    DEBUG_PRINTLN(F("opening grid"));
    DEBUG_PRINTLN(grid_name);
    if (!grids[i].open_file(grid_name)) {
      DEBUG_PRINTLN(F("could not open grid"));
      gfx.alert("ERROR", "OPEN GRID");
      return false;
    }
  }

  if ((migrate_track_storage || migrate_route_tracks) &&
      !migrate_track_storage_versions(migrate_track_storage,
                                      migrate_route_tracks)) {
    DEBUG_PRINTLN(F("Could not migrate project tracks"));
    return false;
  }
  if ((migrate_track_storage || migrate_route_tracks) && !write_header()) {
    return false;
  }

  strncpy(mcl_cfg.project, projectname, sizeof(mcl_cfg.project) - 1);
  mcl_cfg.project[sizeof(mcl_cfg.project) - 1] = '\0';
  if (!mcl_cfg.number_projects) { mcl_cfg.number_projects++; }

  ret = mcl_cfg.write_cfg();

  if (!ret) {
    DEBUG_PRINTLN(F("could not write cfg"));
    return false;
  }
  grid_page.row_scan = GRID_LENGTH;
  project_loaded = true;
  return true;
}

bool Project::check_project_version(uint16_t min_version) {
  bool ret;

  DEBUG_PRINT_FN();
  DEBUG_PRINTLN(F("Check project version"));

  ret = file.seekSet(0);

  if (!ret) {
    DEBUG_PRINTLN(F("Seek failed"));
    return false;
  }
  ret = mcl_sd.read_data((uint8_t *)this, sizeof(ProjectHeader), &file);

  if (!ret) {
    DEBUG_PRINTLN(F("Could not read project header"));
    return false;
  }
  return version >= min_version;
}

bool Project::migrate_legacy_md_aux_slots(GridRow row,
                                          GridRowHeader *grid_x_header,
                                          bool *converted_track0_lfo,
                                          bool migrate_legacy_aux_layout,
                                          bool migrate_route_tracks) {
  *converted_track0_lfo = false;
  if (grid_x_header == nullptr) {
    return false;
  }

  GridRowHeader grid_y_header;
  if (!grids[1].read_row_header(&grid_y_header, row)) {
    return false;
  }

  EmptyTrack scratch;
  GridTrack empty_slot;
  empty_slot.link.init(row);
  bool clear_lfo_slot = false;
  bool clear_legacy_perf_slot = false;
  bool perf_moved_to_lfo_slot = false;

  if (migrate_legacy_aux_layout &&
      grid_y_header.track_type[MDLFO_TRACK_NUM] == MDLFO_TRACK_TYPE) {
    auto *legacy_track =
        scratch.load_from_grid_512(MDLFO_TRACK_NUM, row, &grids[1]);
    if (legacy_track == nullptr) {
      return false;
    }

    if (legacy_track->active == MDLFO_TRACK_TYPE &&
        (grid_x_header->track_type[0] == MD_TRACK_TYPE ||
         grid_x_header->track_type[0] == EMPTY_TRACK_TYPE)) {
      SeqLFOData legacy_lfo;
      static_cast<MDLFOTrack *>(legacy_track)->lfo_data.store_data(&legacy_lfo);

      auto *track0 = scratch.load_from_grid_512(0, row, &grids[0]);
      if (track0 == nullptr) {
        return false;
      }

      MDTrack *md_track = nullptr;
      if (track0->active == MD_TRACK_TYPE) {
        md_track = static_cast<MDTrack *>(track0);
      } else if (track0->active == EMPTY_TRACK_TYPE) {
        md_track = static_cast<MDTrack *>(track0->init_track_type(MD_TRACK_TYPE));
        md_track->init();
        md_track->link.init(row);
        md_track->machine.track = 0;
        md_track->machine.lfo.init(0);
      }

      if (md_track != nullptr) {
        md_track->mod_data.init();
        LFOSeqTrack::convert_legacy_data(legacy_lfo, &md_track->mod_data.lfo);

        md_track->version[0] = SEQ_TRACK_MOD_STORAGE_VERSION;
        md_track->version[1] = 0;
        if (!grids[0].write(md_track->_this(), md_track->_sizeof(), 0, row)) {
          return false;
        }
        if (grid_x_header->track_type[0] == EMPTY_TRACK_TYPE) {
          grid_x_header->active = true;
          grid_x_header->update_model(0, md_track->get_model(), MD_TRACK_TYPE);
          if (!grids[0].write_row_header(grid_x_header, row)) {
            return false;
          }
        }
        *converted_track0_lfo = true;
      }
    }

    grid_y_header.update_model(MDLFO_TRACK_NUM, 0, EMPTY_TRACK_TYPE);
    clear_lfo_slot = true;
  }

  if (migrate_legacy_aux_layout &&
      grid_y_header.track_type[LEGACY_PERF_TRACK_NUM] == PERF_TRACK_TYPE) {
    auto *perf_track =
        scratch.load_from_grid_512(LEGACY_PERF_TRACK_NUM, row, &grids[1]);
    if (perf_track == nullptr) {
      return false;
    }

    if (perf_track->active == PERF_TRACK_TYPE) {
      if (!perf_track->store_in_grid(PERF_TRACK_NUM, row, nullptr, 0, false,
                                     &grids[1])) {
        return false;
      }
      grid_y_header.update_model(PERF_TRACK_NUM, PERF_TRACK_TYPE,
                                 PERF_TRACK_TYPE);
      perf_moved_to_lfo_slot = true;
    }
    grid_y_header.update_model(LEGACY_PERF_TRACK_NUM, 0, EMPTY_TRACK_TYPE);
    clear_legacy_perf_slot = true;
  }

  if (clear_lfo_slot && !perf_moved_to_lfo_slot &&
      grid_y_header.track_type[MDLFO_TRACK_NUM] == EMPTY_TRACK_TYPE &&
      !grids[1].write(empty_slot._this(), empty_slot._sizeof(),
                      MDLFO_TRACK_NUM, row)) {
    return false;
  }

  if (clear_legacy_perf_slot &&
      grid_y_header.track_type[LEGACY_PERF_TRACK_NUM] == EMPTY_TRACK_TYPE &&
      !grids[1].write(empty_slot._this(), empty_slot._sizeof(),
                      LEGACY_PERF_TRACK_NUM, row)) {
    return false;
  }

  if (migrate_route_tracks &&
      grid_y_header.track_type[MDROUTE_TRACK_NUM] == MDROUTE_TRACK_TYPE) {
    auto *route_track =
        scratch.load_from_grid_512(MDROUTE_TRACK_NUM, row, &grids[1]);
    if (route_track == nullptr) {
      return false;
    }

    if (route_track->active == MDROUTE_TRACK_TYPE) {
      auto *legacy_route = static_cast<LegacyMDRouteTrack *>(route_track);
      GridLink link = legacy_route->link;
      uint8_t routing[16];
      memcpy(routing, legacy_route->routing, sizeof(routing));
      uint16_t poly_mask = legacy_route->poly_mask;

      auto *new_route = static_cast<MDRouteTrack *>(
          route_track->init_track_type(MD_ROUTE_TRACK_TYPE));
      new_route->link = link;
      memcpy(new_route->routing, routing, sizeof(routing));
      new_route->load_legacy_poly_mask(poly_mask, cfg.uart2_poly_chan);
      new_route->version[0] = 0;
      new_route->version[1] = 0;

      if (!grids[1].write(new_route->_this(), new_route->_sizeof(),
                          MDROUTE_TRACK_NUM, row)) {
        return false;
      }
      grid_y_header.update_model(MDROUTE_TRACK_NUM, new_route->get_model(),
                                 MD_ROUTE_TRACK_TYPE);
    }
  }

  if (!grids[1].write_row_header(&grid_y_header, row)) {
    return false;
  }
  return true;
}

bool Project::migrate_track_storage_versions(bool stamp_track_versions,
                                             bool migrate_route_tracks) {
  GridIndex grid_count = stamp_track_versions ? NUM_GRIDS : 1;
  for (GridIndex grid = 0; grid < grid_count; grid++) {
    if (!migrate_grid_track_storage_versions(grid, stamp_track_versions,
                                             migrate_route_tracks)) {
      return false;
    }
  }
  if (migrate_route_tracks && !grids[1].sync()) {
    return false;
  }
  return true;
}

bool Project::migrate_grid_track_storage_versions(GridIndex grid,
                                                  bool stamp_track_versions,
                                                  bool migrate_route_tracks) {
  uint16_t legacy_version = 0;
  for (GridRow row = 0; row < GRID_LENGTH; row++) {
    draw_upgrade_progress(grid, row);

    GridRowHeader row_header;
    if (!grids[grid].read_row_header(&row_header, row)) {
      return false;
    }

    bool converted_track0_lfo = false;
    if (grid == 0 &&
        !migrate_legacy_md_aux_slots(row, &row_header, &converted_track0_lfo,
                                     stamp_track_versions,
                                     migrate_route_tracks)) {
      return false;
    }

    if (!stamp_track_versions) {
      continue;
    }

    for (GridColumn column = 0; column < GRID_WIDTH; column++) {
      if (row_header.track_type[column] == EMPTY_TRACK_TYPE) {
        continue;
      }
      if (grid == 0 && column == 0 && converted_track0_lfo) {
        continue;
      }
      if (!grids[grid].seek(column, row)) {
        return false;
      }
      if (!grids[grid].write((uint8_t *)&legacy_version,
                             sizeof(legacy_version))) {
        return false;
      }
    }
  }

  draw_upgrade_progress(grid, GRID_LENGTH);
  return grids[grid].sync();
}

bool Project::write_header() {

  bool ret;

  DEBUG_PRINT_FN();
  DEBUG_PRINTLN(F("Writing project header"));

  version = PROJ_VERSION;
  //  Config mcl_cfg.
  //  uint8_t reserved[16];
  hash = 0;

  ret = file.seekSet(0);

  if (!ret) {

    DEBUG_PRINTLN(F("Seek failed"));

    return false;
  }

  ret = mcl_sd.write_data((uint8_t *)this, sizeof(ProjectHeader), &file);

  if (!ret) {
    DEBUG_PRINTLN(F("Write header failed"));
    return false;
  }
  DEBUG_PRINTLN(F("Write header success"));
  return true;
}

bool Project::new_project_master_file(const char *projectname) {

  bool ret;

  DEBUG_PRINT_FN();
  DEBUG_PRINTLN(F("Creating new project master file"));

  file.close();

  DEBUG_PRINTLN(F("Attempting to extend project file"));
  DEBUG_PRINTLN(projectname);

  ret = file.open(projectname, O_RDWR | O_CREAT);
  if (!ret) {
    DEBUG_PRINTLN(F("Could not open file"));
    return false;
  }

  ret = file.preAllocate(GRID_SLOT_BYTES);
  if (!ret) {
    file.close();
    DEBUG_PRINTLN(F("Could not extend file"));
    return false;
  }
  DEBUG_PRINTLN(F("extension succeeded, trying to close"));
  file.close();

  ret = file.open(projectname, O_RDWR);

  if (!ret) {
    file.close();

    DEBUG_PRINTLN(F("Could not open file"));
    return false;
  }

  ret = file.seekSet(0);

  if (!ret) {
    DEBUG_PRINTLN(F("Could not seek"));
    return false;
  }

  if (!write_header()) {
    return false;
  }

  // m_strncpy(mcl_cfg.project, projectname, 16);
  file.close();

  mcl_cfg.number_projects++;
  mcl_cfg.write_cfg();

  DEBUG_PRINTLN(projectname);
  DEBUG_PRINTLN(F("project created"));
  // if (!ret) {
  // return false;
  // }

  return true;
}
Project proj;
