#include "GUI/Pages/Project/ProjectVersionPage.h"
#include "GUI/MCLGfx.h"
#include "MCLGUI.h"
#include "GUI/MCLMenus.h"
#include "Project.h"

#ifdef MCL_HAS_PROJECT_BACKUP

void ProjectVersionPage::set_project(const char *project) {
  strncpy(lwd, project, sizeof(lwd) - 1);
  lwd[sizeof(lwd) - 1] = '\0';
}

void ProjectVersionPage::setup() {
  oled_display.clearDisplay();
  encoders[1]->cur = 0;
  encoders[1]->old = 0;
  cur_row = 0;
}

void ProjectVersionPage::init() {
  reset_browser_options();
  show_dirs = false;
  select_dirs = false;
  show_save = true;
  show_parent = false;
  show_new_folder = false;
  show_filemenu = true;
  strcpy(title, "VERSION");
  strcpy(str_save, "[ BACKUP ]");
  query_versions();
}

void ProjectVersionPage::query_versions() {
  numEntries = 0;
  cur_file = 255;
  add_entry(str_save, FILE_TYPE);

  uint8_t active_pair = 0;
  bool have_active = proj.read_active_grid_pair(lwd, &active_pair);

  proj.chdir_projects();
  if (!SD.chdir(lwd)) {
    add_entry("ERROR", FILE_TYPE);
    ((MCLEncoder *)encoders[1])->max = numEntries - 1;
    return;
  }

  const char *project_name = strrchr(lwd, '/');
  project_name = project_name == nullptr ? lwd : project_name + 1;

  for (uint8_t pair = 0; pair < 128; pair++) {
    if (!proj.project_pair_exists(pair, project_name)) {
      continue;
    }

    char label[5] = {'V'};
    mcl_gui.put_value_at(pair + 1, label + 1);
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
                      proj.read_active_grid_pair(lwd, &active_pair) &&
                      pair != active_pair;
    uint16_t disabled = FM_MASK(FM_NEW_FOLDER) | FM_MASK(FM_RENAME) |
                        FM_MASK(FM_DUPLICATE) | FM_MASK(FM_MOVE) |
                        FM_MASK(FM_VERSIONS) | FM_MASK(FM_RECVALL) |
                        FM_MASK(FM_SENDALL);
    if (!can_delete) {
      disabled |= FM_MASK(FM_DELETE);
    }
    set_file_menu_disabled_mask(disabled);
    open_filemenu();
    return true;
  }
  return FileBrowserPage::handleEvent(event);
}

void ProjectVersionPage::on_new() {
  uint8_t pair = 0;
  if (!proj.create_backup(lwd, &pair)) {
    gfx.alert_error("No backup.");
    init();
    return;
  }
  if (proj.load_project_version(lwd, pair)) {
    mcl.setPage(GRID_PAGE);
  } else {
    gfx.alert_error("OPEN VERSION");
    init();
  }
}

void ProjectVersionPage::on_select(const char *entry) {
  (void)entry;
  uint8_t pair = 0;
  if (!selected_pair(&pair)) {
    return;
  }
  if (proj.load_project_version(lwd, pair)) {
    mcl.setPage(GRID_PAGE);
  } else {
    gfx.alert_error("OPEN VERSION");
  }
}

void ProjectVersionPage::on_delete(const char *entry) {
  (void)entry;
  uint8_t pair = 0;
  if (!selected_pair(&pair)) {
    return;
  }
  if (proj.delete_backup(lwd, pair)) {
    gfx.alert_success("Deleted.");
  } else {
    gfx.alert_error("Not deleted.");
  }
  init();
}

#endif
