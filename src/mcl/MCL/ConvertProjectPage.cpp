#include "ConvertProjectPage.h"
#include "Project.h"

void ConvertProjectPage::init() {

  DEBUG_PRINT_FN();
  strcpy_P(title, mclstr_title_project);
  strcpy_P(lwd, mclstr_root_path);
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
    mcl.setPage(GRID_PAGE);
  } else {
//    gfx.alert("PROJECT ERROR", "NOT COMPATIBLE");
  }
}
