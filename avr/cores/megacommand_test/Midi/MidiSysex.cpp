#include "WProgram.h"

#include "Midi.h"
#include "MidiSysex.hh"

void MidiSysexClass::reset() {
  len = 0;
  aborted = false;
  recording = false;
  recordLen = 0;

  sysexLongId = false;
  recvIds[0] = 0;
  recvIds[1] = 0;
  recvIds[2] = 0;
  startRecord();
}

void MidiSysexClass::resetRecord(uint8_t *buf, uint16_t maxLen) {
  if (buf == NULL) {
    recordBuf = data;
    maxRecordLen = max_len;
  } else {
    recordBuf = buf;
    maxRecordLen = maxLen;
  }
  recording = false;
  recordLen = 0;
}

void MidiSysexClass::startRecord(uint8_t *buf, uint16_t maxLen) {
  resetRecord(buf, maxLen);
  recording = true;
}

void MidiSysexClass::stopRecord() {
  recording = false;
}

bool MidiSysexClass::isListenerActive(MidiSysexListenerClass *listener) {
  if (listener == NULL)
    return false;
  /* catch all */
  if (listener->ids[0] == 0xFF)
    return true;
  if (sysexLongId) {
    if (recvIds[0] == listener->ids[0] &&
				recvIds[1] == listener->ids[1] &&
				recvIds[2] == listener->ids[2])
      return true;
    else
      return false;
  } else {
    if (recvIds[0] == listener->ids[0])
      return true;
    else
      return false;
  }
}

void MidiSysexClass::abort() {
  // don't reset len, leave at maximum when aborted
  //  len = 0;
  aborted = true;

  for (int i = 0; i < NUM_SYSEX_SLAVES; i++) {
    if (isListenerActive(listeners[i]))
      listeners[i]->abort();
  }
}

void MidiSysexClass::start() {
  for (int i = 0; i < NUM_SYSEX_SLAVES; i++) {
    if (isListenerActive(listeners[i])) {
      listeners[i]->start();
    }
  }
}

void MidiSysexClass::end() {
  for (int i = 0; i < NUM_SYSEX_SLAVES; i++) {
    if (isListenerActive(listeners[i])) {
      listeners[i]->end();
    }
  }
}

void MidiSysexClass::handleByte(uint8_t byte) {
  if (aborted)
    return;

  if (len == 0) {
    recvIds[0] = byte;
    if (byte == 0x00) {
      sysexLongId = true;
    } else {
      sysexLongId = false;
      start();
    }
  }
  if (sysexLongId) {
    if (len == 1) {
      recvIds[1] = byte;
    } else if (len == 2) {
      recvIds[2] = byte;
      start();
    }
  }

  for (int i = 0; i < NUM_SYSEX_SLAVES; i++) {
    if (isListenerActive(listeners[i])) {
      listeners[i]->handleByte(byte);
    }
  }

  len++;

  if (recording) {
    recordByte(byte);
  }
}

bool MidiSysexClass::recordByte(uint8_t c) {
  if (recordLen < maxRecordLen && recordBuf != NULL) {
    recordBuf[recordLen++] = c;
    return true;
  } else {
    return false;
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
