#include "MidiID.h"
#include "platform.h"
#include "MidiIDSysex.h"
#include "Midi.h"
#include "helpers.h"
#include "global.h"
#include "MidiUart.h"
#include "Devices/MidiSetup.h"

void MidiID::send_id_request(uint8_t id, uint8_t port) {
  uint8_t data[6] = {0xF0, 0x7E, id, 0x06, 0x01, 0xF7};
  MidiClass *midi = midi_class_for_port(port);
  if (midi == nullptr || midi->uart == nullptr) return;
  midi->uart->sendRaw(data, sizeof(data));
}

void MidiID::init() {
  set_id(DEVICE_NULL);
  set_name("");
}

bool MidiID::getBlockingId(uint8_t id, uint8_t port, uint16_t timeout) {
  return getBlockingId(id, DEVICE_NULL, port, timeout);
}

bool MidiID::getBlockingId(uint8_t id, uint8_t alternate_id, uint8_t port,
                           uint16_t timeout) {

  MidiClass *midi = midi_class_for_port(port);
  if (midi == nullptr || midi->midiSysex == nullptr) return false;
  if (port == UART1_PORT) {
    DEBUG_PRINTLN("adding listener port1");
  }
  midi->midiSysex->addSysexListener(&MidiIDSysexListener);

  uint8_t ret = waitForId(id, port, timeout);
  if (port == UART1_PORT) {
    DEBUG_PRINTLN("removing listener port1");
  }
  midi->midiSysex->removeSysexListener(&MidiIDSysexListener);

  if (id == ret || alternate_id == ret) {
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
  uint8_t _midi_lock_tmp = MidiUartParent::handle_midi_lock;
  MidiUartParent::handle_midi_lock = 1;
  do {
    platform_wait_poll();
    current_clock = read_slowclock();
    handleIncomingMidi();
    // GUI.display();
  } while ((clock_diff(start_clock, current_clock) < timeout) &&
           (!MidiIDSysexListener.isIDMessage));
#if !defined(__AVR__)
  // The hosted wasm/desktop path receives replies through a host-owned virtual
  // cable. If a reply lands as the timeout boundary is crossed, drain one last
  // time before reporting no ID so the next probe does not consume this reply.
  if (!MidiIDSysexListener.isIDMessage) {
    platform_wait_poll();
    handleIncomingMidi();
  }
#endif
  MidiUartParent::handle_midi_lock = _midi_lock_tmp;
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
