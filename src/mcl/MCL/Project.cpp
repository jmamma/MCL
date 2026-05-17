#include "Project.h"
#include "MCLSd.h"
#include "MCLGUI.h"
#include "GridPages.h"
#include "MidiSetup.h"
#include "oled.h"

#include "MDTrack.h"
#include "ExtTrack.h"
#include "A4Track.h"
#include "MNMTrack.h"
#include "MDFXTrack.h"
#include "MDLFOTrack.h"
#include "MDRouteTrack.h"
#include "EmptyTrack.h"
#include <stddef.h>

namespace {

char *write_u8(char *out, uint8_t value) {
  if (value >= 100) {
    *out++ = '0' + value / 100;
    value %= 100;
    *out++ = '0' + value / 10;
    value %= 10;
  } else if (value >= 10) {
    *out++ = '0' + value / 10;
    value %= 10;
  }
  *out++ = '0' + value;
  *out = '\0';
  return out;
}

bool join_project_file(char *out, size_t out_len, const char *project,
                       const char *filename) {
  if (out_len == 0) {
    return false;
  }
  out[0] = '\0';

  size_t project_len = strlen(project);
  size_t filename_len = strlen(filename);
  bool add_sep = project_len > 0 && project[project_len - 1] != '/';
  size_t needed = project_len + (add_sep ? 1 : 0) + filename_len + 1;
  if (needed > out_len) {
    return false;
  }

  strcpy(out, project);
  if (add_sep) {
    strcat(out, "/");
  }
  strcat(out, filename);
  return true;
}

bool copy_grid_slot_raw(Grid &src_grid, Grid &dst_grid, GridColumn col,
                        GridRow row) {
  uint8_t buf[256];
  if (!src_grid.seek(col, row) || !dst_grid.seek(col, row)) {
    return false;
  }

  uint16_t remaining = GRID_SLOT_BYTES;
  while (remaining > 0) {
    uint16_t n = remaining > sizeof(buf) ? sizeof(buf) : remaining;
    if (!src_grid.read(buf, n) || !dst_grid.write(buf, n)) {
      return false;
    }
    remaining -= n;
  }
  return true;
}

constexpr uint16_t PROJECT_RENAME_SUFFIXES =
    256;

class LegacyProjectHeader {
public:
  uint32_t version;
  uint8_t active_grid_pair;
  uint8_t reserved[15];
  uint32_t hash;
  MCLSysConfigData cfg;
};

constexpr size_t PROJECT_CONFIG_OFFSET =
    offsetof(MCLSysConfigData, uart1_turbo_speed);
constexpr size_t PROJECT_CONFIG_SIZE =
    offsetof(MCLSysConfigData, project_config) - PROJECT_CONFIG_OFFSET;

bool project_config_valid(const MCLSysConfigData &source) {
  return source.version == CONFIG_VERSION;
}

void copy_project_config(MCLSysConfigData *dst,
                         const MCLSysConfigData &source) {
  dst->version = CONFIG_VERSION;
  dst->project[0] = '\0';
  dst->number_projects = 0;
  memcpy((uint8_t *)dst + PROJECT_CONFIG_OFFSET,
         (const uint8_t *)&source + PROJECT_CONFIG_OFFSET,
         PROJECT_CONFIG_SIZE);
  dst->project_config = 0;
}

void apply_project_config(MCLSysConfigData *dst,
                          const MCLSysConfigData &source) {
  if (!project_config_valid(source)) {
    return;
  }
  memcpy((uint8_t *)dst + PROJECT_CONFIG_OFFSET,
         (const uint8_t *)&source + PROJECT_CONFIG_OFFSET,
         PROJECT_CONFIG_SIZE);
}

} // namespace

void Project::draw_wait_popup(const char *message) {
  mcl_gui.draw_infobox("PLEASE WAIT", message);
  oled_display.display();
}

