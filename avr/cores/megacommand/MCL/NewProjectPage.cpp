#include "NewProjectPage.h"
#include "MCL.h"

void NewProjectPage::setup() {}

void NewProjectPage::init() {

  DEBUG_PRINTLN("New project page");
  char my_string[sizeof(newprj)] = "project___";

  my_string[7] = (mcl_cfg.number_projects % 1000) / 100 + '0';
  my_string[7 + 1] = (mcl_cfg.number_projects % 100) / 10 + '0';
  my_string[7 + 2] = (mcl_cfg.number_projects % 10) + '0';
  m_strncpy(newprj, my_string, sizeof(newprj));
}

void NewProjectPage::display() {
}

bool NewProjectPage::handleEvent(gui_event_t *event) {
  // don't handle any events
  if (mcl_gui.wait_for_input(newprj, "New Project:", sizeof(newprj))) {

    char full_path[sizeof(newprj) + 5] = {'\0'};
    strcat(full_path, "/");
    strcat(full_path, newprj);
    strcat(full_path, ".mcl");

    gfx.alert("PLEASE WAIT", "CREATING PROJECT");

    DEBUG_PRINTLN(full_path);
    if (SD.exists(full_path)) {
      gfx.alert("PROJECT EXISTS", "");
      DEBUG_PRINTLN("Project exists");
      return false;
    }

    bool ret = proj.new_project(full_path);
    if (ret) {
      if (proj.load_project(full_path)) {
        grid_page.reload_slot_models = false;
        GUI.setPage(&grid_page);
      } else {
        gfx.alert("SD FAILURE", "--");
      }
      return false;
    }
  } else if (proj.project_loaded) {
    GUI.setPage(&grid_page);
    return true;
  }
}
