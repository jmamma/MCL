#include "MixerPage.h"

void MixerPage::draw_levels() {
  GUI.setLine(GUI.LINE2);
  uint8_t scaled_level;
  char str[17] = "                ";
  for (int i = 0; i < 16; i++) {
    //  if (MD.kit.levels[i] > 120) { scaled_level = 8; }
    // else if (MD.kit.levels[i] < 4) { scaled_level = 0; }
    scaled_level = (int)(((float)MD.kit.levels[i] / (float)127) * 7);
    if (scaled_level == 7) {
      str[i] = (char)(255);
    } else if (scaled_level > 0) {
      str[i] = (char)(scaled_level + 2);
    }
  }
  GUI.put_string_at(0, str);
}
void MixerPage::display() {
  note_interface.draw_notes(0);
  this.draw_levels();
}
bool MixerPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {

    return true;
  }
  if (EVENT_PRESSED(evt, Buttons.ENCODER1)) {
    level_pressmode = 1;
    return true;
  }
  if (EVENT_RELEASED(evt, Buttons.ENCODER1)) {
    level_pressmode = 0;
    return true;
  }

  if (EVENT_PRESSED(evt, Buttons.BUTTON1)) {

    curpage = CUE_PAGE;
    return true;
  }
  if (EVENT_PRESSED(evt, Buttons.BUTTON2)) {
    md_exploit.off();
    GUI.setPage(&grid_page);
    curpage = 0;
    return true;
  }
  return false;
}

bool MixerPage::setup() {
  create_chars_mixer();
  currentkit_temp = MD.getCurrentKit(CALLBACK_TIMEOUT);
  curpage = MIXER_PAGE;
  MD.saveCurrentKit(currentkit_temp);
  MD.getBlockingKit(currentkit_temp);
  level_pressmode = 0;
  mixer_param1.cur = 60;
  md_exploit.on();
  note_inteface.collect_trigs = true;
}
