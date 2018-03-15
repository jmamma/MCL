#include "LoadProjectPage.h"

void LoadProjectPage::display() {
  GUI.setLine(GUI.LINE1);
  GUI.put_string_at(0, "Project:");
  GUI.setLine(GUI.LINE2);
  GUI.put_string_at_fill(0, file_entries[encoders[1]->cur]);
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
    uint8_t size = m_strlen(file_entries[encoders[1]->getValue()]);
    if (strcmp(&file_entries[encoders[1]->getValue()][size - 4], "mcl") ==
        0) {

      char temp[size + 1];
      temp[0] = '/';
      m_strncpy(&temp[1], file_entries[encoders[1]->getValue()], size);

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

bool LoadProjectPage::setup() {}
