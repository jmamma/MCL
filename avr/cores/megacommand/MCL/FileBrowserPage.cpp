#include "FileBrowserPage.h"
#include "MCL.h"

void FileBrowserPage::setup() {
  bool ret;
  int b;
#ifdef OLED_DISPLAY
  oled_display.clearDisplay();
  classic_display = false;
  //char *mcl = ".mcl";
  //strcpy(match, mcl);
#endif
  char *files = "Files";
  strcpy(title, files);
  DEBUG_PRINT_FN();
}

void FileBrowserPage::add_entry(char *entry) {
      uint32_t pos = FILE_ENTRIES_START + numEntries * 16;
      volatile uint8_t *ptr = pos;
      switch_ram_bank(1);
      memcpy(ptr, &entry[0], 16);
      switch_ram_bank(0);
      numEntries++;
}

void FileBrowserPage::init() {
  DEBUG_PRINT_FN();
  char temp_entry[16];

  int index = 0;
  // file.open("/",O_READ);
  SD.vwd()->rewind();
  numEntries = 0;
  cur_file = 255;
  if (show_save) {
  char create_new[9] = "[ SAVE ]";
  add_entry(&create_new[0]);
  }

  if (show_new_folder) {
  char folder[16] = "[ NEW FOLDER ]";
  add_entry(&folder[0]);
  }

  char up_one_dir[3] = "..";
  SD.vwd()->getName(temp_entry, 16);
  if ((show_parent) && (strlen(temp_entry) > 1)) {
    add_entry(&up_one_dir[0]);
  }

  encoders[1]->cur = 0;

  while (file.openNext(SD.vwd(), O_READ) && (numEntries < MAX_ENTRIES)) {
    for (uint8_t c = 0; c < 16; c++) {
      temp_entry[c] = 0;
    }
    file.getName(temp_entry, 16);
    bool is_match_file = false;
    //    if (dir_browser == true) {
    DEBUG_PRINTLN(temp_entry);
    if (temp_entry[0] == '.') {
      is_match_file = false;
    } else if (file.isDirectory() && dir_browser) {
      is_match_file = true;
    } else {
      //  } else {
     char *arg1 =  &temp_entry[strlen(temp_entry) - 4];
    DEBUG_PRINTLN(arg1);
             if (strcmp(arg1,match) == 0) {
        is_match_file = true;
        }
    }
    //  }
    if (is_match_file) {
     DEBUG_PRINTLN("project file identified");
      add_entry(&temp_entry[0]);
      if (strcmp(&temp_entry[0], &mcl_cfg.project[0]) == 0) {
        DEBUG_PRINTLN("match");
        DEBUG_PRINTLN(temp_entry);
        DEBUG_PRINTLN(mcl_cfg.project);

        cur_file = numEntries;
        encoders[1]->cur = numEntries;
      }

    }
    index++;
    file.close();
    DEBUG_PRINTLN(numEntries);
  }

  if (numEntries <= 0) {
    numEntries = 0;
    ((MCLEncoder *)encoders[1])->max = 0;
  }
  ((MCLEncoder *)encoders[1])->max = numEntries - 1;
  DEBUG_PRINTLN("finished load proj setup");
}

void FileBrowserPage::display() {
#ifdef OLED_DISPLAY
  uint8_t x_offset = 43, y_offset = 8, width = MENU_WIDTH;
  oled_display.clearDisplay();
  oled_display.setFont(&TomThumb);
  oled_display.setCursor(0, 8);
  oled_display.setTextColor(WHITE, BLACK);
  oled_display.println(title);
  for (uint8_t n = 0; n < 32; n++) {
    if (n % 2 != 0) {
      oled_display.drawPixel(x_offset - 6, n, WHITE);
    }
  }

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
    uint32_t pos = FILE_ENTRIES_START + entry_num * 16;
    volatile uint8_t *ptr = pos;
    switch_ram_bank(1);
    memcpy(&temp_entry[0], ptr, 16);
    switch_ram_bank(0);
    oled_display.println(temp_entry);
  }
  if (numEntries > MAX_VISIBLE_ROWS) {
    draw_scrollbar(120);
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
  char temp_entry[16];
  uint16_t entry_num = encoders[1]->cur;
  uint32_t pos = FILE_ENTRIES_START + entry_num * 16;
  volatile uint8_t *ptr;
  switch_ram_bank(1);
  strcpy(&temp_entry[0], ptr);
  switch_ram_bank(0);

  GUI.put_string_at_fill(1, temp_entry);

#endif
  return;
}

