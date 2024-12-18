#include "LoadProjectPage.h"
#include "MCLGFX.h"
#include "MCLSysConfig.h"
#include "Project.h"

void LoadProjectPage::init() {

  DEBUG_PRINTLN("load project page init");
  DEBUG_PRINT_FN();
  show_dirs = true;
  select_dirs = true;
  show_save = false;
  show_parent = false;
  show_new_folder = false;
  show_filemenu = true;
  show_overwrite = false;

  FileBrowserPage::init();
  strncpy(focus_match, mcl_cfg.project, PRJ_NAME_LEN);
  query_filesystem();
}

void LoadProjectPage::setup() {
  FileBrowserPage::setup();
  _cd(PRJ_DIR);
}

void LoadProjectPage::on_select(const char *entry) {
  DEBUG_PRINT_FN();
  DEBUG_DUMP(entry);

  file.close();

  char proj_filename[f_len] = {'\0'};
  strcpy(proj_filename, entry);
  uint8_t count = 2;
  while (count--) {
    if (proj.load_project(proj_filename)) {
      DEBUG_PRINTLN("loaded, setting grid");
      mcl.setPage(GRID_PAGE);
      return;
    } else {
      gfx.alert("PROJECT ERROR", "NOT COMPATIBLE");
      memcpy(proj_filename, mcl_cfg.project, sizeof(proj_filename));
    }
  }
}

void LoadProjectPage::on_delete(const char *entry) {
  file.open(entry, O_READ);
  bool dir = file.isDirectory();
  file.close();
  if (strcmp(mcl_cfg.project, entry) == 0) {
    gfx.alert("ERROR", "CURRENT PROJECT");
    return;
  }
  if (dir) {
    rm_dir(entry);
  }
}

void LoadProjectPage::on_rename(const char *from, const char *to) {
  /*
  char to[f_len] = {0};
  strncpy(to,to_,f_len);
  char from[f_len] = {0};
  strncpy(from,from_,f_len);
*/
  DEBUG_PRINTLN("on rename");
  file.open(from, O_READ);
  bool dir = file.isDirectory();
  file.close();

  if (!dir) {
    return;
  }

  char grid_filename[f_len] = {'\0'};
  char to_grid_filename[f_len] = {'\0'};
  char proj_filename[f_len] = {'\0'};

  char to_proj_filename[f_len] = {'\0'};
  bool reload_current = false;
  uint8_t l, l2 = 0;
  if (!SD.chdir(from)) {
    goto error;
  }

  if (strcmp(mcl_cfg.project, from) == 0) {
    DEBUG_PRINTLN("reload current");
    reload_current = true;
  }
  strncpy(proj_filename, from, f_len);
  strcat(proj_filename, ".mcl");

  strncpy(to_proj_filename, to, f_len);
  strcat(to_proj_filename, ".mcl");

  strncpy(to_grid_filename, to, f_len);
  strncpy(grid_filename, from, f_len);
  l = strlen(grid_filename);
  l2 = strlen(to_grid_filename);

  for (uint8_t i = 0; i < NUM_GRIDS; i++) {
    grid_filename[l] = '.';
    grid_filename[l + 1] = i + '0';
    grid_filename[l + 2] = '\0';

    to_grid_filename[l2] = '.';
    to_grid_filename[l2 + 1] = i + '0';
    to_grid_filename[l2 + 2] = '\0';

    if (!SD.rename(grid_filename, to_grid_filename)) {
      DEBUG_PRINTLN("Rename failed");
      goto error;
    }
  }

  if (!SD.rename(proj_filename, to_proj_filename)) {
    goto error;
  }
  SD.chdir(lwd);
  if (SD.rename(from, to)) {
    if (reload_current) {
      proj.load_project(to);
    }
    gfx.alert("SUCCESS", "Project renamed.");
    return;
  }
error:
  DEBUG_PRINTLN("error");
  gfx.alert("ERROR", "Not renamed.");
}
