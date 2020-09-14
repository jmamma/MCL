#include "MCL_impl.h"

void TrigInterface::start() {

}

void TrigInterface::send_md_leds() {
    uint16_t led_mask = 0;
    for (uint8_t i = 0; i < 16; i++) {
      if (note_interface.notes[i] == 1) {
        SET_BIT16(led_mask, i);
      }
    }
    MD.set_trigleds(led_mask, TRIGLED_OVERLAY);
}

bool TrigInterface::on() {

  if (state) {
    return false;
  }
  if (!MD.connected) {
    return false;
  }

  sysex->addSysexListener(this);
  state = true;
  DEBUG_PRINTLN(F("activating trig interface"));
  MD.activate_trig_interface();
  note_interface.notecount = 0;
  note_interface.init_notes();
  note_interface.note_proceed = true;
  return true;
}

bool TrigInterface::off() {
 if (!state) {
    return false;
  }
  if (!MD.connected) {
    return false;
  }
  sysex->removeSysexListener(this);
  note_interface.note_proceed = false;
  DEBUG_PRINTLN(F("deactiviating trig interface"));
  state = false;
  MD.deactivate_trig_interface();
  return true;
}

void TrigInterface::end() { }

void TrigInterface::end_immediate() {
  if (!state) {
    return;
  }
 if (sysex->getByte(0) != ids[0]) { return; }
 if (sysex->getByte(1) != ids[1]) { return; }

 uint8_t trig = sysex->getByte(2);

  if (trig >= 0x40) {
  note_interface.note_off_event(trig - 0x40, UART1_PORT);
  }
  else {
  DEBUG_PRINTLN(F("trig on"));
  DEBUG_PRINTLN(trig);
  note_interface.note_on_event(trig, UART1_PORT);
  }
  return;
}

TrigInterface trig_interface;