void FileBrowserPage::draw_scrollbar(uint8_t x_offset) {
  uint8_t number_of_items = numEntries;
  uint8_t length = round(
      ((float)(MAX_VISIBLE_ROWS - 1) / (float)(number_of_items - 1)) * 32);
  uint8_t y = round(
      ((float)(encoders[1]->cur - cur_row) / (float)(number_of_items - 1)) *
      32);
  for (uint8_t n = 0; n < 32; n++) {
    if (n % 2 == 0) {
      oled_display.drawPixel(x_offset + 1, n, WHITE);
      oled_display.drawPixel(x_offset + 3, n, WHITE);

    } else {
      oled_display.drawPixel(x_offset + 2, n, WHITE);
    }
  }

  oled_display.fillRect(x_offset + 1, y + 1, 3, length - 2, BLACK);
  oled_display.drawRect(x_offset, y, 5, length, WHITE);
}

void FileBrowserPage::loop() {

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
}

bool FileBrowserPage::create_folder() {
    char *my_title = "Create Folder";
    char new_dir[17] = "new_folder      ";
    if (mcl_gui.wait_for_input(new_dir, my_title, 8)) {
      for (uint8_t n = 0; n < strlen(new_dir); n++) {
        if (new_dir[n] == ' ') {
          new_dir[n] = '\0';
        }
      }
      SD.mkdir(new_dir);
      init();
    }
   return true;
}
bool FileBrowserPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {

    return true;
  }
  if (EVENT_PRESSED(event, Buttons.ENCODER1) ||
      EVENT_PRESSED(event, Buttons.ENCODER2) ||
      EVENT_PRESSED(event, Buttons.ENCODER3) ||
      EVENT_PRESSED(event, Buttons.ENCODER4)) {

    if (encoders[0]->getValue() == 0) {
     return;
    }

    if (encoders[1]->getValue() == 1) {
      create_folder();
      return;
    }

    char temp_entry[16];
    char dir_entry[16];
    uint32_t pos = FILE_ENTRIES_START + encoders[1]->getValue() * 16;
    volatile uint8_t *ptr = pos;
    switch_ram_bank(1);
    memcpy(&temp_entry[0], ptr, 16);
    switch_ram_bank(0);
    char *up_one_dir = "..";
    if ((temp_entry[0] == '.') && (temp_entry[1] == '.')) {
      /*
            SD.vwd()->getName(temp_entry,16);
            DEBUG_PRINTLN(temp_entry);

            file.openParent(SD.vwd());
            file.getName(temp_entry,16);

          // SD.chdir(temp_entry);
      */
      file.close();
      SD.chdir(lwd);

      SD.vwd()->getName(dir_entry, 16);
      lwd[strlen(lwd) - strlen(dir_entry) - 1] = '\0';
      DEBUG_PRINTLN(lwd);

      init();
      return;
      SD.vwd()->getName(temp_entry, 16);
      DEBUG_PRINTLN(temp_entry);
      return;
    }
    file.open(temp_entry, O_READ);
    if (file.isDirectory()) {
      file.close();
      SD.vwd()->getName(dir_entry, 16);
      strcat(lwd, dir_entry);
      if (dir_entry[strlen(dir_entry) - 1] != '/') {
        char *slash = "/";
        strcat(lwd, slash);
      }
      DEBUG_PRINTLN(lwd);
      DEBUG_PRINTLN(temp_entry);
      SD.chdir(temp_entry);
      init();
      return;
    }

    GUI.popPage();
    /*
    #ifdef OLED_DISPLAY
            oled_display.clearDisplay();
            oled_display.setFont(&TomThumb);
            oled_display.setCursor(0, 8);
            oled_display.setTextColor(WHITE, BLACK);
            oled_display.println("PROJECT NOT COMPATIBLE");
            oled_display.display();
            delay(700);
    #else
            GUI.flash_strings_fill("PROJECT ERROR", "NOT COMPATIBLE");
    #endif
    */
    return true;
  }
  if (EVENT_RELEASED(event, Buttons.BUTTON2)) {
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON1) ||
      EVENT_RELEASED(event, Buttons.BUTTON3) ||
      EVENT_PRESSED(event, Buttons.BUTTON4)) {
      GUI.setPage(&grid_page);
      return true;
  }
  return false;
}

