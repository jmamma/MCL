#include "WProgram.h"

#include "MD.h"
#include "MDMessages.h"
#include "MDSysex.h"
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
  if (sysex->getByte(3) == 0x02) {
    isMDMessage = true;
  } else {
    isMDMessage = false;
    return;
  }
  msgType = sysex->getByte(sizeof(machinedrum_sysex_hdr));
  switch (msgType) {
  case MD_STATUS_RESPONSE_ID:
   onStatusResponseCallbacks.call(sysex->getByte(6), sysex->getByte(7));
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

void MDSysexListenerClass::setup(MidiClass *_midi) { sysex = &(_midi->midiSysex); sysex->addSysexListener(this); }
