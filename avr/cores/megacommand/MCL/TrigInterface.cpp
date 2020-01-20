#include "TrigInterface.h"
#include "MD.h"
#include "NoteInterface.h"
#include "MCL.h"

void TrigInterface::start() {

}

bool TrigInterface::on() {

  sysex->addSysexListener(this);
  if (state) {
    return false;
  }
  if (!MD.connected) {
    return false;
  }
  state = true;
  activate_trig_interface();
  note_interface.notecount = 0;
  note_interface.init_notes();
  note_interface.note_proceed = true;
  return true;
}

bool TrigInterface::off() {
  sysex->removeSysexListener(this);
  note_interface.note_proceed = false;
  if (!state) {
    return false;
  }
  if (!MD.connected) {
    return false;
  }
  state = false;
  deactivate_trig_interface();
  return true;
}

void TrigInterface::end() { }

void TrigInterface::activate_trig_interface() {
 uint8_t data[3] = { 0x70, 0x31, 0x01 };
 MD.sendRequest(data, sizeof(data));
}

void TrigInterface::deactivate_trig_interface() {
 uint8_t data[3] = { 0x70, 0x31, 0x00 };
 MD.sendRequest(data, sizeof(data));
}

bool TrigInterface::is_trig_interface() {
 uint8_t msg[3] = { 0xF0, 0x7F, 0x0D };

 if (sysex->getByte(0) != msg[0]) { return false; }
 if (sysex->getByte(1) != msg[1]) { return false; }
 if (sysex->getByte(2) != msg[2]) { return false; }

 return true;
}

void TrigInterface::end_immediate() {
  if (!is_trig_interface()) { return; }

  if (!state) {
    return;
  }
  uint8_t trig = sysex->getByte(3);

  if (trig >= 0x40) {
  note_interface.note_off_event(trig - 0x40, UART1_PORT);
  }
  else {
  note_interface.note_on_event(trig, UART1_PORT);
  }

  return;
}

TrigInterface trig_interface;
