#include "LoadProjectPage.h"
#include "MCL.h"

void LoadProjectPage::display() {
#ifdef OLED_DISPLAY
  uint8_t x_offset = 43, y_offset = 8, width = MENU_WIDTH;
  oled_display.clearDisplay();
  oled_display.setFont(&TomThumb);
  oled_display.setCursor(0, 8);
  oled_display.setTextColor(WHITE, BLACK);
  oled_display.println("Projects");
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
      if (encoders[1]->cur - cur_row + n == cur_proj) {
        oled_display.setCursor(x_offset - 4, y_offset + n * 8);
        oled_display.print(">");
      }
    }
    char temp_entry[16];
    uint16_t entry_num = encoders[1]->cur - cur_row + n;
    uint32_t pos = FILE_ENTRIES_START + entry_num * 16;
    volatile uint8_t *ptr = pos;
    switch_ram_bank(1);
    memcpy(&temp_entry[0], ptr,16);
    switch_ram_bank(0);
    oled_display.println(temp_entry);
  }
  if (numEntries > MAX_VISIBLE_ROWS) {
    draw_scrollbar(120);
  }
  oled_display.display();
#else
  GUI.setLine(GUI.LINE1);
  GUI.put_string_at(0, "Load Project:");
  GUI.setLine(GUI.LINE2);
  if (cur_proj == encoders[1]->cur) {
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

void LoadProjectPage::setup() {
  bool ret;
  int b;
#ifdef OLED_DISPLAY
  oled_display.clearDisplay();
  classic_display = false;
#endif

  DEBUG_PRINT_FN();
  DEBUG_PRINTLN("Load project page");
}
void LoadProjectPage::init() {
  DEBUG_PRINT_FN();
  char temp_entry[16];

  SdFile dirfile;
  int index = 0;
  //  dirfile.open("/",O_READ);
  SD.vwd()->rewind();
  numEntries = 0;
  while (dirfile.openNext(SD.vwd(), O_READ) && (numEntries < MAX_ENTRIES)) {
    for (uint8_t c = 0; c < 16; c++) {
      temp_entry[c] = 0;
    }
    dirfile.getName(temp_entry, 16);
    char mcl[4] = "mcl";
    bool is_mcl_file = true;

    DEBUG_PRINTLN(temp_entry);

    for (uint8_t a = 1; a < 3; a++) {
      if (temp_entry[strlen(temp_entry) - a] != mcl[3 - a]) {
        is_mcl_file = false;
      }
    }
    if (is_mcl_file) {
      uint16_t num_of_entries = numEntries;
      uint32_t pos = FILE_ENTRIES_START + numEntries * 16;
      volatile uint8_t *ptr = pos;
      switch_ram_bank(1);
      memcpy(ptr, &temp_entry[0],16);
      switch_ram_bank(0);
      DEBUG_PRINTLN("project file identified");

      if (strcmp(&temp_entry[0], &mcl_cfg.project[1]) == 0) {
        DEBUG_PRINTLN("match");
        DEBUG_PRINTLN(temp_entry);
        DEBUG_PRINTLN(mcl_cfg.project);

        cur_proj = numEntries;
        encoders[1]->cur = numEntries;
      }

      numEntries++;
    }
    index++;
    dirfile.close();
    DEBUG_PRINTLN(numEntries);
  }

  if (numEntries <= 0) {
    numEntries = 0;
    ((MCLEncoder *)encoders[1])->max = 0;
  }
  ((MCLEncoder *)encoders[1])->max = numEntries - 1;

  DEBUG_PRINTLN("finished load proj setup");
}

void LoadProjectPage::draw_scrollbar(uint8_t x_offset) {
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

void LoadProjectPage::loop() {

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

bool LoadProjectPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {

    return true;
  }

  if (EVENT_PRESSED(event, Buttons.ENCODER1) ||
      EVENT_PRESSED(event, Buttons.ENCODER2) ||
      EVENT_PRESSED(event, Buttons.ENCODER3) ||
      EVENT_PRESSED(event, Buttons.ENCODER4)) {

      char temp_entry[16];
      uint32_t pos = FILE_ENTRIES_START + encoders[1]->getValue() * 16;
      volatile uint8_t *ptr = pos;
      switch_ram_bank(1);
      memcpy(&temp_entry[0],ptr,16);
      switch_ram_bank(0);

    uint8_t size = m_strlen(temp_entry);
    if (strcmp(temp_entry[size - 4], "mcl") == 0) {

      char temp[size + 1];
      temp[0] = '/';
      m_strncpy(&temp[1], temp_entry, size);

      if (proj.load_project(temp)) {
        GUI.setPage(&grid_page);
      } else {
        gfx.alert("PROJECT ERROR", "NOT COMPATIBLE");
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
      }
    }
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON1) ||
      EVENT_RELEASED(event, Buttons.BUTTON2) ||
      EVENT_RELEASED(event, Buttons.BUTTON3) ||
      EVENT_PRESSED(event, Buttons.BUTTON4)) {
    if (proj.project_loaded) {
      GUI.popPage();
      return true;
    }
  }
  return false;
}
