#include "memory.h"
#include "NoteInterface.h"
#include "helpers.h"
#include "FileBrowserPage.h"
#include "MCLSd.h"
#include "MCLGfx.h"
#include "MCLMenus.h"
#include "MCLGUI.h"

File FileBrowserPage::file;
int FileBrowserPage::numEntries;

char FileBrowserPage::title[12];
char FileBrowserPage::str_save[16];
char FileBrowserPage::focus_match[FILE_ENTRY_SIZE];
uint8_t FileBrowserPage::cur_file = 0;

bool FileBrowserPage::draw_dirs = false;
bool FileBrowserPage::show_dirs = false;
bool FileBrowserPage::select_dirs = false;
bool FileBrowserPage::show_save = true;
bool FileBrowserPage::show_parent = true;
bool FileBrowserPage::show_new_folder = true;
bool FileBrowserPage::show_filemenu = true;

bool FileBrowserPage::show_samplemgr = false;
bool FileBrowserPage::show_copy = false;
bool FileBrowserPage::show_move = false;
bool FileBrowserPage::show_versions = false;

bool FileBrowserPage::filemenu_active = false;
#ifdef MCL_HAS_FILE_MOVE
bool FileBrowserPage::move_destination_mode = false;
char FileBrowserPage::move_source_path[PRJ_PATH_LEN];
#endif

FileBrowserFileTypes FileBrowserPage::file_types;

bool FileBrowserPage::selection_change = false;

uint16_t FileBrowserPage::selection_change_clock = 0;

static uint8_t filebrowser_name_length(uint8_t min_length, const char *name) {
  uint8_t len = strlen(name);
  return len > min_length ? len : min_length;
}

void FileBrowserPage::build_delete_message(char *dst, uint8_t dst_len,
                                           const char *entry) {
  if (dst_len == 0) {
    return;
  }
  uint8_t used = 0;
  const char *prefix = mclstr_delete_space;
  while (used + 1 < dst_len) {
    char c = pgm_read_byte(prefix++);
    if (c == '\0') {
      break;
    }
    dst[used++] = c;
  }
  while (used + 2 < dst_len && *entry != '\0') {
    dst[used++] = *entry++;
  }
  if (used + 1 < dst_len) {
    dst[used++] = '?';
  }
  dst[used] = '\0';
}

bool FileBrowserPage::path_starts_with_dir(const char *path, const char *dir) {
  if (*dir == '\0') {
    return false;
  }
  while (*dir != '\0' && *path == *dir) {
    path++;
    dir++;
  }
  return *dir == '\0' && (*path == '\0' || *path == '/');
}

void FileBrowserPage::setup() {
  oled_display.clearDisplay();
  // char *mcl = ".mcl";
  // strcpy(match, mcl);
#ifndef __AVR__
  if (mcl_sd.mcl_root[0] == '\0') {
    strcpy_P(lwd, mclstr_root_path);
  } else {
    strcpy(lwd, mcl_sd.mcl_root);
  }
#else
  strcpy_P(lwd, mclstr_root_path);
#endif

  encoders[1]->cur = 1;
}

void FileBrowserPage::get_entry(uint16_t n, char *entry) {
  uint8_t discard_type;
  get_entry(n, entry, discard_type);
}

void FileBrowserPage::get_entry(uint16_t n, char *entry, uint8_t &type) {
  volatile uint8_t *ptr =
      (uint8_t *)BANK3_FILE_ENTRIES_START + n * FILE_ENTRY_SIZE;
  char buf[FILE_ENTRY_SIZE];
  get_bank3(buf, ptr, FILE_ENTRY_SIZE);
  type = buf[0];
  strcpy(entry, buf + 1);
}

void FileBrowserPage::set_entry_type(uint16_t n, uint8_t type) {
  volatile uint8_t *ptr =
      (uint8_t *)BANK3_FILE_ENTRIES_START + n * FILE_ENTRY_SIZE;
  put_bank3(ptr, &type, 1);
}

bool FileBrowserPage::can_show_parent_entry() const {
  return show_parent && strcmp(lwd, "/") != 0;
}

uint8_t FileBrowserPage::entry_type_for_dir(const char *entry) {
  (void)entry;
  return DIR_TYPE;
}

