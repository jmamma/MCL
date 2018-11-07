#include "MCL.h"
#include "Project.h"

void Project::setup() {}

bool Project::load_project(char *projectname) {

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

  m_strncpy(mcl_cfg.project, projectname, 16);

  ret = mcl_cfg.write_cfg();

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
  ret = mcl_sd.read_data((uint8_t *)this, sizeof(ProjectHeader), &file);

  if (!ret) {
    DEBUG_PRINTLN("Could not read project header");
    return false;
  }
  if (version >= PROJ_VERSION) {
    project_loaded = true;
    return true;
  } else {
    return false;
  }
}

bool Project::write_header() {

  bool ret;
  int b;

  DEBUG_PRINT_FN();
  DEBUG_PRINTLN("Writing project header");

  version = PROJ_VERSION;
  //  Config mcl_cfg.
  //  uint8_t reserved[16];
  hash = 0;

  ret = file.seekSet(0);

  if (!ret) {

    DEBUG_PRINTLN("Seek failed");

    return false;
  }

  ret = mcl_sd.write_data((uint8_t *)this, sizeof(ProjectHeader), &file);

  if (!ret) {
    DEBUG_PRINTLN("Write header failed");
    return false;
  }
  DEBUG_PRINTLN("Write header success");
  return true;
}

bool Project::new_project(char *projectname) {

  bool ret;

  DEBUG_PRINT_FN();
  DEBUG_PRINTLN("Creating new project");

  file.close();

  DEBUG_PRINTLN("Attempting to extend project file");

  ret = file.createContiguous(projectname, (uint32_t)GRID_SLOT_BYTES +
                                               (uint32_t)GRID_SLOT_BYTES *
                                                   (uint32_t)GRID_LENGTH *
                                                   (uint32_t)(GRID_WIDTH + 1));

  if (!ret) {
    file.close();
    DEBUG_PRINTLN("Could not extend file");
    return false;
  }
  DEBUG_PRINTLN("extension succeeded, trying to close");
  file.close();

  ret = file.open(projectname, O_RDWR);

  if (!ret) {
    file.close();

    DEBUG_PRINTLN("Could not open file");
    return false;
  }

  uint8_t ledstatus = 0;

  DEBUG_PRINTLN("Initializing project.. please wait");
#ifdef OLED_DISPLAY
  oled_display.drawRect(15, 23, 98, 6, WHITE);
#endif
  // Initialise the project file by filling the grid with blank data.
  for (int32_t i = 0; i < GRID_LENGTH; i++) {

#ifdef OLED_DISPLAY
          if (i % 16 == 0) {
        oled_display.fillRect(15, 23, ((float)i / (float)GRID_LENGTH) * 98, 6,
                              WHITE);
        oled_display.display();
    }
#endif
          if (i % 2 == 0) {
      if (ledstatus == 0) {
        setLed2();
        ledstatus = 1;
     } else {
        clearLed2();
        ledstatus = 0;
      }
    }

    ret = grid.clear_row(i);
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

  if (!write_header()) {
    return false;
  }

  // m_strncpy(mcl_cfg.project, projectname, 16);
  file.close();
  mcl_cfg.number_projects++;
  mcl_cfg.write_cfg();

  // if (!ret) {
  // return false;
  // }

  return true;
}
Project proj;
