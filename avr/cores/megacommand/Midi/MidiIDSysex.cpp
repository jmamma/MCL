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
 

  if ((MidiSysex.data[0] == ids[0]) && (MidiSysex.data[2] == ids[1]) &&
      (MidiSysex.data[3] == ids[2])) {
    uint8_t i = 4;
    dev->manufacturer_id[0] = MidiSysex.data[i];
    if (MidiSysex.data[i++] == 0) {
      dev->manufacturer_id[1] = MidiSysex.data[i++];
      dev->manufacturer_id[2] = MidiSysex.data[i++];
    }
    DEBUG_PRINTLN(MidiSysex.data[i]);
    dev->family_code[0] = MidiSysex.data[i++];
    dev->family_code[1] = MidiSysex.data[i++];
    dev->family_member[0] = MidiSysex.data[i++];
    dev->family_member[1] = MidiSysex.data[i++];

    dev->software_revision[0] = MidiSysex.data[i++];
    dev->software_revision[1] = MidiSysex.data[i++];
    dev->software_revision[2] = MidiSysex.data[i++];
    dev->software_revision[3] = MidiSysex.data[i++];
    
    msgType = MidiSysex.data[2];
    isIDMessage = true;
    
    DEBUG_PRINTLN(dev->family_code[0]);
   return;
  }

  else {
    isIDMessage = false;
    return;
  }

}
void MidiIDSysexListenerClass::setup() {
  //MidiSysex.addSysexListener(this);
//  MidiSysex2.addSysexListener(this);
}

void MidiIDSysexListenerClass::cleanup() {
 // MidiSysex.removeSysexListener(this);
 // MidiSysex2.removeSysexListener(this);
}