bool FileBrowserPage::add_entry(const char *entry, uint8_t type) {
  if (numEntries >= (int)NUM_FILE_ENTRIES) {
    return false;
  }
  char buf[FILE_ENTRY_SIZE];
  buf[0] = type;
  strncpy(buf + 1, entry, FILE_ENTRY_SIZE - 1);
  buf[FILE_ENTRY_SIZE - 1] = '\0';
  volatile uint8_t *ptr =
      (uint8_t *)BANK3_FILE_ENTRIES_START + numEntries * FILE_ENTRY_SIZE;
  put_bank3(ptr, buf, sizeof(buf));
  numEntries++;
  return true;
}

void FileBrowserPage::query_filesystem() {

  char temp_entry[FILE_ENTRY_SIZE];
  // config menu
  file_menu_page.visible_rows = 3;
  uint16_t disabled = FM_MASK(FM_RECVALL) | FM_MASK(FM_SENDALL);
#ifdef MCL_HAS_FILE_MOVE
  if (move_destination_mode) {
    disabled |= FM_MASK(FM_DELETE) | FM_MASK(FM_RENAME);
  }
#else
  disabled |= FM_MASK(FM_MOVE);
#endif
  if (!show_new_folder) {
    disabled |= FM_MASK(FM_NEW_FOLDER);
  }
  if (!show_copy) {
    disabled |= FM_MASK(FM_DUPLICATE);
  }
  if (!show_move) {
    disabled |= FM_MASK(FM_MOVE);
  }
  if (!show_versions) {
    disabled |= FM_MASK(FM_VERSIONS);
  }
  set_file_menu_disabled_mask(disabled);

  file_menu_encoder.cur = file_menu_encoder.old = 0;
  file_menu_encoder.max = file_menu_page.menu.get_number_of_items() - 1;

  DEBUG_PRINTLN("query");
  DEBUG_PRINTLN(lwd);
  DEBUG_PRINTLN(file_menu_encoder.max);

  //  reset directory pointer
  File d;
  d.open(lwd);
  d.rewind();
  numEntries = 0;
  cur_file = 255;

  if (show_save) {
    add_entry(&str_save[0]);
  }
  if (can_show_parent_entry()) {
    add_entry("..", DIR_TYPE);
  }
  //  iterate through the files
  while (file.openNext(&d, O_READ) && (numEntries < MAX_ENTRIES)) {
    file.getName(temp_entry, FILE_ENTRY_SIZE);
    bool is_match_file = false;
    DEBUG_PRINTLN(numEntries);
    DEBUG_PRINTLN(temp_entry);
    uint8_t entry_type = FILE_TYPE;
    if (temp_entry[0] == '.') {
      is_match_file = false;
    } else if (file.isDirectory() && show_dirs) {
      entry_type = entry_type_for_dir(temp_entry);
      is_match_file = entry_type != SKIP_TYPE;
    } else {
      // XXX only 3char suffix
      size_t len = strlen(temp_entry);
      is_match_file =
          len >= 4 && file_types.compare(&temp_entry[len - 4]);
    }
    if (is_match_file && temp_entry[0] != '\0') {
      DEBUG_PRINTLN(F("file matched"));
      if (add_entry(temp_entry, entry_type)) {
        if (focus_match[0] != '\0' && strcmp(temp_entry, focus_match) == 0) {
          DEBUG_DUMP(temp_entry);
          DEBUG_DUMP(mcl_cfg.project);
          uint16_t focused_entry = numEntries - 1;
          cur_file = focused_entry;
          encoders[1]->cur = focused_entry;
          if ((uint16_t)cur_row > focused_entry) {
            cur_row = focused_entry;
          }
        }
      }
    }
    file.close();
  }

  if (numEntries <= 0) {
    numEntries = 0;
    ((MCLEncoder *)encoders[1])->max = 0;
  } else {
    ((MCLEncoder *)encoders[1])->max = numEntries - 1;
  }
  DEBUG_PRINTLN(F("finished list files"));
}

void FileBrowserPage::reset_browser_options() {
  filemenu_active = false;
  show_samplemgr = false;
  show_copy = false;
  show_move = false;
  show_versions = false;
  draw_dirs = false;
}

void FileBrowserPage::init() {
  reset_browser_options();
  focus_match[0] = '\0';
  strcpy_P(title, mclstr_title_files);
  file_types.reset();
  SD.chdir(lwd);
#ifdef MCL_HAS_FILE_MOVE
  if (move_destination_mode) {
    show_dirs = true;
    select_dirs = false;
    show_save = true;
    show_parent = true;
    show_new_folder = true;
    show_filemenu = true;
    draw_dirs = true;
    strcpy(title, "DEST");
    strcpy(str_save, "[ MOVE ]");
  }
#endif
}

