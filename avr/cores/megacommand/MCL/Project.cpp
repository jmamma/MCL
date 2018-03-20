#include "Project.h"

void Project::setup() {}

bool Project::sd_load_project(char *projectname) {

  bool ret;

  DEBUG_PRINT_FN();
  DEBUG_PRINTLN("Loading project");
  DEBUG_PRINTLN(projectname);

  file.close();

  ret = file.open(projectname, O_RDWR);
  if (!ret) {

    DEBUG_PRINTLN("Could not open project file");
    return false;
  }
  ret = check_project_version();

  if (!ret) {
    DEBUG_PRINTLN("Project version incompatible");
    file.close();
    return false;
  }

  m_strncpy(cfg.project, projectname, 16);

  ret = cfg.write_cfg();

  if (!ret) {
    return false;
  }

  return true;

}

bool Project::check_project_version() {
  bool ret;
  int b = 0;

  DEBUG_PRINT_FN();
  DEBUG_PRINTLN("Check project version");

  ret = file.seekSet(0);

  if (!ret) {
    DEBUG_PRINTLN("Seek failed");
    return false;
  }
  ret = mcl_sd.read_data(( uint8_t*) & (project_header), sizeof(project_header), &file);

  if (!ret) {
    DEBUG_PRINTLN("Could not read project header");
    return false;
  }
  if (project_header.version >= VERSION) {
    return true;
  }
  else {
    return false;
  }
}

bool ProjectHeader::write_project_header() {

  bool ret;
  int b;

  DEBUG_PRINT_FN();
  DEBUG_PRINTLN("Writing project header");

  project_header.version = VERSION;
  //  Config cfg;
  //  uint8_t reserved[16];
  project_header.hash = 0;

  ret = file.seekSet(0);

  if (!ret) {

    DEBUG_PRINTLN("Seek failed");

    return false;
  }

  ret = mcl_sd.write_data((uint8_t *)&project_header, sizeof(project_header),
                          &file);

  if (!ret) {
    DEBUG_PRINTLN("Write header failed");
    return false;
  }
  DEBUG_PRINTLN("Write header success");
  return true;
}

bool Project::sd_new_project(char *projectname) {

  bool ret;

  DEBUG_PRINT_FN();
  DEBUG_PRINTLN("Creating new project");
  numProjects++;

  file.close();

  temptrack.active = EMPTY_TRACK_TYPE;

  // DEBUG_PRINTLN(exitcode);

  ret = file.createContiguous(projectname, (uint32_t)GRID_SLOT_BYTES +
                                               (uint32_t)GRID_SLOT_BYTES *
                                                   (uint32_t)GRID_LENGTH *
                                                   (uint32_t)GRID_WIDTH);

  if (!ret) {
    file.close();
    DEBUG_PRINTLN("Could not extend file");
    return false;
  }

  file.close();

  ret = file.open(projectname, O_RDWR);

  if (!ret) {
    file.close();

    DEBUG_PRINTLN("Could not open file");
    return false;
  }

  uint8_t ledstatus = 0;

  DEBUG_PRINTLN("Initializing project.. please wait");

  // Initialise the project file by filling the grid with blank data.
  for (int32_t i = 0; i < GRID_LENGTH * GRID_WIDTH; i++) {
    if (i % 25 == 0) {
      if (ledstatus == 0) {
        setLed2();
        ledstatus = 1;
      } else {
        clearLed2();
        ledstatus = 0;
      }
    }

    ret = clear_slot(i);
    if (!ret) {
      return false;
    }
  }
  clearLed2();
  ret = file.seekSet(0);

  if (!ret) {
    DEBUG_PRINTLN("Could not seek");
    return false;
  }

  if (!write_project_header()) {
    return false;
  }

  // m_strncpy(cfg.project, projectname, 16);
  file.close();
  cfg.number_projects++;
  cfg.write_cfg();

  // if (!ret) {
  // return false;
  // }

  return true;
}
Project proj;
