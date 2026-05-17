#include "LoadProjectPage.h"
#include "MCLGFX.h"
#include "MCLSd.h"
#include "MCLSysConfig.h"
#include "Project.h"
#include "DevicePanelRef.h"
#include "KeyInterface.h"

namespace {

bool path_starts_with_dir(const char *path, const char *dir) {
  size_t dir_len = strlen(dir);
  if (dir_len == 0) {
    return false;
  }
  return strncmp(path, dir, dir_len) == 0 &&
         (path[dir_len] == '\0' || path[dir_len] == '/');
}

} // namespace

void LoadProjectPage::init() {

  DEBUG_PRINTLN("load project page init");
  DEBUG_PRINT_FN();
  DevicePanelRef::set_primary_key_repeat(1);
  FileBrowserPage::init();

  show_dirs = true;
  select_dirs = false;
  show_save = false;
  show_parent = true;
  show_new_folder = true;
  show_filemenu = true;
  show_overwrite = false;
  draw_dirs = true;
  strcpy(title, "PROJECT");

  focus_current_project();
  query_filesystem();
}

void LoadProjectPage::cleanup() {
  FileBrowserPage::cleanup();
  key_interface.ignoreNextEventClear(MDX_KEY_YES);
  key_interface.ignoreNextEventClear(MDX_KEY_NO);
}

void LoadProjectPage::setup() {
  FileBrowserPage::setup();
#ifndef __AVR__
  char path[64];
  _cd(mcl_sd.full_path(PRJ_DIR, path, sizeof(path)));
#else
  _cd(PRJ_DIR);
#endif
  position.reset();
}

void LoadProjectPage::on_select(const char *entry) {
  DEBUG_PRINT_FN();
  DEBUG_DUMP(entry);

  file.close();

  char proj_filename[PRJ_PATH_LEN] = {'\0'};
  if (!build_project_path(entry, proj_filename, sizeof(proj_filename))) {
    gfx.alert("ERROR", "BAD PATH");
    return;
  }
  uint8_t count = 2;
  while (count--) {
    if (proj.load_project(proj_filename)) {
      DEBUG_PRINTLN("loaded, setting grid");
      mcl.setPage(GRID_PAGE);
      return;
    } else {
      gfx.alert("Error", "Not compatible");
      strncpy(proj_filename, mcl_cfg.project, sizeof(proj_filename) - 1);
      proj_filename[sizeof(proj_filename) - 1] = '\0';
    }
  }
}

void LoadProjectPage::on_delete(const char *entry) {
  file.open(entry, O_READ);
  bool dir = file.isDirectory();
  file.close();
  char project_path[PRJ_PATH_LEN] = {'\0'};
  if (build_project_path(entry, project_path, sizeof(project_path)) &&
      path_starts_with_dir(mcl_cfg.project, project_path)) {
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

  bool project_dir = is_project_dir(from);
  char from_project_path[PRJ_PATH_LEN] = {'\0'};
  if (!build_project_path(from, from_project_path, sizeof(from_project_path))) {
    gfx.alert("ERROR", "BAD PATH");
    return;
  }

  if (!project_dir) {
    if (path_starts_with_dir(mcl_cfg.project, from_project_path)) {
      gfx.alert("ERROR", "CURRENT PROJECT");
      return;
    }
    FileBrowserPage::on_rename(from, to);
    return;
  }

  char to_project_path[PRJ_PATH_LEN] = {'\0'};
  if (!build_project_path(to, to_project_path, sizeof(to_project_path))) {
    gfx.alert("ERROR", "BAD PATH");
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

  if (strcmp(mcl_cfg.project, from_project_path) == 0) {
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
      proj.load_project(to_project_path);
    }
    gfx.alert("SUCCESS", "Project renamed.");
    return;
  }
error:
  DEBUG_PRINTLN("error");
  gfx.alert("ERROR", "Not renamed.");
}

bool LoadProjectPage::current_project_parent(const char **parent) const {
  char root[64];
  const char *root_path = mcl_sd.full_path(PRJ_DIR, root, sizeof(root));
  size_t root_len = strlen(root_path);

  if (strcmp(lwd, root_path) == 0) {
    *parent = "";
    return true;
  }
  if (strncmp(lwd, root_path, root_len) == 0 && lwd[root_len] == '/') {
    *parent = lwd + root_len + 1;
    return true;
  }
  return false;
}

bool LoadProjectPage::can_show_parent_entry() const {
  const char *parent = nullptr;
  return show_parent && current_project_parent(&parent) && parent[0] != '\0';
}

#ifdef PLATFORM_TBD
bool LoadProjectPage::tbd_can_cd_up() const {
  return can_show_parent_entry();
}
#endif

uint8_t LoadProjectPage::entry_type_for_dir(const char *entry) {
  return is_project_dir(entry) ? FILE_TYPE : DIR_TYPE;
}

bool LoadProjectPage::is_project_dir(const char *entry) const {
  size_t len = strlen(entry);
  if (len == 0 || len > PRJ_NAME_LEN || strchr(entry, '/') != nullptr) {
    return false;
  }

  char project_file[PRJ_NAME_LEN * 2 + 6] = {'\0'};
  strcpy(project_file, entry);
  strcat(project_file, "/");
  strcat(project_file, entry);
  strcat(project_file, ".mcl");
  return SD.exists(project_file);
}

bool LoadProjectPage::build_project_path(const char *entry, char *out,
                                         size_t out_len) const {
  if (out_len == 0) {
    return false;
  }
  out[0] = '\0';

  const char *parent = nullptr;
  if (!current_project_parent(&parent)) {
    return false;
  }

  size_t parent_len = strlen(parent);
  size_t entry_len = strlen(entry);
  if (entry_len == 0 || entry_len > PRJ_NAME_LEN) {
    return false;
  }
  size_t needed = parent_len + (parent_len ? 1 : 0) + entry_len + 1;
  if (needed > out_len) {
    return false;
  }

  if (parent_len) {
    strcpy(out, parent);
    strcat(out, "/");
  }
  strcat(out, entry);
  return true;
}

void LoadProjectPage::focus_current_project() {
  focus_match[0] = '\0';

  const char *parent = nullptr;
  if (!current_project_parent(&parent)) {
    return;
  }

  const char *project_path = mcl_cfg.project;
  size_t parent_len = strlen(parent);
  const char *focus = nullptr;

  if (parent_len == 0) {
    focus = project_path;
  } else if (strncmp(project_path, parent, parent_len) == 0 &&
             project_path[parent_len] == '/') {
    focus = project_path + parent_len + 1;
  } else {
    return;
  }

  const char *slash = strchr(focus, '/');
  size_t len = slash == nullptr ? strlen(focus) : (size_t)(slash - focus);
  if (len >= sizeof(focus_match)) {
    len = sizeof(focus_match) - 1;
  }
  memcpy(focus_match, focus, len);
  focus_match[len] = '\0';
}