void FileBrowserPage::draw_menu() {
  oled_display.fillRect(0, 3, 45, 29, BLACK);
  oled_display.drawRect(1, 4, 43, 27, WHITE);
  file_menu_page.draw_menu(6, 12, 34, 2);
  oled_display.display();
}

void FileBrowserPage::open_filemenu() {
  filemenu_active = true;
  file_menu_page.select_item(0);
  encoders[0] = file_menu_page.encoders[0];
  encoders[1] = file_menu_page.encoders[1];
  file_menu_page.init();
}

void FileBrowserPage::draw_sidebar() {
  constexpr uint8_t x_offset = 43;
  oled_display.clearDisplay();
  oled_display.setCursor(0, 8);
  oled_display.setTextColor(WHITE, BLACK);
  oled_display.println(title);
  mcl_gui.draw_vertical_dashline(x_offset - 6);
}

void FileBrowserPage::draw_filebrowser() {

  constexpr uint8_t x_offset = 43, y_offset = 8, width = MENU_WIDTH;
  oled_display.setCursor(x_offset, 8);
  uint8_t max_items = min(MAX_VISIBLE_ROWS, numEntries);
  uint16_t first_entry = encoders[1]->cur - cur_row;

  char temp_entry[FILE_ENTRY_SIZE];

  for (uint8_t n = 0; n < max_items; n++) {
    uint8_t y_pos = y_offset + 8 * n;
    oled_display.setCursor(x_offset, y_pos);
    bool color = n == cur_row;
    if (color) {
      oled_display.setTextColor(BLACK, WHITE);
      oled_display.fillRect(oled_display.getCursorX() - 3,
                            oled_display.getCursorY() - 6, width, 7, WHITE);
    } else {
      oled_display.setTextColor(WHITE, BLACK);
    }
    uint16_t entry_num = first_entry + n;
    if (entry_num == cur_file) {
      oled_display.setCursor(x_offset - 4, y_pos);
      mcl_print_P(mclstr_arrow_right);
    }
    if (entry_num < (uint16_t)numEntries) {
      uint8_t type;
      get_entry(entry_num, temp_entry, type);
      type = resolve_entry_type(entry_num, temp_entry, type);
      if (type == DIR_TYPE && draw_dirs) {
        oled_display.drawRect(x_offset, y_pos - 4, 6, 4, !color);
        oled_display.drawFastHLine(x_offset + 1, y_pos - 1 - 4, 3, !color);
        oled_display.setCursor(x_offset + 8, y_pos);
      }
      oled_display.println(temp_entry);
    }
  }
  if (numEntries > MAX_VISIBLE_ROWS) {
    draw_scrollbar(120);
  }
}

void FileBrowserPage::display() {
  oled_display.setFont(&TomThumb);
  if (filemenu_active) {
    draw_menu();
    return;
  }
  draw_sidebar();
  draw_filebrowser();
  return;
}

void FileBrowserPage::draw_scrollbar(uint8_t x_offset) {
  mcl_gui.draw_vertical_scrollbar(x_offset, numEntries, MAX_VISIBLE_ROWS,
                                  encoders[1]->cur - cur_row);
}

void FileBrowserPage::loop() {
  if (filemenu_active) {
    file_menu_page.loop();
    return;
  }

  if (encoders[1]->hasChanged()) {
    selection_change = true;
    selection_change_clock = read_clock_ms();
    uint8_t diff = encoders[1]->cur - encoders[1]->old;
    int8_t new_val = cur_row + diff;

    if (new_val > MAX_VISIBLE_ROWS - 1) {
      new_val = MAX_VISIBLE_ROWS - 1;
    }
    if (new_val < 0) {
      new_val = 0;
    }
    // MD.assignMachine(0, encoders[1]->cur);
    cur_row = new_val;
  }
}

bool FileBrowserPage::create_folder() {
  char new_dir[17] = "new_folder      ";
  if (mcl_gui.wait_for_input(new_dir, "Create Folder", NAME_LENGTH)) {
    SD.mkdir(new_dir);
    init();
  }
  return true;
}

void FileBrowserPage::_calcindices(int &saveidx) {
  saveidx = show_save ? 0 : -1;
}

