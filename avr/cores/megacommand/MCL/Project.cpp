#include "Project.h"
#include "MCL.h"

void Project::setup() {}

bool Project::new_project() {
  char newprj[14];

  char my_string[sizeof(newprj)] = "project___";

  my_string[7] = (mcl_cfg.number_projects % 1000) / 100 + '0';
  my_string[7 + 1] = (mcl_cfg.number_projects % 100) / 10 + '0';
  my_string[7 + 2] = (mcl_cfg.number_projects % 10) + '0';
  m_strncpy(newprj, my_string, sizeof(newprj));
  again:

  if (mcl_gui.wait_for_input(newprj, "New Project:", sizeof(newprj))) {

    char full_path[sizeof(newprj) + 5] = {'\0'};
    strcat(full_path, "/");
    strcat(full_path, newprj);
    strcat(full_path, ".mcl");

    gfx.alert("PLEASE WAIT", "CREATING PROJECT");

    DEBUG_PRINTLN(full_path);
    if (SD.exists(full_path)) {
      gfx.alert("ERROR", "PROJECT EXISTS");
      goto again;
    }   

    bool ret = proj.new_project(full_path);
    if (ret) {
      if (proj.load_project(full_path)) {
        grid_page.reload_slot_models = false;
        DEBUG_PRINTLN("project loaded, setting page to grid");
        GUI.setPage(&grid_page);
        return true;
      } else {
        gfx.alert("ERROR", "SD ERROR");
        goto again;
      }   
      return false;
    }   
  } else if (proj.project_loaded) {
    GUI.setPage(&grid_page);
    return true;
  }
}

bool Project::load_project(const char *projectname) {

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
    DEBUG_PRINTLN("could not write cfg");
    return false;
  }

  return true;
}

bool Project::check_project_version() {
  bool ret;

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

bool Project::new_project(const char *projectname) {

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
//    if (i % 16 == 0) {
      mcl_gui.draw_progress("INITIALIZING", i, GRID_LENGTH);
  //  }
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
      DEBUG_PRINTLN("coud not clear row");
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
  DEBUG_PRINTLN("project created");
  // if (!ret) {
  // return false;
  // }

  return true;
}
Project proj;
