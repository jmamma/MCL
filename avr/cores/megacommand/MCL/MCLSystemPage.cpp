#include "MCL.h"
#include "MCLSystemPage.h"

void MCLSystemPage::setup() {}
void MCLSystemPage::init() {}

bool MCLSystemPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {

    return true;
  }
  if (EVENT_RELEASED(event, Buttons.ENCODER1) ||
      EVENT_RELEASED(event, Buttons.ENCODER2) ||
      EVENT_RELEASED(event, Buttons.ENCODER3) ||
      EVENT_RELEASED(event, Buttons.ENCODER1)) {
    if (encoders[0]->getValue() == 0) {
      GUI.setPage(&load_proj_page);
      return true;
    } else if (encoders[0]->getValue() == 1) {
      GUI.setPage(&new_proj_page);
      return true;
    }
    mcl_cfg.write_cfg();
    midi_setup.cfg_ports();
    if ((!Serial) && (mcl_cfg.display_mirror == 1)) {
      GUI.display_mirror = true;
      Serial.begin(SERIAL_SPEED);
    }
    if ((Serial) && (mcl_cfg.display_mirror == 0)) {
      GUI.display_mirror = false;
      Serial.end();
    }
    GUI.setPage(&grid_page);
    curpage = 0;
    return true;
  }

  return false;
}

void MCLSystemPage::display() {
  GUI.setLine(GUI.LINE1);
  GUI.put_string_at_fill(0, "Name:");
  GUI.put_string_at(6, &mcl_cfg.project[1]);
  GUI.setLine(GUI.LINE2);
  switch (encoders[0]->getValue()) {
  case 0:
    GUI.put_string_at_fill(0, "Load Project");
    break;
  case 1:
    GUI.put_string_at_fill(0, "New Project");
    break;

  case 2:

    if (encoders[0]->hasChanged()) {
      encoders[0]->old = encoders[0]->cur;
      encoders[1]->setValue(mcl_cfg.uart1_turbo);
    }
    if (encoders[1]->hasChanged()) {
      if (encoders[1]->getValue() > 3) {
        encoders[1]->cur = 3;
      }
      //     encoders[1]->old = encoders[1]->cur;
      mcl_cfg.uart1_turbo = encoders[1]->getValue();
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
    break;

  case 3:

    if (encoders[0]->hasChanged()) {
      encoders[0]->old = encoders[0]->cur;
      encoders[1]->setValue(mcl_cfg.uart2_turbo);
    }
    if (encoders[1]->hasChanged()) {
      if (encoders[1]->getValue() > 3) {
        encoders[1]->cur = 3;
      }
      //    encoders[1]->old = encoders[1]->cur;
      mcl_cfg.uart2_turbo = encoders[1]->getValue();
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
    break;
  case 4:

    if (encoders[0]->hasChanged()) {
      encoders[0]->old = encoders[0]->cur;
      encoders[1]->setValue(mcl_cfg.clock_rec);
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
      mcl_cfg.clock_rec = encoders[1]->getValue();
    }
    break;
  case 5:

    if (encoders[0]->hasChanged()) {
      encoders[0]->old = encoders[0]->cur;
      encoders[1]->setValue(mcl_cfg.clock_send);
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
      mcl_cfg.clock_send = encoders[1]->getValue();
    }
    break;
  case 6:

    if (encoders[0]->hasChanged()) {
      encoders[0]->old = encoders[0]->cur;
      encoders[1]->setValue(mcl_cfg.poly_max);
    }
    GUI.put_string_at_fill(0, "MD POLYMAX:");
    GUI.put_value_at2(13, mcl_cfg.poly_max);
    if (encoders[1]->hasChanged()) {
      if (encoders[1]->getValue() > 16) {
        encoders[1]->cur = 16;
      }
      if (encoders[1]->getValue() < 1) {
        encoders[1]->cur = 1;
      }
      mcl_cfg.poly_max = encoders[1]->getValue();
    }
    break;
  case 7:

    if (encoders[0]->hasChanged()) {
      encoders[0]->old = encoders[0]->cur;
      encoders[1]->setValue(mcl_cfg.uart2_ctrl_mode);
    }
    GUI.put_string_at_fill(0, "MD CTRL:");
    if (encoders[1]->cur == 16) {
      GUI.put_string_at_fill(9, "INT");
    }
    if (encoders[1]->cur < 16) {
      GUI.put_string_at_fill(9, "CHAN ");
      GUI.put_value_at2(14, encoders[1]->cur);
    }
    if (encoders[1]->cur == 17) {
      GUI.put_string_at_fill(9, "OMNI");
    }
    if (encoders[1]->hasChanged()) {
      if (encoders[1]->getValue() > 17) {
        encoders[1]->cur = 17;
      }
      mcl_cfg.uart2_ctrl_mode = encoders[1]->getValue();
    }
    break;
  case 8:

    if (encoders[0]->hasChanged()) {
      encoders[0]->old = encoders[0]->cur;
      encoders[1]->setValue(mcl_cfg.display_mirror);
    }
    GUI.put_string_at_fill(0, "Display:");
    if (encoders[1]->cur == 0) {
      GUI.put_string_at_fill(8, "INT");
    }
    if (encoders[1]->cur > 0) {
      GUI.put_string_at_fill(8, "INT+EXT");
    }
  if (encoders[1]->hasChanged()) {
    if (encoders[1]->getValue() > 1) {
      encoders[1]->cur = 1;
    }
    mcl_cfg.display_mirror = encoders[1]->getValue();
  }

  break;
}
}
