#include "LoadProjectPage.h"

void LoadProjectPage::display() {
  GUI.setLine(GUI.LINE1);
  GUI.put_string_at(0, "Project:");
  GUI.setLine(GUI.LINE2);
  GUI.put_string_at_fill(0, file_entries[encoders[0]->cur]);
  return;
}

bool LoadProjectPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {

    return true;
  }

  if (EVENT_RELEASED(event, Buttons.ENCODER1) ||
      EVENT_RELEASED(event, Buttons.ENCODER2) ||
      EVENT_RELEASED(event, Buttons.ENCODER3) ||
      EVENT_RELEASED(event, Buttons.ENCODER4)) {
    uint8_t size = m_strlen(file_entries[encoders[0]->getValue()]);
    if (strcmp(&file_entries[encoders[0]->getValue()][size - 4], "mcl") ==
        0) {

      char temp[size + 1];
      temp[0] = '/';
      m_strncpy(&temp[1], file_entries[encoders[0]->getValue()], size);

      if (sd_load_project(temp)) {
        reload_slot_models = 0;
        GUI.setPage(&grid_page);
        curpage = GRID_PAGE;
      } else {
        GUI.flash_strings_fill("PROJECT ERROR", "NOT COMPATIBLE");
      }
    }
    return true;
  }

  return false;
}

void LoadProjectPage::setup() {
  bool ret;
  int b;

  DEBUG_PRINT_FN();
  DEBUG_PRINTLN("Load project page");

  char temp_entry[16];

  SdFile dirfile;
  int index = 0;
  int numEntries = 0;
  //  dirfile.open("/",O_READ);
  SD.vwd()->rewind();

  while (dirfile.openNext(SD.vwd(), O_READ)) {
    for (uint8_t c = 0; c < 16; c++ ) {
      temp_entry[c] = 0;
    }
    dirfile.getName(temp_entry, 16);
    char mcl[3] = "mcl";
    bool is_mcl_file = true;

    DEBUG_PRINTLN(temp_entry);

    for (uint8_t a = 1; a < 3; a++) {
      if (temp_entry[14 - a] != mcl[3 - a]) {
        is_mcl_file = false;
      }
    }
    if (is_mcl_file) {
      strcpy(&file_entries[numEntries][0], &temp_entry[0]);
      DEBUG_PRINTLN("project file identified");
      DEBUG_PRINTLN(file_entries[index]);
      numEntries++;
    }
    index++;
    dirfile.close();

  }

  if (numEntries <= 0) {
    numEntries = 0;
    loadproj_param1->max = 0;
  }
  loadproj_param1->max = numEntries - 1;

  curpage = LOAD_PROJECT_PAGE;
  GUI.setPage(&loadproj_page);

}
