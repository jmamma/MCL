#include "LoadProjectPage.h"
#include "MCL.h"

void LoadProjectPage::init() {

  DEBUG_PRINT_FN();
  show_save = false;
  dir_browser = false;
  show_new_folder = false;
  char *mcl = ".mcl";
  strcpy(match, mcl);
  char *files = "Project";
  strcpy(title, files);
  FileBrowserPage::init();
}
bool LoadProjectPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {

    return true;
  }
  if (EVENT_RELEASED(event, Buttons.BUTTON1) ||
      EVENT_RELEASED(event, Buttons.BUTTON2) ||
      EVENT_RELEASED(event, Buttons.BUTTON3) ||
      EVENT_RELEASED(event, Buttons.BUTTON4)) {
    GUI.popPage();
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.ENCODER1) ||
      EVENT_PRESSED(event, Buttons.ENCODER2) ||
      EVENT_PRESSED(event, Buttons.ENCODER3) ||
      EVENT_PRESSED(event, Buttons.ENCODER4)) {

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
      file.close();
      SD.chdir(lwd);

      SD.vwd()->getName(dir_entry, 16);
      lwd[strlen(lwd) - strlen(dir_entry) - 1] = '\0';
      DEBUG_PRINTLN(lwd);

      init();
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
    file.close();
    if (proj.load_project(temp_entry)) {
      GUI.setPage(&grid_page);
    } else {
      gfx.alert("PROJECT ERROR", "NOT COMPATIBLE");
    }
    return true;
  }
}
