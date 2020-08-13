#include "MCL.h"
#include "Project.h"

void Project::setup() {}

bool Project::new_project() {
  char newprj[14];

  const char *c_project_root = "/Projects";

  SD.mkdir(c_project_root, true);
  SD.chdir(c_project_root);

  char my_string[sizeof(newprj)] = "project___";

  my_string[7] = (mcl_cfg.number_projects % 1000) / 100 + '0';
  my_string[7 + 1] = (mcl_cfg.number_projects % 100) / 10 + '0';
  my_string[7 + 2] = (mcl_cfg.number_projects % 10) + '0';

  m_strncpy(newprj, my_string, sizeof(newprj));
again:

  if (mcl_gui.wait_for_input(newprj, "New Project:", sizeof(newprj))) {

    // Create project directory
    SD.mkdir(newprj, true);
    SD.chdir(newprj);

    char proj_filename[sizeof(newprj) + 5] = {'\0'};
    strcat(proj_filename, newprj);
    strcat(proj_filename, ".mcl");

    gfx.alert("PLEASE WAIT", "CREATING PROJECT");

    DEBUG_PRINTLN(proj_filename);
    if (SD.exists(proj_filename)) {
      gfx.alert("ERROR", "PROJECT EXISTS");
      goto again;
    }

    char grid_filename[sizeof(newprj) + 3] = {'\0'};
    for (uint8_t i = 0; i < NUM_GRIDS; i++) {
      grid_filename[sizeof(newprj) + 1] = '.';
      grid_filename[sizeof(newprj) + 2] = i + '0';
      if (!SD.exists(grid_filename)) {
        if (!grids[i].new_grid(grid_filename)) {
          gfx.alert("ERROR", "SD ERROR");
          goto again;
        }
      }
    }

    bool ret = proj.new_project(proj_filename);
    if (ret) {
      if (proj.load_project(proj_filename)) {
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

  ret = file.createContiguous(projectname, (uint32_t)GRID_SLOT_BYTES);

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
