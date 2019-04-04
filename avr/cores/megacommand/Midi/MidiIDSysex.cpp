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
  DEBUG_PRINT_FN(); 
}

void MidiIDSysexListenerClass::end_immediate() {
  // MidiUartParent *uart = sysex->uart;
  DEBUG_PRINT_FN();
  MidiID *dev = &(sysex->uart->device);
  uint16_t p = (uint16_t) dev;
  DEBUG_PRINTLN(p);
 

  if ((sysex->getByte(0) == ids[0]) && (sysex->getByte(2) == ids[1]) &&
      (sysex->getByte(3) == ids[2])) {
    uint8_t i = 4;
    dev->manufacturer_id[0] = sysex->getByte(i);
    if (sysex->getByte(i++) == 0) {
      dev->manufacturer_id[1] = sysex->getByte(i++);
      dev->manufacturer_id[2] = sysex->getByte(i++);
    }
    DEBUG_PRINTLN(sysex->getByte(i));
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
    
    DEBUG_PRINTLN(dev->family_code[0]);
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
