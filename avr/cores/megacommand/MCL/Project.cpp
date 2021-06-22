#include "MCL_impl.h"

#define PRJ_NAME_LEN 14
#define PRJ_DIR "/Projects"

void Project::setup() {}

bool Project::new_project(const char *newprj) {
  // Create parent project directory
  //
  chdir_projects();

  // Create project directory
  SD.mkdir(newprj, true);

  if (!SD.chdir(newprj)) {
    gfx.alert("ERROR", "DIR");
    return false;
  }


  char proj_filename[PRJ_NAME_LEN + 4] = {'\0'};
  strcpy(proj_filename, newprj);
  strcat(proj_filename, ".mcl");

  gfx.alert("PLEASE WAIT", "CREATING PROJECT");

  DEBUG_PRINTLN(proj_filename);
  if (SD.exists(proj_filename)) {
    gfx.alert("ERROR", "PROJECT EXISTS");
    return false;
  }

  // Initialise Grid Files.
  //

  char grid_filename[PRJ_NAME_LEN + 4] = {'\0'};
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
      GUI.setPage(&grid_page);
      return true;
    } else {
      gfx.alert("ERROR", "SD ERROR");
      goto again;
    }
  }
  if (proj.project_loaded) {
    GUI.setPage(&grid_page);
    return true;
  }
  return false;
}

void Project::chdir_projects() {
  const char *c_project_root = PRJ_DIR;
  SD.mkdir(c_project_root, true);
  SD.chdir(c_project_root);
}

#define OLD_PROJ_VERSION 2025

bool Project::convert_project(const char *projectname) {
  // TODO

  Project src_proj;
  char filename[PRJ_NAME_LEN + 4];
  strcpy(filename, projectname);
  filename[strlen(projectname) - 4] = '\0'; // truncate filename

  DEBUG_DUMP(filename);

  bool ret;
  ret = src_proj.file.open(projectname, O_RDWR);
  if (!ret) {
    DEBUG_PRINTLN("can't open");
    goto error;
  }
  ret = src_proj.check_project_version(OLD_PROJ_VERSION);
  DEBUG_DUMP(src_proj.version);
  if (!ret) {
    DEBUG_PRINTLN("Bad verision");
    goto error;
  }

  if (!new_project(filename)) {
    DEBUG_PRINTLN("new proj failed");
    goto error;
  }

  if (!load_project(filename)) {
    DEBUG_PRINTLN("load failed");
    goto error;
  }
  Grid_270 src_grid;
  KitExtra kit_extra;

  for (uint8_t y = 0; y < GRID_LENGTH_270; y++) {
    mcl_gui.draw_progress("CONVERTING", y, GRID_LENGTH_270);
    src_proj.file.seekSet(src_grid.get_row_header_offset(y));

    GridRowHeader_270 row_header_src;
    mcl_sd.read_data(&row_header_src, sizeof(GridRowHeader_270), &src_proj.file);

    DEBUG_DUMP(row_header_src.active);
    if (!row_header_src.active)
      continue;

    GridRowHeader row_headers[NUM_DEVS];

    for (uint8_t a = 0; a < NUM_DEVS; a++) {
      row_headers[a].init();
      memcpy(&row_headers[a].name, &row_header_src.name, 17);
      row_headers[a].active = row_header_src.active;
    }
    uint8_t first_track = 255;
    for (uint8_t x = 0; x < GRID_WIDTH_270; x++) {

      src_proj.file.seekSet(src_grid.get_slot_offset(x, y));
      uint8_t grid = 0;
      if (x < NUM_MD_TRACKS) {

        select_grid(grid);

        MDTrack_270 md_track_src;
        mcl_sd.read_data(&md_track_src, sizeof(MDTrack_270), &src_proj.file);

        //Fix bug whereby unpopulated slots still were not initialiased on delete/copy/paste
        if (row_header_src.track_type[x] == EMPTY_TRACK_TYPE || row_header_src.track_type[x] == 255) {
        md_track_src.active = EMPTY_TRACK_TYPE;
        }

        MDTrack md_track;
        md_track.convert(&md_track_src);

        if (md_track.active == MD_TRACK_TYPE) {
          md_track.store_in_grid(x, y);
        }
        else {
          ((GridTrack)md_track).store_in_grid(x, y);
        }

        if (md_track_src.active == MD_TRACK_TYPE_270) {
          if (first_track == 255) {
            // Extract MDFX from first active MD_TRACK slot in row
            first_track = x;
            MDFXTrack md_fx_track;
            md_fx_track.get_fx_from_kit_extra(&md_track_src.kitextra);
            md_fx_track.link.init(y);
            select_grid(1);
            md_fx_track.store_in_grid(MDFX_TRACK_NUM, y);
            row_headers[1].update_model(MDFX_TRACK_NUM, MDFX_TRACK_TYPE,
                                        MDFX_TRACK_TYPE);
          }
          row_headers[grid].update_model(x, md_track_src.machine.get_model(),
                                         md_track.active);
        }
      } else {
        grid = 1;
        select_grid(grid);

        A4Track_270 a4_track_src;
        mcl_sd.read_data(&a4_track_src, sizeof(A4Track_270), &src_proj.file);
        //Fix bug whereby unpopulated slots still were not initialiased on delete/copy/paste
        if (row_header_src.track_type[x] == EMPTY_TRACK_TYPE || row_header_src.track_type[x] == 255) {
        a4_track_src.active = EMPTY_TRACK_TYPE;
        }

        if (a4_track_src.active == EXT_TRACK_TYPE_270) {
          ExtTrack ext_track;
          ext_track.convert((ExtTrack_270 *)&a4_track_src);
          ext_track.seq_data.channel = x - NUM_MD_TRACKS;
          ext_track.store_in_grid(x - NUM_MD_TRACKS, y);
          row_headers[grid].update_model(x - NUM_MD_TRACKS, EXT_TRACK_TYPE, EXT_TRACK_TYPE);
        }
        if (a4_track_src.active == A4_TRACK_TYPE_270) {
          A4Track a4_track;
          a4_track.convert(&a4_track_src);
          a4_track.seq_data.channel = x - NUM_MD_TRACKS;
          a4_track.store_in_grid(x - NUM_MD_TRACKS, y);
          row_headers[grid].update_model(x - NUM_MD_TRACKS, A4_TRACK_TYPE, A4_TRACK_TYPE);
        }
      }
    }
    for (uint8_t a = 0; a < NUM_DEVS; a++) {
      proj.write_grid_row_header(&row_headers[a], y, a);
      proj.sync_grid(a);
    }
  }
  select_grid(0);
  src_proj.file.close();
  // Old projects located in root dir
  return true;

error:
  src_proj.file.close();
  load_project(mcl_cfg.project);
  DEBUG_PRINTLN("error");
  return false;
}

