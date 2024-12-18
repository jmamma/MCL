#include "memory.h"
#include "NoteInterface.h"
#include "helpers.h"
#include "FileBrowserPage.h"
#include "MCLSD.h"
#include "MCLGFX.h"
#include "MCLMenus.h"
#include "MCLGUI.h"
File FileBrowserPage::file;
int FileBrowserPage::numEntries;

char FileBrowserPage::title[12];
char FileBrowserPage::str_save[12];
char FileBrowserPage::focus_match[14];
uint8_t FileBrowserPage::cur_file = 0;

bool FileBrowserPage::draw_dirs = false;
bool FileBrowserPage::show_dirs = false;
bool FileBrowserPage::select_dirs = false;
bool FileBrowserPage::show_save = true;
bool FileBrowserPage::show_parent = true;
bool FileBrowserPage::show_new_folder = true;
bool FileBrowserPage::show_filemenu = true;
bool FileBrowserPage::show_overwrite = false;

bool FileBrowserPage::show_samplemgr = false;

bool FileBrowserPage::filemenu_active = false;

bool FileBrowserPage::call_handle_filemenu = false;

FileBrowserFileTypes FileBrowserPage::file_types;

bool FileBrowserPage::selection_change = false;

uint16_t FileBrowserPage::selection_change_clock = 0;

void FileBrowserPage::cleanup() {
  // always call setup() when entering this page.
//  this->isSetup = false;
}

