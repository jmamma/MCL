#include "MidiID.h"
#include "WProgram.h"
#include "MidiIDSysex.h"
#include "helpers.h"
#define UART1_PORT 1
#define UART2_PORT 2

void MidiID::send_id_request(uint8_t id, uint8_t port) {
  DEBUG_PRINT_FN();
  uint8_t data[6] = {0xF0, 0x7E, id, 0x06, 0x01, 0xF7};
  MidiUartParent *uart;
  if (port == UART1_PORT) {
    uart = &MidiUart;
  } else {
    uart = &MidiUart2;
  }
  uart->sendRaw(data, sizeof(data));
}

void MidiID::init() {
  set_id(DEVICE_NULL);
  set_name("");
}

bool MidiID::getBlockingId(uint8_t id, uint8_t port, uint16_t timeout) {
  DEBUG_PRINT_FN();

  if (port == UART1_PORT) {
    DEBUG_PRINTLN("adding listener port1");
    MidiSysex.addSysexListener(&MidiIDSysexListener);
  }
  else {
    MidiSysex2.addSysexListener(&MidiIDSysexListener);
  }

  uint8_t ret = waitForId(id, port, timeout);
  if (port == UART1_PORT) {
    DEBUG_PRINTLN("removing listener port1");
    MidiSysex.removeSysexListener(&MidiIDSysexListener);
  }
  else {
    MidiSysex2.removeSysexListener(&MidiIDSysexListener);
  }

  if (id == ret) {
    return true;
  }
  DEBUG_PRINTLN("bad id");
  return false;
}

uint8_t MidiID::waitForId(uint8_t id, uint8_t port, uint16_t timeout) {
  MidiIDSysexListener.msgType = 255;
  MidiIDSysexListener.isIDMessage = false;

  uint16_t start_clock = read_slowclock();
  uint16_t current_clock = start_clock;
  send_id_request(id, port);
  DEBUG_PRINTLN("waiting for ID");
  do {
    current_clock = read_slowclock();
    handleIncomingMidi();
    // GUI.display();
  } while ((clock_diff(start_clock, current_clock) < timeout) &&
           (!MidiIDSysexListener.isIDMessage));
  return get_id();
}

void MidiID::set_id(uint8_t id) { family_code[0] = id; }
uint8_t MidiID::get_id() { return family_code[0]; }

void MidiID::set_name(const char* str) {
  strncpy(name, str, 16);
  name[sizeof(name)-1] = '\0';
}
char *MidiID::get_name(char *str) {
  strcpy(str, name);
  return str;
}