bool Project::load_project(const char *projectname) {

  bool ret;

  DEBUG_PRINT_FN();
  DEBUG_PRINTLN(F("Loading project"));
  DEBUG_PRINTLN(projectname);
  file.close();

  uint8_t l = strlen(projectname);
  DEBUG_PRINTLN(l);
  char proj_filename[PRJ_NAME_LEN + 4] = {'\0'};
  strcpy(proj_filename, projectname);
  strcat(proj_filename, ".mcl");

  char grid_name[PRJ_NAME_LEN + 4] = {'\0'};
  strcpy(grid_name, projectname);

  // Open project parent
  chdir_projects();

  // Open project directory.
  SD.chdir(projectname);

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

  for (uint8_t i = 0; i < NUM_GRIDS; i++) {
    grids[i].close_file();

    grid_name[l] = '.';
    grid_name[l + 1] = i + '0';
    grid_name[l + 2] = '\0';
    DEBUG_PRINTLN(F("opening grid"));
    DEBUG_PRINTLN(grid_name);
    if (!grids[i].open_file(grid_name)) {
      DEBUG_PRINTLN(F("could not open grid"));
      gfx.alert("ERROR", "OPEN GRID");
      return false;
    }
  }

  strncpy(mcl_cfg.project, projectname, 16);

  ret = mcl_cfg.write_cfg();

  if (!ret) {
    DEBUG_PRINTLN(F("could not write cfg"));
    return false;
  }
  grid_page.row_scan = GRID_LENGTH;
  return true;
}

bool Project::check_project_version(uint16_t version_current) {
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
  if (version >= version_current) {
    project_loaded = true;
    return true;
  } else {
    return false;
  }
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
  ret = file.createContiguous(projectname, (uint32_t)GRID_SLOT_BYTES);
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

  uint8_t ledstatus = 0;
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