void FileBrowserPage::setup() {
  oled_display.clearDisplay();
  // char *mcl = ".mcl";
  // strcpy(match, mcl);
  strcpy(title, "Files");
  SD.chdir();
  strcpy(lwd, "/");

  encoders[1]->cur = 1;
  encoders[2]->cur = 1;
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

bool FileBrowserPage::add_entry(const char *entry, uint8_t type) {
  if (numEntries >= NUM_FILE_ENTRIES) {
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
  call_handle_filemenu = false;
  // config menu
  file_menu_page.visible_rows = 3;
  file_menu_page.menu.enable_entry(FM_NEW_FOLDER, show_new_folder);
  file_menu_page.menu.enable_entry(FM_DELETE, true); // delete
  file_menu_page.menu.enable_entry(FM_RENAME, true); // rename
  file_menu_page.menu.enable_entry(FM_CANCEL, true); // cancel
  file_menu_page.menu.enable_entry(FM_RECVALL, false);
  file_menu_page.menu.enable_entry(FM_SENDALL, false);

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
  // SD.vwd()->getName(temp_entry, FILE_ENTRY_SIZE);
  // SD.vwd()->getName(temp_entry, FILE_ENTRY_SIZE);
  file.getName(temp_entry, FILE_ENTRY_SIZE);

  if ((show_parent) && !(strcmp(lwd, "/") == 0)) {
    add_entry("..");
  }
  //  iterate through the files
  while (file.openNext(&d, O_READ) && (numEntries < MAX_ENTRIES)) {
    memset(temp_entry, 0, sizeof(temp_entry));
    file.getName(temp_entry, FILE_ENTRY_SIZE);
    bool is_match_file = false;
    DEBUG_PRINTLN(numEntries);
    DEBUG_PRINTLN(temp_entry);
    bool is_dir = false;
    if (temp_entry[0] == '.') {
      is_match_file = false;
    } else if (file.isDirectory() && show_dirs) {
      is_dir = true;
      is_match_file = true;
    } else {
      // XXX only 3char suffix
      is_match_file = file_types.compare(&temp_entry[strlen(temp_entry) - 4]);
    }
    if (is_match_file && (strlen(temp_entry) > 0)) {
      DEBUG_PRINTLN(F("file matched"));
      if (add_entry(temp_entry,is_dir)) {
        if (strlen(focus_match) > 0 && strcmp(temp_entry, focus_match) == 0) {
          DEBUG_DUMP(temp_entry);
          DEBUG_DUMP(mcl_cfg.project);
          cur_file = numEntries - 1;
          encoders[1]->cur = numEntries - 1;
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

void FileBrowserPage::init() {
  filemenu_active = false;
  show_samplemgr = false;
  draw_dirs = false;
  strcpy(focus_match, "");
  file_types.reset();
  SD.chdir(lwd);
}

void FileBrowserPage::draw_menu() {
  oled_display.fillRect(0, 3, 45, 29, BLACK);
  oled_display.drawRect(1, 4, 43, 27, WHITE);
  file_menu_page.draw_menu(6, 12, 39);
  oled_display.display();
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
  uint8_t max_items = min(MAX_VISIBLE_ROWS,numEntries);

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
    if (encoders[1]->cur - cur_row + n == cur_file) {
      oled_display.setCursor(x_offset - 4, y_pos);
      oled_display.print(F(">"));
    }
    uint16_t entry_num = encoders[1]->cur - cur_row + n;
    if (entry_num < numEntries) {
      uint8_t type;
      get_entry(entry_num, temp_entry, type);
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
    selection_change_clock = g_clock_ms;
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
  auto len_lwd = strlen(lwd);
  if (len_lwd < 2) {
    init();
    return;
  }

  // trim ending '/'
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
    strcpy(lwd, "/");
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

  uint8_t len_lwd = strlen(lwd);

  if (len_lwd == 1) {
    lwd[0] = '\0';
  }

  if (child[0] != '/') {
    if (lwd[len_lwd] != '/') {
      strcat(lwd, "/");
      len_lwd++;
    }
  } else {
    lwd[0] = '\0';
  }
  strcat(lwd, child);
  DEBUG_PRINTLN(lwd);
  uint16_t pos = encoders[1]->getValue();
  uint8_t row = cur_row;
  init();
  encoders[1]->cur = 1;
  encoders[2]->cur = 1;
  position.push(pos, row);
  return true;
}

bool FileBrowserPage::_handle_filemenu() {
  char buf1[FILE_ENTRY_SIZE];

  get_entry(encoders[1]->getValue(), buf1);

  char *suffix_pos = strchr(buf1, '.');
  char buf2[32] = {'\0'};
  for (uint8_t n = 1; n < 32; n++) {
    buf2[n] = ' ';
  }
  uint8_t name_length = NAME_LENGTH;

  switch (file_menu_page.menu.get_item_index(file_menu_encoder.cur)) {
  case FM_NEW_FOLDER: // new folder
    create_folder();
    return true;
  case FM_DELETE: // delete
    strcpy(buf2, "Delete ");
    strcat(buf2, buf1);
    strcat(buf2, "?");
    if (mcl_gui.wait_for_confirm("CONFIRM", buf2)) {
      on_delete(buf1);
    }
    return true;
  case FM_RENAME: // rename
    // trim the suffix is present, add back later
    strcat(buf2, buf1);
    if (suffix_pos != nullptr) {
      buf2[suffix_pos - buf1] = '\0';
    }
    // default max length = NAME_LENGTH, can extend if buf2 without suffix
    // is longer than NAME_LENGTH.
    name_length = max(name_length, strlen(buf2));
    if (mcl_gui.wait_for_input(buf2, "RENAME TO:", name_length)) {
      if (suffix_pos != nullptr) {
        // paste the suffix back
        strcat(buf2, suffix_pos);
      }
      on_rename(buf1, buf2);
    }
    return true;
  }
  return false;
}

bool FileBrowserPage::rm_dir(const char *dir) {
  char temp_entry[FILE_ENTRY_SIZE];

  // SD.vwd()->getName(temp_entry, FILE_ENTRY_SIZE);
  DEBUG_PRINTLN("preparing to delete");
  DEBUG_PRINTLN(dir);
  if (_cd(dir)) {
    File d;
    d.open(lwd);
    d.rewind();
    // bool ret = SD.vwd()->rmRfStar(); // extra 276 bytes
    while (file.openNext(&d, O_READ)) {
      file.getName(temp_entry, FILE_ENTRY_SIZE);
      DEBUG_PRINT("deleting ");
      DEBUG_PRINTLN(temp_entry);
      file.close();
      SD.remove(temp_entry);
    }
    SD.chdir("/");
    _cd_up();
    return SD.rmdir(dir);
  }
  return false;
}

void FileBrowserPage::on_delete(const char *entry) {
  file.open(entry, O_READ);
  bool dir = file.isDirectory();
  file.close();
  if (dir) {
    if (rm_dir(entry)) {
      gfx.alert("SUCCESS", "Folder removed.");
    } else {
      gfx.alert("ERROR", "Folder not removed.");
    }
  } else {
    if (SD.remove(entry)) {
      gfx.alert("SUCCESS", "File removed.");
    } else {
      gfx.alert("ERROR", "File not removed.");
    }
  }
}

void FileBrowserPage::on_rename(const char *from, const char *to) {
  if (SD.rename(from, to)) {
    gfx.alert("SUCCESS", "File renamed.");
  } else {
    gfx.alert("ERROR", "File not renamed.");
  }
}

bool FileBrowserPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {
    return false;
  }
  //bool dir_only = false;

  if (EVENT_CMD(event)) {
    uint8_t key = event->source - 64;
    if (event->mask == EVENT_BUTTON_PRESSED) {
      uint8_t inc = 1;
      if (trig_interface.is_key_down(MDX_KEY_FUNC)) {
        inc = 8;
      }
      switch (key) {
      case MDX_KEY_YES:
        trig_interface.ignoreNextEvent(MDX_KEY_YES);
        goto YES;
      case MDX_KEY_NO:
        trig_interface.ignoreNextEvent(MDX_KEY_NO);
        goto NO;
      case MDX_KEY_UP:
        encoders[1]->cur -= inc;
        break;
      case MDX_KEY_DOWN:
        encoders[1]->cur += inc;
        break;
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

  if (EVENT_PRESSED(event, Buttons.BUTTON3) && show_filemenu) {
    filemenu_active = true;
    file_menu_encoder.cur = file_menu_encoder.old = 0;
    file_menu_page.cur_row = 0;
    encoders[0] = &config_param1;
    encoders[1] = &file_menu_encoder;
    file_menu_page.init();
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
    get_entry(encoders[1]->getValue(), temp_entry);

    if (!show_samplemgr) {
      file.open(temp_entry, O_READ);
  //    if (!dir_only) {

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
      if (!select_dirs && file.isDirectory()) {
        _cd(temp_entry);
        goto fin;
      }
    }

      GUI.ignoreNextEvent(event->source);
      on_select(temp_entry);
    fin:
    //file.close();
    return true;
  }

  // cancel
  if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
  GUI.ignoreNextEvent(event->source);
  NO:
    on_cancel();
    return true;
  }

  return false;
}
