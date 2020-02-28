#include "FileBrowserPage.h"
#include "MCL.h"
#include "MCLMenus.h"

void FileBrowserPage::setup() {
#ifdef OLED_DISPLAY
  oled_display.clearDisplay();
  classic_display = false;
  // char *mcl = ".mcl";
  // strcpy(match, mcl);
#endif
  strcpy(title, "Files");
  DEBUG_PRINT_FN();
}

void FileBrowserPage::add_entry(const char *entry) {
  char buf[16];
  m_strncpy(buf, entry, sizeof(buf));
  buf[15] = '\0';
  volatile uint8_t *ptr = (uint8_t *)BANK1_FILE_ENTRIES_START + numEntries * 16;
  memcpy_bank1(ptr, buf, sizeof(buf));
  numEntries++;
}

void FileBrowserPage::init() {
  DEBUG_PRINT_FN();

  if (show_filetypes) {
    if (filetype_idx > filetype_max) filetype_idx = filetype_max;
    if (filetype_idx < 0) filetype_idx = 0;
    strcpy(match, filetypes[filetype_idx]);
    ((MCLEncoder*)param1)->min = 0;
    ((MCLEncoder*)param1)->max = filetype_max;
  }

  char temp_entry[16];
  call_handle_filemenu = false;
  // config menu
  file_menu_page.visible_rows = 3;
  file_menu_page.menu.enable_entry(0, show_new_folder);
  file_menu_page.menu.enable_entry(1, true); // delete
  file_menu_page.menu.enable_entry(2, true); // rename
  file_menu_page.menu.enable_entry(3, show_overwrite);
  file_menu_page.menu.enable_entry(4, true); // cancel
  file_menu_encoder.cur = file_menu_encoder.old = 0;
  file_menu_encoder.max = file_menu_page.menu.get_number_of_items() - 1;
  filemenu_active = false;

  int index = 0;
  //  reset directory pointer
  SD.vwd()->rewind();
  numEntries = 0;
  cur_file = 255;
  if (show_save) {
    add_entry("[ SAVE ]");
  }

  SD.vwd()->getName(temp_entry, 16);
  DEBUG_DUMP(temp_entry);

  if ((show_parent) && !(strcmp(temp_entry, "/") == 0)) {
    add_entry("..");
  }
  encoders[1]->cur = 1;
  encoders[1]->old = 1;
  cur_row = 1;

  //  iterate through the files
  while (file.openNext(SD.vwd(), O_READ) && (numEntries < MAX_ENTRIES)) {
    for (uint8_t c = 0; c < 16; c++) {
      temp_entry[c] = 0;
    }
    file.getName(temp_entry, 16);
    bool is_match_file = false;
    DEBUG_DUMP(temp_entry);
    if (temp_entry[0] == '.') {
      is_match_file = false;
    } else if (file.isDirectory() && show_dirs) {
      is_match_file = true;
    } else {
      // XXX only 3char suffix
      char *arg1 = &temp_entry[strlen(temp_entry) - 4];
      DEBUG_DUMP(arg1);
      if (strcmp(arg1, match) == 0) {
        is_match_file = true;
      }
    }
    if (is_match_file) {
      DEBUG_PRINTLN("file matched");
      add_entry(temp_entry);
      if (strcmp(temp_entry, mcl_cfg.project) == 0) {
        DEBUG_DUMP(temp_entry);
        DEBUG_DUMP(mcl_cfg.project);

        cur_file = numEntries - 1;
        encoders[1]->cur = numEntries - 1;
      }
    }
    index++;
    file.close();
    DEBUG_DUMP(numEntries);
  }

  if (numEntries <= 0) {
    numEntries = 0;
    ((MCLEncoder *)encoders[1])->max = 0;
  }
  ((MCLEncoder *)encoders[1])->max = numEntries - 1;
  DEBUG_PRINTLN("finished list files");
}