void FileBrowserPage::_cd_up() {
  file.close();
  DEBUG_PRINTLN("cd_up");
  // don't cd up if we are at the root
#ifndef __AVR__
  if (mcl_sd.mcl_root[0] == '\0') {
    if (strlen(lwd) < 2) {
      init();
      return;
    }
  } else {
    if (strcmp(lwd, mcl_sd.mcl_root) == 0) {
      init();
      return;
    }
  }
#else
  if (strlen(lwd) < 2) {
    init();
    return;
  }
#endif

  // trim ending '/'
  auto len_lwd = strlen(lwd);
  if (lwd[len_lwd - 1] == '/') {
    lwd[--len_lwd] = '\0';
  }

  // find parent path separator and trim it
  for (int i = len_lwd - 1; i >= 0; --i) {
    if (lwd[i] == '/') {
      lwd[i] = '\0';
      break;
    }
  }

  // in case root is trimmed, add it back
  if (lwd[0] == '\0') {
#ifndef __AVR__
    if (mcl_sd.mcl_root[0] == '\0') {
      strcpy_P(lwd, mclstr_root_path);
    } else {
      strcpy(lwd, mcl_sd.mcl_root);
    }
#else
    strcpy_P(lwd, mclstr_root_path);
#endif
  }
  DEBUG_PRINTLN(lwd);
  if (!SD.chdir(lwd)) {
    DEBUG_PRINTLN("bad directory change");
  }
  init();
  uint16_t pos = 0;
  uint8_t row = 0;
  position.pop(pos, row);
  encoders[1]->cur = pos;
  encoders[1]->old = pos;
  cur_row = row;
}

bool FileBrowserPage::_cd(const char *child) {
  DEBUG_PRINTLN("_cd");
  DEBUG_PRINTLN(child);

  DEBUG_PRINTLN(lwd);

  file.close();
#ifndef __AVR__
  char rooted_child[sizeof(lwd)];
  if (mcl_sd.mcl_root[0] != '\0' && child[0] == '/') {
    child = mcl_sd.full_path(child, rooted_child, sizeof(rooted_child));
  }
#endif
  size_t len_lwd = strlen(lwd);
  size_t child_len = strlen(child);
  bool absolute_child = child[0] == '/';
  size_t new_len = child_len;
  if (!absolute_child) {
    new_len += len_lwd;
    if (len_lwd > 0 && lwd[len_lwd - 1] != '/') {
      new_len++;
    }
  }
  if (new_len >= sizeof(lwd)) {
    DEBUG_PRINTLN(F("path too long"));
    gfx.alert_error("BAD PATH");
    return false;
  }
  const char *ptr = child;

  if (child[0] == '/' && child[1] != '\0') {
    SD.chdir("/");
    ptr++;
  }

  if (!SD.chdir(ptr)) {
    DEBUG_PRINTLN("cd failed");
    init();
    return false;
  }

  if (absolute_child) {
    strcpy(lwd, child);
  } else {
    if (len_lwd > 0 && lwd[len_lwd - 1] != '/') {
      lwd[len_lwd++] = '/';
      lwd[len_lwd] = '\0';
    }
    strcpy(lwd + len_lwd, child);
  }
  DEBUG_PRINTLN(lwd);
  uint16_t pos = encoders[1]->getValue();
  uint8_t row = cur_row;
  init();
  encoders[1]->cur = 1;
  position.push(pos, row);
  return true;
}

