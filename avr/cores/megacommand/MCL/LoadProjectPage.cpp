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

  file.close();

  char proj_filename[PRJ_NAME_LEN + 5] = {'\0'};
  strcpy(proj_filename, entry);

again:
  if (proj.load_project(proj_filename)) {
    GUI.setPage(&grid_page);
  } else {
    gfx.alert("PROJECT ERROR", "NOT COMPATIBLE");
    strcpy(proj_filename, mcl_cfg.project);
    goto again;
  }
}

void LoadProjectPage::on_delete(const char *entry) {
  file.open(entry, O_READ);
  bool dir = file.isDirectory();
  file.close();
  char temp_entry[16];
  if (strcmp(mcl_cfg.project, entry) == 0) {
    gfx.alert("ERROR", "CURRENT PROJECT");
    return;
  }
  if (dir) {
    if (SD.chdir(entry)) {
      // SD.vwd()->rmRfStar(); //extra 276 bytes
      while (file.openNext(SD.vwd(), O_READ)) {
        file.getName(temp_entry, 16);
        file.close();
        SD.remove(temp_entry);
      }
      _cd_up();
      SD.rmdir(entry);
    }
  }
}

void LoadProjectPage::on_rename(const char *from, const char *to) {

  bool reload_current = false;
  if (strcmp(mcl_cfg.project, from) == 0) {
    DEBUG_PRINTLN("reload current");
    reload_current = true;
  }
  char grid_filename[PRJ_NAME_LEN + 4] = {'\0'};
  strcpy(grid_filename, from);
  uint8_t l = strlen(grid_filename);

  char to_grid_filename[PRJ_NAME_LEN + 4] = {'\0'};
  strcpy(to_grid_filename, to);
  uint8_t l2 = strlen(to_grid_filename);

  char proj_filename[PRJ_NAME_LEN + 4] = {'\0'};
  strcpy(proj_filename, from);
  strcat(proj_filename, ".mcl");

  char to_proj_filename[PRJ_NAME_LEN + 4] = {'\0'};
  strcpy(to_proj_filename, to);
  strcat(to_proj_filename, ".mcl");

  file.open(from, O_READ);
  bool dir = file.isDirectory();
  file.close();

  if (!dir) {
    return;
  }

  if (SD.chdir(from)) {
    for (uint8_t i = 0; i < NUM_GRIDS; i++) {
      grid_filename[l] = '.';
      grid_filename[l + 1] = i + '0';
      grid_filename[l + 2] = '\0';

      to_grid_filename[l2] = '.';
      to_grid_filename[l2 + 1] = i + '0';
      to_grid_filename[l2 + 2] = '\0';

      SD.rename(grid_filename, to_grid_filename);
    }

    SD.rename(proj_filename, to_proj_filename);

    _cd_up();
  }
  if (SD.rename(from, to)) {
    if (reload_current) {
      proj.load_project(to);
    }
    gfx.alert("SUCCESS", "Project renamed.");
  } else {
    gfx.alert("ERROR", "Not renamed.");
  }
}