void FileBrowserPage::display() {
#ifdef OLED_DISPLAY
  if (filemenu_active) {
    oled_display.fillRect(0, 3, 45, 28, BLACK);
    oled_display.drawRect(1, 4, 43, 26, WHITE);
    file_menu_page.draw_menu(6, 12, 39);
    oled_display.display();
    return;
  }

  constexpr uint8_t x_offset = 43, y_offset = 8, width = MENU_WIDTH;
  oled_display.clearDisplay();
  oled_display.setFont(&TomThumb);
  oled_display.setCursor(0, 8);
  oled_display.setTextColor(WHITE, BLACK);
  oled_display.println(title);
  mcl_gui.draw_vertical_dashline(x_offset - 6);

  oled_display.setCursor(x_offset, 8);
  uint8_t max_items;
  if (numEntries > MAX_VISIBLE_ROWS) {
    max_items = MAX_VISIBLE_ROWS;
  } else {
    max_items = numEntries;
  }
  for (uint8_t n = 0; n < max_items; n++) {

    oled_display.setCursor(x_offset, y_offset + 8 * n);
    if (n == cur_row) {
      oled_display.setTextColor(BLACK, WHITE);
      oled_display.fillRect(oled_display.getCursorX() - 3,
                            oled_display.getCursorY() - 6, width, 7, WHITE);
    } else {
      oled_display.setTextColor(WHITE, BLACK);
      if (encoders[1]->cur - cur_row + n == cur_file) {
        oled_display.setCursor(x_offset - 4, y_offset + n * 8);
        oled_display.print(">");
      }
    }
    char temp_entry[16];
    uint16_t entry_num = encoders[1]->cur - cur_row + n;
    volatile uint8_t *ptr =
        (uint8_t *)BANK1_FILE_ENTRIES_START + entry_num * 16;
    memcpy_bank1(temp_entry, ptr, 16);
    oled_display.println(temp_entry);
  }
  if (numEntries > MAX_VISIBLE_ROWS) {
    draw_scrollbar(120);
  }

  if (show_filetypes) {
    oled_display.setTextColor(WHITE, BLACK);
    for (int i = 0; i <= filetype_max; ++i) {
      oled_display.setCursor(2, 18 + i * 6);
      oled_display.println(filetype_names[i]);
    }
    oled_display.fillRect(0, 12 + filetype_idx * 6, 35, 7, INVERT);
  }

  oled_display.display();
#else
  GUI.setLine(GUI.LINE1);
  GUI.put_string_at(0, title);
  GUI.setLine(GUI.LINE2);
  if (cur_file == encoders[1]->cur) {
    GUI.put_string_at_fill(0, ">");
  } else {
    GUI.put_string_at_fill(0, " ");
  }
  char temp_entry[17];
  uint16_t entry_num = encoders[1]->cur;
  uint32_t pos = BANK1_FILE_ENTRIES_START + entry_num * 16;
  volatile uint8_t *ptr = pos;
  memcpy_bank1(temp_entry, ptr, 16);
  temp_entry[16] = '\0';
  GUI.put_string_at(1, temp_entry);

#endif
  return;
}

void FileBrowserPage::draw_scrollbar(uint8_t x_offset) {
#ifdef OLED_DISPLAY
  mcl_gui.draw_vertical_scrollbar(x_offset, numEntries, MAX_VISIBLE_ROWS,
                                  encoders[1]->cur - cur_row);
#endif
}

void FileBrowserPage::loop() {
#ifndef OLED_DISPLAY
  if (call_handle_filemenu) {
    call_handle_filemenu = false;
    _handle_filemenu();
  }
#endif

  if (filemenu_active) {
    file_menu_page.loop();
    return;
  }

  if (encoders[1]->hasChanged()) {

    uint8_t diff = encoders[1]->cur - encoders[1]->old;
    int8_t new_val = cur_row + diff;
#ifdef OLED_DISPLAY
    if (new_val > MAX_VISIBLE_ROWS - 1) {
      new_val = MAX_VISIBLE_ROWS - 1;
    }
    if (new_val < 0) {
      new_val = 0;
    }
#endif
    // MD.assignMachine(0, encoders[1]->cur);
    cur_row = new_val;
  }

  if (param1->hasChanged()) {
    if (show_filetypes) {
      filetype_idx = param1->cur;
      init();
    } else {
      // lock the value -- upper logic may disable it temporarily
      param1->cur = param1->old;
    }
  }
}

bool FileBrowserPage::create_folder() {
  char new_dir[17] = "new_folder      ";
  if (mcl_gui.wait_for_input(new_dir, "Create Folder", 8)) {
    SD.mkdir(new_dir);
    init();
  }
  return true;
}

void FileBrowserPage::_calcindices(int &saveidx) {
  saveidx = show_save ? 0 : -1;
}

void FileBrowserPage::_cd_up() {
  DEBUG_PRINT_FN();

  file.close();

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

  DEBUG_DUMP(lwd);

  SD.chdir(lwd);
  init();
}