bool FileBrowserPage::_handle_filemenu() {
  char buf1[FILE_ENTRY_SIZE];

  get_entry(encoders[1]->getValue(), buf1);

  char *suffix_pos = strchr(buf1, '.');
  char buf2[32];
  uint8_t name_length = NAME_LENGTH;
  bool copy_entry = false;
  const char *input_prompt = nullptr;

  switch (file_menu_page.menu.get_item_index(file_menu_encoder.cur)) {
  case FM_NEW_FOLDER: // new folder
    create_folder();
    return true;
  case FM_DELETE: // delete
    build_delete_message(buf2, sizeof(buf2), buf1);
    if (mcl_gui.wait_for_confirm("CONFIRM", buf2)) {
      on_delete(buf1);
    }
    return true;
  case FM_RENAME: // rename
    // trim the suffix is present, add back later
    strcpy(buf2, buf1);
    if (suffix_pos != nullptr) {
      buf2[suffix_pos - buf1] = '\0';
    }
    input_prompt = "RENAME TO:";
    break;
  case FM_DUPLICATE: // duplicate
    strcpy(buf2, buf1);
    if (suffix_pos != nullptr) {
      buf2[suffix_pos - buf1] = '\0';
    }
    {
      uint8_t suffix_len = suffix_pos == nullptr ? 0 : strlen(suffix_pos);
      uint8_t max_base = NAME_LENGTH > suffix_len + 2
                             ? NAME_LENGTH - suffix_len - 2
                             : 0;
      uint8_t base_len = strlen(buf2);
      if (base_len > max_base) {
        base_len = max_base;
        buf2[base_len] = '\0';
      }
      buf2[base_len++] = '_';
      buf2[base_len++] = '2';
      buf2[base_len] = '\0';
    }
    if (suffix_pos != nullptr) {
      strcat(buf2, suffix_pos);
    }
    copy_entry = true;
    input_prompt = "CLONE:";
    break;
#ifdef MCL_HAS_FILE_MOVE
  case FM_MOVE: // move
    enter_move_destination(buf1);
    return false;
#endif
  default:
    return false;
  }

  // default max length = NAME_LENGTH, can extend if the default is longer.
  name_length = filebrowser_name_length(name_length, buf2);
  if (mcl_gui.wait_for_input(buf2, input_prompt, name_length)) {
    if (!copy_entry && suffix_pos != nullptr) {
      // paste the suffix back
      strcat(buf2, suffix_pos);
    }
    if (copy_entry) {
      on_copy(buf1, buf2);
    } else {
      on_rename(buf1, buf2);
    }
  }
  return true;
}

#ifdef MCL_HAS_FILE_MOVE
bool FileBrowserPage::start_move_destination(const char *source_path) {
  if (source_path[0] == '\0') {
    gfx.alert_error("BAD PATH");
    return false;
  }
  strncpy(move_source_path, source_path, sizeof(move_source_path) - 1);
  move_source_path[sizeof(move_source_path) - 1] = '\0';
  move_destination_mode = true;
  position.reset();
  init();
  return true;
}

bool FileBrowserPage::enter_move_destination(const char *entry) {
  char source_path[PRJ_PATH_LEN];
  if (!MCLSd::join_path(source_path, sizeof(source_path), lwd, entry)) {
    gfx.alert_error("BAD PATH");
    return false;
  }
  return start_move_destination(source_path);
}

bool FileBrowserPage::finish_move_to_path(const char *dest_path) {
  if (!move_destination_mode) {
    return false;
  }
  if (path_starts_with_dir(dest_path, move_source_path)) {
    gfx.alert_error("BAD DEST");
    return false;
  }
  if (SD.exists(dest_path)) {
    gfx.alert_error("EXISTS");
    return false;
  }
  if (!SD.rename(move_source_path, dest_path)) {
    gfx.alert_error("Not moved.");
    return false;
  }
  move_destination_mode = false;
  move_source_path[0] = '\0';
  gfx.alert_success("Moved.");
  init();
  return true;
}

bool FileBrowserPage::move_to_current_folder() {
  const char *name = strrchr(move_source_path, '/');
  name = name == nullptr ? move_source_path : name + 1;

  char dest_path[PRJ_PATH_LEN];
  if (!MCLSd::join_path(dest_path, sizeof(dest_path), lwd, name)) {
    gfx.alert_error("BAD PATH");
    return false;
  }
  return finish_move_to_path(dest_path);
}

void FileBrowserPage::cancel_move_destination() {
  move_destination_mode = false;
  move_source_path[0] = '\0';
  init();
}
#endif

#ifdef PLATFORM_TBD
bool FileBrowserPage::tbd_can_cd_up() const {
  if (!show_parent) return false;
#ifndef __AVR__
  if (mcl_sd.mcl_root[0] == '\0') {
    return strlen(lwd) >= 2;
  }
  return strcmp(lwd, mcl_sd.mcl_root) != 0;
#else
  return strlen(lwd) >= 2;
#endif
}

bool FileBrowserPage::tbd_cd_up() {
  if (tbd_can_cd_up()) {
    _cd_up();
  }
  return true;
}
#endif

bool FileBrowserPage::rm_dir(const char *dir) {
  return mcl_sd.remove_dir(dir);
}

