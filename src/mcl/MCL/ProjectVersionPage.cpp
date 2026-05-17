#include "ProjectVersionPage.h"
#ifndef __AVR__
#include "MCLGFX.h"
#include "MCLGUI.h"
#include "MCLMenus.h"
#include "Project.h"

namespace {

bool append_u8(char *out, size_t out_len, uint8_t value) {
  char tmp[3];
  uint8_t n = 0;
  do {
    tmp[n++] = (value % 10) + '0';
    value /= 10;
  } while (value > 0 && n < sizeof(tmp));

  size_t len = strlen(out);
  if (len + n + 1 > out_len) {
    return false;
  }
  while (n > 0) {
    out[len++] = tmp[--n];
  }
  out[len] = '\0';
  return true;
}

} // namespace

void ProjectVersionPage::set_project(const char *project) {
  strncpy(project_path, project, sizeof(project_path) - 1);
  project_path[sizeof(project_path) - 1] = '\0';

  const char *basename = strrchr(project_path, '/');
  basename = basename == nullptr ? project_path : basename + 1;
  strncpy(project_name, basename, sizeof(project_name) - 1);
  project_name[sizeof(project_name) - 1] = '\0';
}

void ProjectVersionPage::setup() {
  oled_display.clearDisplay();
  encoders[1]->cur = 0;
  encoders[1]->old = 0;
  encoders[2]->cur = 0;
  cur_row = 0;
}

void ProjectVersionPage::init() {
  filemenu_active = false;
  show_samplemgr = false;
  draw_dirs = false;
  show_dirs = false;
  select_dirs = false;
  show_save = true;
  show_parent = false;
  show_new_folder = false;
  show_filemenu = true;
  show_overwrite = false;
  show_copy = false;
  show_move = false;
  show_versions = false;
  strcpy(title, "VERSIONS");
  strcpy(str_save, "BACKUP");
  query_versions();
}

void ProjectVersionPage::query_versions() {
  numEntries = 0;
  cur_file = 255;
  add_entry(str_save, FILE_TYPE);

  uint8_t active_pair = 0;
  bool have_active = proj.read_active_grid_pair(project_path, &active_pair);

  proj.chdir_projects();
  if (!SD.chdir(project_path)) {
    add_entry("ERROR", FILE_TYPE);
    ((MCLEncoder *)encoders[1])->max = numEntries - 1;
    return;
  }

  for (uint8_t pair = 0; pair < 128; pair++) {
    bool complete = true;
    for (uint8_t i = 0; i < NUM_GRIDS; i++) {
      char grid_name[PRJ_NAME_LEN + 5] = {'\0'};
      if (!proj.build_grid_filename(project_name, pair * NUM_GRIDS + i,
                                    grid_name, sizeof(grid_name)) ||
          !SD.exists(grid_name)) {
        complete = false;
        break;
      }
    }
    if (!complete) {
      continue;
    }

    char label[FILE_ENTRY_SIZE] = {'\0'};
    strcpy(label, "V");
    append_u8(label, sizeof(label), pair + 1);
    strcat(label, " ");
    append_u8(label, sizeof(label), pair * NUM_GRIDS);
    strcat(label, "/");
    append_u8(label, sizeof(label), pair * NUM_GRIDS + NUM_GRIDS - 1);
    add_entry(label, VERSION_ENTRY_BASE + pair);
    if (have_active && pair == active_pair) {
      cur_file = numEntries - 1;
    }
  }

  ((MCLEncoder *)encoders[1])->max = numEntries > 0 ? numEntries - 1 : 0;
}

bool ProjectVersionPage::selected_pair(uint8_t *pair) {
  if (pair == nullptr || encoders[1]->getValue() >= numEntries) {
    return false;
  }
  char entry[FILE_ENTRY_SIZE];
  uint8_t type = FILE_TYPE;
  get_entry(encoders[1]->getValue(), entry, type);
  if (type < VERSION_ENTRY_BASE) {
    return false;
  }
  *pair = type - VERSION_ENTRY_BASE;
  return true;
}

bool ProjectVersionPage::handleEvent(gui_event_t *event) {
  if (EVENT_BUTTON(event) && EVENT_PRESSED(event, Buttons.BUTTON3) &&
      show_filemenu) {
    uint8_t pair = 0;
    uint8_t active_pair = 0;
    bool can_delete = selected_pair(&pair) && pair > 0 &&
                      proj.read_active_grid_pair(project_path, &active_pair) &&
                      pair != active_pair;
    file_menu_page.menu.enable_entry(FM_NEW_FOLDER, false);
    file_menu_page.menu.enable_entry(FM_DELETE, can_delete);
    file_menu_page.menu.enable_entry(FM_RENAME, false);
    file_menu_page.menu.enable_entry(FM_DUPLICATE, false);
    file_menu_page.menu.enable_entry(FM_MOVE, false);
    file_menu_page.menu.enable_entry(FM_VERSIONS, false);
    file_menu_page.menu.enable_entry(FM_RECVALL, false);
    file_menu_page.menu.enable_entry(FM_SENDALL, false);
    open_filemenu();
    return true;
  }
  return FileBrowserPage::handleEvent(event);
}

void ProjectVersionPage::on_new() {
  if (proj.create_backup(project_path)) {
    gfx.alert("SUCCESS", "Backup made.");
  } else {
    gfx.alert("ERROR", "No backup.");
  }
  init();
}

void ProjectVersionPage::on_select(const char *entry) {
  (void)entry;
  uint8_t pair = 0;
  if (!selected_pair(&pair)) {
    return;
  }
  if (proj.load_project_version(project_path, pair)) {
    mcl.setPage(GRID_PAGE);
  } else {
    gfx.alert("ERROR", "OPEN VERSION");
  }
}

void ProjectVersionPage::on_delete(const char *entry) {
  (void)entry;
  uint8_t pair = 0;
  if (!selected_pair(&pair)) {
    return;
  }
  if (proj.delete_backup(project_path, pair)) {
    gfx.alert("SUCCESS", "Deleted.");
  } else {
    gfx.alert("ERROR", "Not deleted.");
  }
}
#endif