void FileBrowserPage::_cd(const char *child) {
  file.close();
  if (!SD.chdir(child)) {
    gfx.alert("ERROR", "Failed to change dir.");
    init();
    return;
  }
  if (strcmp(lwd, "/") != 0) {
    strcat(lwd, "/");
  }
  strcat(lwd, child);
  auto len_lwd = strlen(lwd);
  // trim ending '/'
  if (lwd[len_lwd - 1] == '/') {
    lwd[--len_lwd] = '\0';
  }

  DEBUG_DUMP(lwd);
  DEBUG_DUMP(child);
  init();
}

void FileBrowserPage::_handle_filemenu() {
  char buf1[16];
  volatile uint8_t *ptr =
      (uint8_t *)BANK1_FILE_ENTRIES_START + encoders[1]->getValue() * 16;
  memcpy_bank1(&buf1[0], ptr, sizeof(buf1));

  char *suffix_pos = strchr(buf1, '.');
  char buf2[32] = {'\0'};
  for (uint8_t n = 1; n < 32; n++) {
    buf2[n] = ' ';
  }
  uint8_t name_length = 8;

  switch (file_menu_page.menu.get_item_index(file_menu_encoder.cur)) {
  case 0: // new folder
    create_folder();
    break;
  case 1: // delete
    sprintf(buf2, "Delete %s?", buf1);
    if (mcl_gui.wait_for_confirm("CONFIRM", buf2)) {
      on_delete(buf1);
    }
    break;
  case 2: // rename
    // trim the suffix is present, add back later
    strcat(buf2, buf1);
    if (suffix_pos != nullptr) {
      buf2[suffix_pos - buf1] = '\0';
    }
    // default max length = 8, can extend if buf2 without suffix
    // is longer than 8.
    name_length = max(name_length, strlen(buf2));
    if (mcl_gui.wait_for_input(buf2, "RENAME TO:", name_length)) {
      if (suffix_pos != nullptr) {
        // paste the suffix back
        strcat(buf2, suffix_pos);
      }
      on_rename(buf1, buf2);
    }
    break;
  case 3: // overwrite
    sprintf(buf2, "Overwrite %s?", buf1);
    if (mcl_gui.wait_for_confirm("CONFIRM", buf2)) {
      // the derived class may expect the file to be open
      // when on_select is called.
      file.open(buf1, O_READ);
      on_select(buf1);
    }
    break;
  }
}

void FileBrowserPage::on_delete(const char *entry) {
  file.open(entry, O_READ);
  bool dir = file.isDirectory();
  file.close();
  if (dir) {
    if (SD.rmdir(entry)) {
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

  DEBUG_PRINT_FN();

  if (note_interface.is_event(event)) {
    return false;
  }
#ifdef OLED_DISPLAY
  if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
    filemenu_active = true;
    file_menu_encoder.cur = file_menu_encoder.old = 0;
    file_menu_page.cur_row = 0;
    encoders[0] = &config_param1;
    encoders[1] = &file_menu_encoder;
    file_menu_page.init();
    return true;
  }
  if (EVENT_RELEASED(event, Buttons.BUTTON3)) {
    encoders[0] = param1;
    encoders[1] = param2;

    _handle_filemenu();
    init();
    return true;
  }
#else
  if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
    call_handle_filemenu = true;
    GUI.pushPage(&file_menu_page);
  }
#endif
  if (EVENT_PRESSED(event, Buttons.BUTTON4)) {

    int i_save;
    _calcindices(i_save);

    if (encoders[1]->getValue() == i_save) {
      on_new();
      return true;
    }

    char temp_entry[16];
    volatile uint8_t *ptr =
        (uint8_t *)BANK1_FILE_ENTRIES_START + encoders[1]->getValue() * 16;
    memcpy_bank1(temp_entry, ptr, 16);

    // chdir to parent
    if ((temp_entry[0] == '.') && (temp_entry[1] == '.')) {
      _cd_up();
      return true;
    }

    DEBUG_DUMP(temp_entry);
    file.open(temp_entry, O_READ);

    // chdir to child
    if (file.isDirectory()) {
      _cd(temp_entry);
      return true;
    }

    // select an entry
    GUI.ignoreNextEvent(event->source);
    on_select(temp_entry);
    return true;
  }

  // cancel
  if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
    on_cancel();
    return true;
  }

  return false;
}
