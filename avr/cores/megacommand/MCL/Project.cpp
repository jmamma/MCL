#include "MCL.h"
#include "Project.h"

#define PRJ_NAME_LEN 14
#define PRJ_DIR "/Projects"

void Project::setup() {}

bool Project::new_project() {
  char newprj[PRJ_NAME_LEN];

  char my_string[sizeof(newprj)] = "project___";

  my_string[7] = (mcl_cfg.number_projects % 1000) / 100 + '0';
  my_string[7 + 1] = (mcl_cfg.number_projects % 100) / 10 + '0';
  my_string[7 + 2] = (mcl_cfg.number_projects % 10) + '0';

  strncpy(newprj, my_string, sizeof(newprj));
again:

  if (mcl_gui.wait_for_input(newprj, "New Project:", sizeof(newprj))) {

    // Create project directory
    chdir_projects();

    char proj_filename[sizeof(newprj) + 5] = {'\0'};
    strcat(proj_filename, newprj);
    strcat(proj_filename, ".mcl");

    gfx.alert("PLEASE WAIT", "CREATING PROJECT");

    DEBUG_PRINTLN(proj_filename);
    if (SD.exists(proj_filename)) {
      gfx.alert("ERROR", "PROJECT EXISTS");
      goto again;
    }

    // Initialise Grid Files.
    //
    char grid_filename[sizeof(newprj) + 2];
    strncpy(grid_filename, newprj, sizeof(newprj));
    uint8_t l = strlen(grid_filename);
    for (uint8_t i = 0; i < NUM_GRIDS; i++) {
      grid_filename[l] = '.';
      grid_filename[l + 1] = i + '0';
      grid_filename[l + 2] = '\0';
      if (!SD.exists(grid_filename)) {
        if (!grids[i].new_grid(grid_filename)) {
          gfx.alert("ERROR", "SD ERROR");
          goto again;
        }
      }
    }
    // Initialiase Project File.
    //
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

void Project::chdir_projects() {
  const char *c_project_root = PRJ_DIR;
  SD.mkdir(c_project_root, true);
  SD.chdir(c_project_root);
}

bool Project::load_project(const char *projectname) {

  bool ret;

  DEBUG_PRINT_FN();
  DEBUG_PRINTLN("Loading project");
  DEBUG_PRINTLN(projectname);
  file.close();

  char name[PRJ_NAME_LEN + 2];
  uint8_t l = strlen(projectname) - 4;
  strncpy(name, projectname, l);

  const char *c_project_root = PRJ_DIR;
  chdir_projects();
  //Open project directory.
  SD.chdir(name);

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

 for (uint8_t i = 0; i < NUM_GRIDS; i++) {
    grids[i].close_file();

    name[l] = '.';
    name[l + 1] = i + '0';
    name[l + 2] = '\0';
    DEBUG_PRINTLN("opening grid");
    DEBUG_PRINTLN(name);
    if (!grids[i].open_file(name)) {
      DEBUG_PRINTLN("could not open grid");
      gfx.alert("ERROR", "OPEN GRID");
      return false;
    }
 }

  strncpy(mcl_cfg.project, projectname, 16);

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
