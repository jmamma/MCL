#include "GUI/Pages/Project/ConvertProjectPage.h"

#ifdef MCL_HAS_PROJECT_CONVERSION
#include "Project.h"

void ConvertProjectPage::init() {

  DEBUG_PRINT_FN();
  strcpy_P(title, mclstr_title_project);
  if (mcl_sd.mcl_root[0] == '\0') {
    strcpy_P(lwd, mclstr_root_path);
    SD.chdir("/");
  } else {
    strcpy(lwd, mcl_sd.mcl_root);
    SD.chdir(mcl_sd.mcl_root);
  }

  show_dirs = false;
  select_dirs = false;
  show_save = false;
  show_parent = false;
  show_new_folder = false;
  show_filemenu = true;

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
#endif