void FileBrowserPage::on_delete(const char *entry) {
  file.open(entry, O_READ);
  bool dir = file.isDirectory();
  file.close();
  if (dir) {
    if (rm_dir(entry)) {
      gfx.alert_success("Folder removed.");
    } else {
      gfx.alert_error("Folder not removed.");
    }
  } else {
    if (SD.remove(entry)) {
      gfx.alert_success("File removed.");
    } else {
      gfx.alert_error("File not removed.");
    }
  }
}

void FileBrowserPage::on_rename(const char *from, const char *to) {
  if (SD.rename(from, to)) {
    gfx.alert_success("File renamed.");
  } else {
    gfx.alert_error("File not renamed.");
  }
}

void FileBrowserPage::on_copy(const char *from, const char *to) {
#ifdef __AVR__
  mcl_gui.draw_progress("CLONE", 0, 64);
  bool ok = mcl_sd.copy_file(from, to, 0, 64, 64);
#else
  file.open(from, O_READ);
  bool dir = file.isDirectory();
  file.close();
  bool ok = false;
  if (dir) {
    mcl_gui.draw_progress("CLONE", 0, 64);
    ok = mcl_sd.copy_dir(from, to, 0, 64, 64);
  } else {
    mcl_gui.draw_progress("CLONE", 0, 64);
    ok = mcl_sd.copy_file(from, to, 0, 64, 64);
  }
#endif
  if (ok) {
    gfx.alert_success("Cloned.");
  } else {
    gfx.alert_error("Not cloned.");
  }
}

bool FileBrowserPage::handleEvent(gui_event_t *event) {
  if (EVENT_CMD(event)) {
    uint8_t key = event->source;
    if (event->mask == EVENT_BUTTON_PRESSED) {
      uint8_t inc = 1;
      if (key_interface.is_key_down(MDX_KEY_FUNC)) {
        inc = 8;
      }
      switch (key) {
      case MDX_KEY_YES:
        key_interface.ignoreNextEvent(MDX_KEY_YES);
        goto YES;
      case MDX_KEY_NO:
        key_interface.ignoreNextEvent(MDX_KEY_NO);
        goto NO;
      case MDX_KEY_UP:
        encoders[1]->cur -= inc;
        return true;
      case MDX_KEY_DOWN:
        encoders[1]->cur += inc;
        return true;
        /*
        case MDX_KEY_LEFT:
          _cd_up();
          break;
        case MDX_KEY_RIGHT:
          dir_only = true;
          goto YES;
        */
      }
    }
  }
  if (EVENT_BUTTON(event)) {
    if (EVENT_PRESSED(event, Buttons.BUTTON3) && show_filemenu) {
#ifdef __AVR__
      bool can_clone = false;
      if (show_copy && encoders[1]->getValue() < numEntries) {
        char entry[FILE_ENTRY_SIZE];
        uint8_t entry_type;
        get_entry(encoders[1]->getValue(), entry, entry_type);
        can_clone = strcmp(entry, "..") != 0 && entry_type == FILE_TYPE;
      }
      file_menu_page.menu.enable_entry(FM_DUPLICATE, can_clone);
#endif
      open_filemenu();
      return true;
    }
    if (EVENT_RELEASED(event, Buttons.BUTTON3) && filemenu_active) {
      encoders[0] = param1;
      encoders[1] = param2;

      filemenu_active = false;
      if (_handle_filemenu()) {
        init();
        return true;
      }
      selection_change = true;
      return true;
    }

    if (EVENT_PRESSED(event, Buttons.BUTTON4)) {
      GUI.ignoreNextEvent(event->source);
    YES:
      int i_save;
      _calcindices(i_save);

      char temp_entry[FILE_ENTRY_SIZE];
      uint8_t entry_type;
      get_entry(encoders[1]->getValue(), temp_entry, entry_type);

      if (!show_samplemgr) {
        if (encoders[1]->getValue() == i_save) {
          on_new();
          goto fin;
        }

        // chdir to parent
        if ((temp_entry[0] == '.') && (temp_entry[1] == '.')) {
          _cd_up();
          goto fin;
        }
        //  }
        // chdir to child
        if (!select_dirs && entry_type == DIR_TYPE) {
          _cd(temp_entry);
          goto fin;
        }
        file.open(temp_entry, O_READ);
      }

      GUI.ignoreNextEvent(event->source);
      on_select(temp_entry);
    fin:
      // file.close();
      return true;
    }

    // cancel
    if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
      GUI.ignoreNextEvent(event->source);
    NO:
      on_cancel();
      return true;
    }
  }
  return false;
}
