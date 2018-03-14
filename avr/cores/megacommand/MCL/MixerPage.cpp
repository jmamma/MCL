#include "MixerPage.h"

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
MixerPage mixer_page;
