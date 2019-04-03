#include "WProgram.h"

#include "MD.h"
#include "MDMessages.hh"
#include "MDSysex.hh"
#include "helpers.h"

MDSysexListenerClass MDSysexListener;

void MDSysexListenerClass::start() {
  msgType = 255;
  isMDMessage = false;
}

void MDSysexListenerClass::handleByte(uint8_t byte) {
}

void MDSysexListenerClass::end_immediate() {
}

void MDSysexListenerClass::end() {
  if (MD.midi->midiSysex.getSysexByte(3) == 0x02) {
    isMDMessage = true;
  } else {
    isMDMessage = false;
    return;
  }
  msgType = MD.midi->midiSysex.getSysexByte(sizeof(machinedrum_sysex_hdr));
  switch (msgType) {
  case MD_STATUS_RESPONSE_ID:
   onStatusResponseCallbacks.call(MidiSysex.data[6], MidiSysex.data[7]);
    break;

  case MD_GLOBAL_MESSAGE_ID:
    onGlobalMessageCallbacks.call();
    break;

  case MD_KIT_MESSAGE_ID:
    onKitMessageCallbacks.call();
    break;

  case MD_PATTERN_MESSAGE_ID:
    onPatternMessageCallbacks.call();
    break;

  case MD_SONG_MESSAGE_ID:
    onSongMessageCallbacks.call();
    break;
  case MD_SAMPLE_NAME_ID:
    onSampleNameCallbacks.call();
    break;
  }


}

void MDSysexListenerClass::setup() { MidiSysex.addSysexListener(this); }