void Project::draw_upgrade_progress(GridIndex grid, GridRow row) {
#ifdef OLED_DISPLAY
  uint8_t progress = grid * (GRID_LENGTH / NUM_GRIDS) + row / NUM_GRIDS;
  mcl_gui.draw_progress_bar(progress, GRID_LENGTH, false, MCLGUI::s_progress_x,
                            21);
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
  if (!project_file_name(newprj, proj_filename, sizeof(proj_filename))) {
    gfx.alert("ERROR", "BAD NAME");
    return false;
  }

  draw_wait_popup("CREATING PROJECT");

  DEBUG_PRINTLN(proj_filename);
  if (SD.exists(proj_filename)) {
    gfx.alert("ERROR", "PROJECT EXISTS");
    return false;
  }

  // Initialise Grid Files.
  //

  for (uint8_t i = 0; i < NUM_GRIDS; i++) {
    char grid_filename[PRJ_NAME_LEN  + 5] = {'\0'};
    if (!build_grid_filename(newprj, i, grid_filename,
                             sizeof(grid_filename))) {
      gfx.alert("ERROR", "BAD NAME");
      return false;
    }
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

bool Project::split_project_path(const char *projectname,
                                 const char **basename) const {
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
  if (basename != nullptr) {
    *basename = project_basename;
  }
  return true;
}

bool Project::project_file_name(const char *basename, char *out,
                                size_t out_len) const {
  size_t name_len = strlen(basename);
  if (name_len == 0 || name_len > PRJ_NAME_LEN || name_len + 5 > out_len) {
    return false;
  }
  strcpy(out, basename);
  strcat(out, ".mcl");
  return true;
}

bool Project::build_grid_filename(const char *basename, uint8_t suffix,
                                  char *out, size_t out_len) const {
  size_t name_len = strlen(basename);
  if (name_len == 0 || name_len > PRJ_NAME_LEN || name_len + 5 > out_len) {
    return false;
  }
  memcpy(out, basename, name_len);
  out[name_len] = '.';
  write_u8(out + name_len + 1, suffix);
  return true;
}

bool Project::project_pair_exists(uint8_t pair, const char *basename) {
  for (uint8_t i = 0; i < NUM_GRIDS; i++) {
    char grid_name[PRJ_NAME_LEN + 5] = {'\0'};
    if (!build_grid_filename(basename, pair * NUM_GRIDS + i, grid_name,
                             sizeof(grid_name)) ||
        !SD.exists(grid_name)) {
      return false;
    }
  }
  return true;
}

bool Project::grid_pair_exists(const char *projectname, uint8_t pair) {
  const char *basename = nullptr;
  if (!split_project_path(projectname, &basename)) {
    return false;
  }
  chdir_projects();
  if (!SD.chdir(projectname)) {
    return false;
  }
  return project_pair_exists(pair, basename);
}

bool Project::read_active_grid_pair(const char *projectname, uint8_t *pair) {
  const char *basename = nullptr;
  if (pair == nullptr || !split_project_path(projectname, &basename)) {
    return false;
  }
  if (project_loaded && strcmp(projectname, mcl_cfg.project) == 0) {
    *pair = active_grid_pair;
    return true;
  }

  char proj_filename[PRJ_NAME_LEN + 5] = {'\0'};
  if (!project_file_name(basename, proj_filename, sizeof(proj_filename))) {
    return false;
  }

  File header_file;
  chdir_projects();
  if (!SD.chdir(projectname) || !header_file.open(proj_filename, O_READ)) {
    header_file.close();
    return false;
  }

  uint32_t header_version = 0;
  bool ok = header_file.seekSet(0) &&
            mcl_sd.read_data((uint8_t *)&header_version,
                             sizeof(header_version), &header_file);
  if (ok && header_version >= PROJ_VERSION_GRID_PAIRS) {
    ok = mcl_sd.read_data(pair, sizeof(*pair), &header_file);
  } else {
    *pair = 0;
  }
  header_file.close();
  if (!ok || header_version < PROJ_MIN_READABLE_VERSION) {
    return false;
  }
  return true;
}

bool Project::load_project(const char *projectname) {
  return load_project_impl(projectname, 0, false);
}

bool Project::load_project_version(const char *projectname, uint8_t pair) {
  return load_project_impl(projectname, pair, true);
}

bool Project::load_project_impl(const char *projectname, uint8_t requested_pair,
                                bool use_requested_pair) {

  bool ret;

  DEBUG_PRINT_FN();
  DEBUG_PRINTLN(F("Loading project"));
  DEBUG_PRINTLN(projectname);
  file.close();
  project_loaded = false;

  const char *project_basename = strrchr(projectname, '/');
  if (!split_project_path(projectname, &project_basename)) {
    return false;
  }

  char proj_filename[PRJ_NAME_LEN  + 5] = {'\0'};
  if (!project_file_name(project_basename, proj_filename,
                         sizeof(proj_filename))) {
    DEBUG_PRINTLN("bad project filename");
    return false;
  }

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
  bool migrate_grid_pairs = version < PROJ_VERSION_GRID_PAIRS;
  bool migrate_project_config = version < PROJ_VERSION_PROJECT_CONFIG;
  if (migrate_grid_pairs) {
    active_grid_pair = 0;
  }
  uint8_t pair = use_requested_pair ? requested_pair : active_grid_pair;
  if (migrate_track_storage || migrate_route_tracks || migrate_grid_pairs ||
      migrate_project_config) {
    draw_wait_popup("UPGRADING PROJECT");
  }

  if (pair >= 128 || !project_pair_exists(pair, project_basename)) {
    if (use_requested_pair) {
      DEBUG_PRINTLN(F("requested grid pair missing"));
      return false;
    }
    pair = 0;
    active_grid_pair = 0;
    migrate_grid_pairs = true;
    if (!project_pair_exists(pair, project_basename)) {
      DEBUG_PRINTLN(F("default grid pair missing"));
      return false;
    }
  }

  for (uint8_t i = 0; i < NUM_GRIDS; i++) {
    char grid_name[PRJ_NAME_LEN  + 5] = {'\0'};
    grids[i].close_file();

    if (!build_grid_filename(project_basename, pair * NUM_GRIDS + i,
                             grid_name, sizeof(grid_name))) {
      DEBUG_PRINTLN(F("bad grid filename"));
      return false;
    }
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
  if (use_requested_pair) {
    active_grid_pair = requested_pair;
  }
  bool applied_project_config = false;
  if (mcl_cfg.project_config) {
    apply_project_config(&mcl_cfg, cfg);
    ptc_groups.load(mcl_cfg.ptc_group);
    mclsys_normalize_midi_config();
    copy_project_config(&cfg, mcl_cfg);
    applied_project_config = true;
  }
  bool write_project_header = migrate_track_storage || migrate_route_tracks ||
                              migrate_grid_pairs || migrate_project_config ||
                              use_requested_pair;
  if (write_project_header && !write_header()) {
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
  if (applied_project_config) {
    midi_setup.cfg_ports();
  }
  grid_page.row_scan = GRID_LENGTH;
  project_loaded = true;
  return true;
}

bool Project::read_header() {
  if (!file.seekSet(0)) {
    DEBUG_PRINTLN(F("Seek failed"));
    return false;
  }

  uint32_t header_version = 0;
  if (!mcl_sd.read_data((uint8_t *)&header_version, sizeof(header_version),
                        &file)) {
    DEBUG_PRINTLN(F("Could not read project version"));
    return false;
  }

  if (header_version < PROJ_VERSION_PROJECT_CONFIG) {
    LegacyProjectHeader legacy_header;
    legacy_header.version = header_version;
    if (!mcl_sd.read_data((uint8_t *)&legacy_header + sizeof(header_version),
                          sizeof(legacy_header) - sizeof(header_version),
                          &file)) {
      DEBUG_PRINTLN(F("Could not read legacy project header"));
      return false;
    }

    version = legacy_header.version;
    active_grid_pair = version >= PROJ_VERSION_GRID_PAIRS
                           ? legacy_header.active_grid_pair
                           : 0;
    memset(reserved, 0, sizeof(reserved));
    hash = legacy_header.hash;
    if (project_config_valid(legacy_header.cfg)) {
      copy_project_config(&cfg, legacy_header.cfg);
    } else {
      copy_project_config(&cfg, mcl_cfg);
    }
    return true;
  }

  version = header_version;
  if (!mcl_sd.read_data((uint8_t *)(ProjectHeader *)this +
                            sizeof(header_version),
                        sizeof(ProjectHeader) - sizeof(header_version),
                        &file)) {
    DEBUG_PRINTLN(F("Could not read project header"));
    return false;
  }
  if (!project_config_valid(cfg)) {
    copy_project_config(&cfg, mcl_cfg);
  }
  return true;
}

bool Project::check_project_version(uint16_t min_version) {
  bool ret;

  DEBUG_PRINT_FN();
  DEBUG_PRINTLN(F("Check project version"));

  ret = read_header();

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

bool Project::store_config_from_system() {
  if (!project_loaded || !mcl_cfg.project_config) {
    return true;
  }
  copy_project_config(&cfg, mcl_cfg);
  return write_header();
}

bool Project::copy_grid_pair(const char *from_project,
                             const char *from_basename,
                             const char *to_project,
                             const char *to_basename,
                             uint8_t source_pair, uint8_t dest_pair) {
  chdir_projects();
  bool ok = true;
  for (uint8_t grid_idx = 0; ok && grid_idx < NUM_GRIDS; grid_idx++) {
    char src_name[PRJ_NAME_LEN + 5] = {'\0'};
    char dst_name[PRJ_NAME_LEN + 5] = {'\0'};
    char src_path[PRJ_PATH_LEN + PRJ_NAME_LEN + 6] = {'\0'};
    char dst_path[PRJ_PATH_LEN + PRJ_NAME_LEN + 6] = {'\0'};
    Grid src_grid;
    Grid dst_grid;

    ok = build_grid_filename(from_basename, source_pair * NUM_GRIDS + grid_idx,
                             src_name, sizeof(src_name)) &&
         build_grid_filename(to_basename, dest_pair * NUM_GRIDS + grid_idx,
                             dst_name, sizeof(dst_name)) &&
         join_project_file(src_path, sizeof(src_path), from_project,
                           src_name) &&
         join_project_file(dst_path, sizeof(dst_path), to_project, dst_name) &&
         !SD.exists(dst_path) && src_grid.open_file(src_path) &&
         dst_grid.new_file(dst_path);
    if (!ok) {
      src_grid.close_file();
      dst_grid.close_file();
      break;
    }

    ok = dst_grid.write_header();

    for (GridRow row = 0; ok && row < GRID_LENGTH; row++) {
      draw_upgrade_progress(grid_idx, row);

      GridRowHeader row_header;
      ok = src_grid.read_row_header(&row_header, row) &&
           dst_grid.write_row_header(&row_header, row);
      if (!ok) {
        break;
      }

      for (GridColumn col = 0; ok && col < GRID_WIDTH; col++) {
        ok = copy_grid_slot_raw(src_grid, dst_grid, col, row);
      }
    }

    if (ok) {
      draw_upgrade_progress(grid_idx, GRID_LENGTH);
      ok = dst_grid.sync();
    }

    src_grid.close_file();
    dst_grid.close_file();
  }

  if (!ok) {
    for (uint8_t i = 0; i < NUM_GRIDS; i++) {
      char dst_name[PRJ_NAME_LEN + 5] = {'\0'};
      char dst_path[PRJ_PATH_LEN + PRJ_NAME_LEN + 6] = {'\0'};
      if (build_grid_filename(to_basename, dest_pair * NUM_GRIDS + i,
                              dst_name, sizeof(dst_name)) &&
          join_project_file(dst_path, sizeof(dst_path), to_project,
                            dst_name)) {
        SD.remove(dst_path);
      }
    }
  }
  return ok;
}

bool Project::create_backup(const char *projectname) {
  const char *basename = nullptr;
  if (!split_project_path(projectname, &basename)) {
    return false;
  }

  uint8_t source_pair = 0;
  if (project_loaded && strcmp(projectname, mcl_cfg.project) == 0) {
    if (!sync_grid() || !store_config_from_system()) {
      return false;
    }
    source_pair = active_grid_pair;
  } else if (!read_active_grid_pair(projectname, &source_pair)) {
    return false;
  }

  chdir_projects();
  if (!SD.chdir(projectname) || !project_pair_exists(source_pair, basename)) {
    return false;
  }

  uint8_t dest_pair = 0;
  bool found = false;
  for (uint8_t pair = 1; pair < 128; pair++) {
    bool any_file = false;
    for (uint8_t i = 0; i < NUM_GRIDS; i++) {
      char grid_name[PRJ_NAME_LEN + 5] = {'\0'};
      if (!build_grid_filename(basename, pair * NUM_GRIDS + i, grid_name,
                               sizeof(grid_name))) {
        return false;
      }
      any_file |= SD.exists(grid_name);
    }
    if (!any_file) {
      dest_pair = pair;
      found = true;
      break;
    }
  }
  if (!found) {
    return false;
  }

  draw_wait_popup("CREATING BACKUP");
  return copy_grid_pair(projectname, basename, projectname, basename,
                        source_pair, dest_pair);
}

bool Project::delete_backup(const char *projectname, uint8_t pair) {
  if (pair == 0) {
    return false;
  }

  const char *basename = nullptr;
  if (!split_project_path(projectname, &basename)) {
    return false;
  }

  uint8_t active_pair = 0;
  if (!read_active_grid_pair(projectname, &active_pair) || pair == active_pair) {
    return false;
  }

  chdir_projects();
  if (!SD.chdir(projectname) || !project_pair_exists(pair, basename)) {
    return false;
  }

  bool ok = true;
  for (uint8_t i = 0; i < NUM_GRIDS; i++) {
    char grid_name[PRJ_NAME_LEN + 5] = {'\0'};
    if (!build_grid_filename(basename, pair * NUM_GRIDS + i, grid_name,
                             sizeof(grid_name)) ||
        !SD.remove(grid_name)) {
      ok = false;
    }
  }
  return ok;
}

bool Project::rename_project_files(const char *from_basename,
                                   const char *to_basename) {
  char from_name[PRJ_NAME_LEN + 5] = {'\0'};
  char to_name[PRJ_NAME_LEN + 5] = {'\0'};

  for (uint16_t suffix = 0; suffix < PROJECT_RENAME_SUFFIXES; suffix++) {
    if (!build_grid_filename(from_basename, suffix, from_name,
                             sizeof(from_name)) ||
        !build_grid_filename(to_basename, suffix, to_name, sizeof(to_name))) {
      return false;
    }
    if (SD.exists(from_name) && !SD.rename(from_name, to_name)) {
      return false;
    }
  }

  if (!project_file_name(from_basename, from_name, sizeof(from_name)) ||
      !project_file_name(to_basename, to_name, sizeof(to_name))) {
    return false;
  }
  return !SD.exists(from_name) || SD.rename(from_name, to_name);
}

bool Project::copy_project(const char *from_project, const char *to_project) {
  const char *from_basename = nullptr;
  const char *to_basename = nullptr;
  if (!split_project_path(from_project, &from_basename) ||
      !split_project_path(to_project, &to_basename)) {
    return false;
  }

  if (project_loaded && strcmp(from_project, mcl_cfg.project) == 0) {
    if (!sync_grid() || !store_config_from_system()) {
      return false;
    }
  }

  chdir_projects();
  if (SD.exists(to_project) || !SD.mkdir(to_project, true)) {
    return false;
  }

  draw_wait_popup("CLONING PROJECT");

  char from_name[PRJ_NAME_LEN + 5] = {'\0'};
  char to_name[PRJ_NAME_LEN + 5] = {'\0'};
  char from_path[PRJ_PATH_LEN + PRJ_NAME_LEN + 6] = {'\0'};
  char to_path[PRJ_PATH_LEN + PRJ_NAME_LEN + 6] = {'\0'};
  bool ok = project_file_name(from_basename, from_name, sizeof(from_name)) &&
            project_file_name(to_basename, to_name, sizeof(to_name)) &&
            join_project_file(from_path, sizeof(from_path), from_project,
                              from_name) &&
            join_project_file(to_path, sizeof(to_path), to_project, to_name) &&
            mcl_sd.copy_file(from_path, to_path);

  bool copied_pair = false;
  for (uint8_t pair = 0; ok && pair < 128; pair++) {
    chdir_projects();
    bool exists = SD.chdir(from_project) &&
                  project_pair_exists(pair, from_basename);
    if (!exists) {
      continue;
    }
    ok = copy_grid_pair(from_project, from_basename, to_project, to_basename,
                        pair, pair);
    copied_pair = copied_pair || ok;
  }

  if (!copied_pair) {
    ok = false;
  }

  if (!ok) {
    chdir_projects();
    mcl_sd.remove_dir(to_project);
    return false;
  }
  chdir_projects();
  return true;
}

bool Project::move_project(const char *from_project, const char *to_project) {
  if (strcmp(from_project, to_project) == 0) {
    return false;
  }

  bool reload_current = project_loaded &&
                        strcmp(from_project, mcl_cfg.project) == 0;
  if (!copy_project(from_project, to_project)) {
    return false;
  }

  if (reload_current) {
    close_project();
    project_loaded = false;
  }

  chdir_projects();
  if (!mcl_sd.remove_dir(from_project)) {
    if (reload_current) {
      if (!load_project(to_project)) {
        load_project(from_project);
      }
    }
    return false;
  }

  if (reload_current && !load_project(to_project)) {
    return false;
  }
  return true;
}

bool Project::new_project_master_file(const char *projectname) {

  bool ret;

  DEBUG_PRINT_FN();
  DEBUG_PRINTLN(F("Creating new project master file"));

  file.close();
  active_grid_pair = 0;
  copy_project_config(&cfg, mcl_cfg);

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
