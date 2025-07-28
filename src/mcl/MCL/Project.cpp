#include "Project.h"
#include "MCLSd.h"
#include "MCLGUI.h"
#include "GridPages.h"

#include "MDTrack.h"
#include "ExtTrack.h"
#include "A4Track.h"
#include "MNMTrack.h"
#include "MDFXTrack.h"

#define PRJ_NAME_LEN 14
#define PRJ_DIR "/Projects"

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

  gfx.alert("PLEASE WAIT", "CREATING PROJECT");

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
  const char *c_project_root = PRJ_DIR;
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

  uint8_t l = strlen(projectname);
  if (l > PRJ_NAME_LEN) { DEBUG_PRINTLN("bad len"); return false; }

  char proj_filename[PRJ_NAME_LEN  + 5] = {'\0'};
  memcpy(proj_filename, projectname, sizeof(proj_filename));
  strcat(proj_filename, ".mcl");

  char grid_name[PRJ_NAME_LEN  + 5] = {'\0'};
  strcpy(grid_name, projectname);

  // Open project parent
  chdir_projects();

  // Open project directory.
  if (!SD.exists(projectname)) {
    DEBUG_PRINTLN("dir does not exist");
    return false;
  }

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
  if (!mcl_cfg.number_projects) { mcl_cfg.number_projects++; }

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
