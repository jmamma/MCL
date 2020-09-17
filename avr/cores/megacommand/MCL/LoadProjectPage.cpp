#include "MCL_impl.h"

void LoadProjectPage::init() {

  DEBUG_PRINT_FN();
  // strcpy(match, ".mcl");
  strcpy(title, "Project");
  strcpy(lwd, "PRJ_DIR");
  SD.chdir(PRJ_DIR);

  show_save = false;
  show_dirs = true;
  select_dirs = true;
  show_filemenu = true;
  show_new_folder = false;
  show_overwrite = false;
  show_parent = false;

  FileBrowserPage::init();
}

void LoadProjectPage::on_select(const char *entry) {
  DEBUG_PRINT_FN();
  DEBUG_DUMP(entry);
  if (file.isDirectory()) {
    _cd(entry);
  } else {
    return;
  }

  file.close();

  char proj_filename[PRJ_NAME_LEN + 5] = {'\0'};
  strcat(proj_filename, entry);

  if (proj.load_project(proj_filename)) {
    GUI.setPage(&grid_page);
  } else {
    gfx.alert("PROJECT ERROR", "NOT COMPATIBLE");
  }

}
