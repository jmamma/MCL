#include "MCL_impl.h"

void ConvertProjectPage::init() {

  DEBUG_PRINT_FN();
  strcpy(title, "Project");
  strcpy(lwd, "/");
  SD.chdir("/");

  show_dirs = false;
  select_dirs = false;
  show_save = false;
  show_parent = false;
  show_new_folder = false;
  show_filemenu = true;
  show_overwrite = false;

  FileBrowserPage::init();
}

void ConvertProjectPage::on_select(const char *entry) {
  DEBUG_PRINT_FN();
  DEBUG_DUMP(entry);
  file.close();
  if (proj.convert_project(entry)) {
    GUI.setPage(&grid_page);
  } else {
//    gfx.alert("PROJECT ERROR", "NOT COMPATIBLE");
  }
}
