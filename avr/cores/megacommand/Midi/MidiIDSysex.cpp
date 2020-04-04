#include "WProgram.h"

#include "MidiIDSysex.hh"
#include "helpers.h"

MidiIDSysexListenerClass MidiIDSysexListener;

void MidiIDSysexListenerClass::start() {

  msgType = 255;
  isIDMessage = false;
}

void MidiIDSysexListenerClass::handleByte(uint8_t byte) {}

#define ELEKTRON_ID 0x3C
#define MD_ID 0x02
#define MD_ID_NAME 0x73

void MidiIDSysexListenerClass::end() {
}

void MidiIDSysexListenerClass::end_immediate() {
  // MidiUartParent *uart = sysex->uart;
  MidiID *dev = &(sysex->uart->device);
  uint16_t p = (uint16_t) dev;

  uint8_t i = 2;
  if ((sysex->getByte(i++) == 0x06) &&
      (sysex->getByte(i++) == 0x02)) {

    DEBUG_PRINTLN("MidiID message detected");
    dev->manufacturer_id[0] = sysex->getByte(i++);
    if (dev->manufacturer_id[0] == 0) {
      dev->manufacturer_id[1] = sysex->getByte(i++);
      dev->manufacturer_id[2] = sysex->getByte(i++);
    }
    dev->family_code[0] = sysex->getByte(i++);
    dev->family_code[1] = sysex->getByte(i++);
    dev->family_member[0] = sysex->getByte(i++);
    dev->family_member[1] = sysex->getByte(i++);

    dev->software_revision[0] = sysex->getByte(i++);
    dev->software_revision[1] = sysex->getByte(i++);
    dev->software_revision[2] = sysex->getByte(i++);
    dev->software_revision[3] = sysex->getByte(i++);

    msgType = sysex->getByte(2);
    isIDMessage = true;
   return;
  }

  else {
    isIDMessage = false;
    return;
  }

}
void MidiIDSysexListenerClass::setup(MidiClass *_midi) {
  sysex = &(_midi->midiSysex);
  //MidiSysex.addSysexListener(this);
//  MidiSysex2.addSysexListener(this);
}

void MidiIDSysexListenerClass::cleanup() {
 // MidiSysex.removeSysexListener(this);
 // MidiSysex2.removeSysexListener(this);
}
