#include "MNMMessages.h"
#include "MNMParams.h"
#include "MNMSysex.h"
#include "WProgram.h"
#include "helpers.h"

void MNMSysexListenerClass::start() {
  msgType = 255;
  isMNMMessage = false;
}

void MNMSysexListenerClass::handleByte(uint8_t byte) { }

void MNMSysexListenerClass::end() {

  if (sysex->getByte(3) == 0x03) {
    isMNMMessage = true;
  } else {
    isMNMMessage = false;
    return;
  }

  msgType = sysex->getByte(sizeof(monomachine_sysex_hdr));

  switch (msgType) {
  case MNM_STATUS_RESPONSE_ID:
    onStatusResponseCallbacks.call(sysex->getByte(6), sysex->getByte(7));
    break;

  case MNM_GLOBAL_MESSAGE_ID:
  case MNM_KIT_MESSAGE_ID:
  case MNM_PATTERN_MESSAGE_ID:
  case MNM_SONG_MESSAGE_ID:
    onMessageCallbacks.call();
    break;
  }
}

void MNMSysexListenerClass::setup(MidiClass *_midi) {
  sysex = &(_midi->midiSysex);
  sysex->addSysexListener(this);
}
