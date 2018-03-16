#include "NewProjectPage.h"

void NewProjectPage::update_prjpage_char() {
  uint8_t x = 0;
  //Check to see that the character chosen is in the list of allowed characters
  while ((newprj[encoders[1]->cur] != allowedchar[x]) && (x < 38)) {

    x++;
  }

  //Ensure the encoder does not go out of bounds, by resetting it to a character within the allowed characters list
  encoders[2]->setValue(x);
  //Update the projectname.
  encoders[1]->old = encoders[1]->cur;
}

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
        
        Grid.reload_slot_models = false;
        GUI.setPage(&grid_page);
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

void NewProjectPage::setup() {

  char my_string[16] = "/project___.mcl";

  my_string[8] = (cfg.number_projects % 1000) / 100 + '0';
  my_string[8 + 1] = (cfg.number_projects % 100) / 10 + '0';
  my_string[8 + 2] = (cfg.number_projects % 10) + '0';

  m_strncpy(newprj, my_string, 16);
  curpage = NEW_PROJECT_PAGE;


  update_prjpage_char();
}
