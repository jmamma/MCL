#include "WProgram.h"

#include "Midi.h"
#include "MidiSysex.hh"

void MidiSysexClass::end() {
  callSysexCallBacks = false;
  for (int i = 0; i < NUM_SYSEX_SLAVES; i++) {
    if (isListenerActive(listeners[i])) {
      listeners[i]->end();
    }
  }
}
MididuinoSysexListenerClass::MididuinoSysexListenerClass()
    : MidiSysexListenerClass() {
  ids[0] = MIDIDUINO_SYSEX_VENDOR_1;
  ids[1] = MIDIDUINO_SYSEX_VENDOR_2;
  ids[2] = BOARD_ID;
}

void MididuinoSysexListenerClass::handleByte(uint8_t byte) {
  if (sysex->len == 3 && byte == CMD_START_BOOTLOADER) {
#ifdef MIDIDUINO
    start_bootloader();
#endif
  }
}

MididuinoSysexListenerClass MididuinoSysexListener;
