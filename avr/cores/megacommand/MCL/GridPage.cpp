#include "GridPage.h"

void GridPage::displayScroll(uint8_t i) {
  if (encoders[i] != NULL) {

    if (((encoders[0]->getValue() + i + 1) % 4) == 0) {
      char strn[2] = "I";
      strn[0] = (char)001;
      //           strn[0] = (char) 219;
      GUI.setLine(GUI.LINE1);

      GUI.put_string_at_noterminator((2 + (i * 3)), strn);

      GUI.setLine(GUI.LINE2);
      GUI.put_string_at_noterminator((2 + (i * 3)), strn);
    }

    else {
      char strn_scroll[2] = "|";
      GUI.setLine(GUI.LINE1);

      GUI.put_string_at_noterminator((2 + (i * 3)), strn_scroll);

      GUI.setLine(GUI.LINE2);
      GUI.put_string_at_noterminator((2 + (i * 3)), strn_scroll);
    }
  }
}
void GridPage::display() {}

bool GridPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {

    return true;
  }

  if (BUTTON_RELEASED(Buttons.BUTTON1) && BUTTON_DOWN(Buttons.BUTTON3)) {
    clear_row(param2.getValue());
    reload_slot_models = 0;
    return true;
  }
  // TRACK READ PAGE

  if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
    grid_save_page->setup = false;
    GUI.setPage(&grid_save_page);

    return true;
  }

  // TRACK WRITE PAGE

  if (EVENT_RELEASED(event, Buttons.BUTTON4)) {
    grid_write_page->setup = false;
    GUI.setPage(&grid_write_page);

    return true;
  }

  if (BUTTON_DOWN(Buttons.BUTTON2) && BUTTON_DOWN(Buttons.BUTTON4)) {
    setLed();
    int curtrack = MD.getCurrentTrack(CALLBACK_TIMEOUT);

    param1.cur = curtrack;

    clearLed();
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON2)) {
    mixer_page->setup = false;
    GUI.setPage(&mixer_page);
    //   draw_levels();
  }
  /*IF button1 and encoder buttons are pressed, store current track selected on
   * MD into the corresponding Grid*/

  //  if (BUTTON_PRESSED(Buttons.BUTTON3)) {
  //      MD.getBlockingGlobal(1);
  //          MD.global.baseChannel = 9;
  //        //global_new.baseChannel;
  //          for (int i=0; i < 16; i++) {
  //            MD.muteTrack(i,true);
  //          }
  //           setLevel(8,100);
  //  }

  if (BUTTON_PRESSED(Buttons.ENCODER1)) {
    if (BUTTON_DOWN(Buttons.BUTTON3)) {
      seq_euc_page->setup = false;
      GUI.setPage(&seq_euc_page);
    } else {
      seq_step_page->setup = false;
      GUI.setPage(&seq_step_page);
    }

    return true;
  }
  if (BUTTON_PRESSED(Buttons.ENCODER2)) {
    seq_rtrk_page->setup = false;
    GUI.setPage(&seq_rtrk_page);

    return true;
  }
  if (BUTTON_PRESSED(Buttons.ENCODER3)) {
    seq_param_a_page->setup = false;
    GUI.setPage(&seq_param_a_page);

    return true;
  }
  if (BUTTON_PRESSED(Buttons.ENCODER4)) {
    seq_ptc_page->setup = false;
    GUI.setPage(&seq_ptc_page);

    return true;
  }
}

if ((EVENT_PRESSED(event, Buttons.BUTTON1) && BUTTON_DOWN(Buttons.BUTTON4)) ||
    (EVENT_PRESSED(event, Buttons.BUTTON4) && BUTTON_DOWN(Buttons.BUTTON1))) {

  system_page->setup = false;
  GUI.setPage(&system_page);

  curpage = SYSTEM_PAGE;

  return true;
}

return false;
}

bool GridPage::setup() {}
