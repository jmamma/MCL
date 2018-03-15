#include "NewProjectPage.h"

void NewProjectPage::display() {
  if (encoders[1]->hasChanged()) {
      update_prjpage_char();

    }
    //    if ((encoders[2]->hasChanged())){
    newprj[encoders[1]->getValue()] = allowedchar[encoders[2]->getValue()];
    //  }

    GUI.setLine(GUI.LINE1);
    GUI.put_string_at(0, "New Project:");
    GUI.setLine(GUI.LINE2);
    GUI.put_string_at(0, &newprj[1]);
}
bool NewProjectPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {

    return true;
  }

  if (EVENT_RELEASED(event, Buttons.ENCODER1) ||
      EVENT_RELEASED(event, Buttons.ENCODER2) ||
      EVENT_RELEASED(event, Buttons.ENCODER3) ||
      EVENT_RELEASED(event, Buttons.ENCODER4)) {
    LCD.goLine(0);
    LCD.puts("Please Wait");
    LCD.goLine(1);
    LCD.puts("Creating Project");

    if (SD.exists(newprj)) {
      GUI.flash_strings_fill("Project exists", "");
      return true;
    }

    bool ret = sd_new_project(newprj);
    if (ret) {
      if (sd_load_project(newprj)) {
        GUI.setPage(&grid_page);
        reload_slot_models = 0;
        curpage = GRID_PAGE;
        return true;
      } else {
        GUI.flash_strings_fill("SD FAILURE", "--");
        return false;
        //  LCD.goLine(0);
        // LCD.puts("SD Failure");
      }
    }

    return true;
  }
  return false;
}

bool NewProjectPage::setup() {}
