#include "MCL.h"
#include "NewProjectPage.h"

void NewProjectPage::setup() {}

void NewProjectPage::init() {
  last_clock = slowclock;
  DEBUG_PRINTLN("New project page");
  char my_string[16] = "/project___.mcl";

  my_string[8] = (mcl_cfg.number_projects % 1000) / 100 + '0';
  my_string[8 + 1] = (mcl_cfg.number_projects % 100) / 10 + '0';
  my_string[8 + 2] = (mcl_cfg.number_projects % 10) + '0';
  m_strncpy(newprj, my_string, 16);
  curpage = NEW_PROJECT_PAGE;
  encoders[0]->cur = 10;
  update_prjpage_char();
}

void NewProjectPage::update_prjpage_char() {
  uint8_t x = 0;
  char allowedchar[38] = "0123456789abcdefghijklmnopqrstuvwxyz_";
  // Check to see that the character chosen is in the list of allowed characters
  while ((newprj[encoders[0]->cur] != allowedchar[x]) && (x < 38)) {

    x++;
  }

  // Ensure the encoder does not go out of bounds, by resetting it to a
  // character within the allowed characters list
  encoders[1]->setValue(x);
  // Update the projectname.
  encoders[0]->old = encoders[0]->cur;
}

void NewProjectPage::display() {
  char allowedchar[38] = "0123456789abcdefghijklmnopqrstuvwxyz_";
  // Check to see that the character chosen is in the list of allowed characters
  if (encoders[0]->hasChanged()) {

    update_prjpage_char();
  }
  if (encoders[1]->hasChanged()) {
    last_clock = slowclock;
  }
  //    if ((encoders[2]->hasChanged())){
  newprj[encoders[0]->getValue()] = allowedchar[encoders[1]->getValue()];
  //  }

  GUI.setLine(GUI.LINE1);
  GUI.put_string_at(0, "New Project:");
  GUI.setLine(GUI.LINE2);
  char tmp_str[18];
  m_strncpy(tmp_str, newprj, 18);
  if (clock_diff(last_clock, slowclock) > FLASH_SPEED) {
    tmp_str[encoders[0]->getValue()] = ' ';
  }
  if (clock_diff(last_clock, slowclock) > FLASH_SPEED * 2) {
    last_clock = slowclock;
  }
  GUI.put_string_at(0, &tmp_str[1]);
}
bool NewProjectPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {

    return true;
  }

  if (EVENT_PRESSED(event, Buttons.ENCODER1) ||
      EVENT_PRESSED(event, Buttons.ENCODER2) ||
      EVENT_PRESSED(event, Buttons.ENCODER3) ||
      EVENT_PRESSED(event, Buttons.ENCODER4)) {
    LCD.goLine(0);
    LCD.puts("Please Wait");
    LCD.goLine(1);
    LCD.puts("Creating Project");
    DEBUG_PRINTLN(newprj);
    if (SD.exists(newprj)) {
      GUI.flash_strings_fill("Project exists", "");
      DEBUG_PRINTLN("Project exists");
      return true;
    }

    bool ret = proj.new_project(newprj);
    if (ret) {
      if (proj.load_project(newprj)) {

        grid_page.reload_slot_models = false;
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
  if (EVENT_PRESSED(event, Buttons.BUTTON1) ||
      EVENT_RELEASED(event, Buttons.BUTTON2) ||
      EVENT_RELEASED(event, Buttons.BUTTON3) ||
      EVENT_PRESSED(event, Buttons.BUTTON4)) {
    if (proj.project_loaded) {
      GUI.setPage(&grid_page);
      return true;
    }
  }
  return false;
}
