#include "MCLSystemPage.h"

bool MCLSystemPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {

    return true;
  }
  if (EVENT_RELEASED(evt, Buttons.ENCODER1) ||
      EVENT_RELEASED(evt, Buttons.ENCODER2) ||
      EVENT_RELEASED(evt, Buttons.ENCODER3) ||
      EVENT_RELEASED(evt, Buttons.ENCODER1)) {
    if (encoders[0]->getValue() == 0) {
      load_project_page();
      return true;
    } else if (encoders[0]->getValue() == 1) {
      new_project_page();
      return true;
    }
    cfg.write_cfg();
    midi_setup.cfg_ports();

    GUI.setPage(&grid_page);
    curpage = 0;
    return true;
  }

  return false;
}

void MCLSystemPage::display() {
GUI.setLine(GUI.LINE1);
  GUI.put_string_at_fill(0, "Name:");
  GUI.put_string_at(6, &cfg.project[1]);
  GUI.setLine(GUI.LINE2);



  if (encoders[0]->getValue() == 4) {


    if (encoders[0]->hasChanged()) {
      encoders[0]->old = encoders[0]->cur;
      encoders[1]->setValue(cfg.clock_rec);
    }
    GUI.put_string_at_fill(0, "CLK REC:");

    if (encoders[1]->getValue() == 0) {
      GUI.put_string_at_fill(10, "MIDI1");
    }
    if (encoders[1]->getValue() >= 1) {
      GUI.put_string_at_fill(10, "MIDI2");
    }
    if (encoders[1]->hasChanged()) {
      if (encoders[1]->getValue() > 1) {
        encoders[1]->cur = 1;
      }
      cfg.clock_rec = encoders[1]->getValue();
    }
  }
  else if (encoders[0]->getValue() == 5) {


    if (encoders[0]->hasChanged()) {
      encoders[0]->old = encoders[0]->cur;
      encoders[1]->setValue(cfg.clock_send);
    }
    GUI.put_string_at_fill(0, "CLK SEND:");

    if (encoders[1]->getValue() == 0) {
      GUI.put_string_at_fill(11, "  OFF");
    }
    if (encoders[1]->getValue() >= 1) {
      GUI.put_string_at_fill(11, "MIDI2");
    }
    if (encoders[1]->hasChanged()) {
      if (encoders[1]->getValue() > 1) {
        encoders[1]->cur = 1;
      }
      cfg.clock_send = encoders[1]->getValue();
    }
  }
  else if (encoders[0]->getValue() == 2) {

    if (encoders[0]->hasChanged()) {
      encoders[0]->old = encoders[0]->cur;
      encoders[1]->setValue(cfg.uart1_turbo);
    }
    GUI.put_string_at_fill(0, "TURBO 1:");

    if (encoders[1]->getValue() == 0) {

      GUI.put_string_at_fill(10, "1x");
    }
    if (encoders[1]->getValue() == 1) {
      GUI.put_string_at_fill(10, "2x");
    }
    if (encoders[1]->getValue() == 2) {
      GUI.put_string_at_fill(10, "4x");
    }
    if (encoders[1]->getValue() == 3) {
      GUI.put_string_at_fill(10, "8x");
    }
    if (encoders[1]->hasChanged()) {
      encoders[1]->old = encoders[1]->cur;
      cfg.uart1_turbo = encoders[1]->getValue();
    }

  }

  else if (encoders[0]->getValue() == 3) {

    if (encoders[0]->hasChanged()) {
      encoders[0]->old = encoders[0]->cur;
      encoders[1]->setValue(cfg.uart2_turbo);
    }
    GUI.put_string_at_fill(0, "TURBO 2:");

    if (encoders[1]->getValue() == 0) {

      GUI.put_string_at_fill(10, "1x");
    }
    if (encoders[1]->getValue() == 1) {
      GUI.put_string_at_fill(10, "2x");
    }
    if (encoders[1]->getValue() == 2) {
      GUI.put_string_at_fill(10, "4x");
    }
    if (encoders[1]->getValue() == 3) {
      GUI.put_string_at_fill(10, "8x");
    }
    if (encoders[1]->hasChanged()) {
      encoders[1]->old = encoders[1]->cur;
      cfg.uart2_turbo = encoders[1]->getValue();
    }

  }
  else if (encoders[0]->getValue() == 0) {
    GUI.put_string_at_fill(0, "Load Project");
  }
  else if (encoders[0]->getValue() == 1) {
    GUI.put_string_at_fill(0, "New Project");
  }

}

bool MCLSystemPage::setup() {}

