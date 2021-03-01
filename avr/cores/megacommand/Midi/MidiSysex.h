/*c Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef MIDISYSEX_H__
#define MIDISYSEX_H__

#include "MidiUart.h"
#include <inttypes.h>

class MidiSysexClass;

/**
 * \addtogroup Midi
 *
 * @{
 **/

/**
 * \addtogroup midi_sysex Midi Sysex
 *
 * @{
 **/

class MidiSysexListenerClass {
  /**
   * \addtogroup midi_sysex
   *
   * @{
   **/

public:
  uint8_t ids[3];
  MidiSysexClass *sysex;
  uint8_t msgType;

  MidiSysexListenerClass(MidiSysexClass *_sysex = NULL) {
    sysex = _sysex;
    ids[0] = 0;
    ids[1] = 0;
    ids[2] = 0;
    msgType = 255;
  };

  virtual void start() {}
  virtual void abort() {}
  virtual void end() {}
  virtual void end_immediate() {}
  virtual void handleByte(uint8_t byte) {}

#ifdef HOST_MIDIDUINO
  virtual ~MidiSysexListenerClass() {}
#endif

  /* @} */
};

#define NUM_SYSEX_SLAVES 4

class MidiSysexClass {
  /**
   * \addtogroup midi_sysex
   *
   * @{
   **/

protected:
  bool aborted;
  bool recording;
  uint8_t recvIds[3];
  bool sysexLongId;
  uint8_t *sysex_highmem_buf;
  uint16_t sysex_bufsize;

public:

  bool callSysexCallBacks;
  uint16_t recordLen;
  MidiUartParent *uart;
  MidiSysexListenerClass *listeners[NUM_SYSEX_SLAVES];

  MidiSysexClass(MidiUartParent *_uart, uint16_t size, uint8_t *ptr) {
    uart = _uart;
    sysex_highmem_buf = ptr;
    sysex_bufsize = size;
    aborted = false;
    recording = false;
    sysexLongId = false;
  }

  ALWAYS_INLINE() void startRecord() {
    recording = true;
    recordLen = 0;
  }

  ALWAYS_INLINE() void stopRecord() { recording = false; }

  ALWAYS_INLINE() void resetRecord(uint16_t maxLen = 0) {
    recording = false;
    recordLen = 0;
  }

  ALWAYS_INLINE() void putByte(uint16_t offset, uint8_t c) {
    put_byte_bank1(sysex_highmem_buf + offset, c);
  }

  uint8_t getByte(uint16_t n) {
    if (n < sysex_bufsize) {
      // Retrieve data from specified memory buffer
      // Read from sysex buffers in HIGH membank
      return get_byte_bank1(sysex_highmem_buf + n);
    }
    return 255;
  }

  ALWAYS_INLINE() bool recordByte(uint8_t c) {
    if (recordLen < sysex_bufsize) {
      // Record data to specified memory buffer
      // Write to sysex buffers in HIGH membank
      putByte(recordLen, c);
      ++recordLen;
      return true;
    }
    return false;
  }

  void initSysexListeners() {
    for (int i = 0; i < NUM_SYSEX_SLAVES; i++)
      listeners[i] = NULL;
  }
  bool addSysexListener(MidiSysexListenerClass *listener) {
    for (int i = 0; i < NUM_SYSEX_SLAVES; i++) {
      if (listeners[i] == listener) {
        return true;
      }
    }
   for (int i = 0; i < NUM_SYSEX_SLAVES; i++) {
      if (listeners[i] == NULL) {
        listeners[i] = listener;
        listener->sysex = this;
        return true;
      }
    }
    return false;
  }
  void removeSysexListener(MidiSysexListenerClass *listener) {
    for (int i = 0; i < NUM_SYSEX_SLAVES; i++) {
      if (listeners[i] == listener)
        listeners[i] = NULL;
    }
  }
  ALWAYS_INLINE() bool isListenerActive(MidiSysexListenerClass *listener) {
    if (listener == NULL)
      return false;
    /* catch all */
    if (listener->ids[0] == 0xFF)
      return true;
    if (sysexLongId) {
      if (recvIds[0] == listener->ids[0] && recvIds[1] == listener->ids[1] &&
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

  ALWAYS_INLINE() void reset() {
    aborted = false;
    recording = false;
    recordLen = 0;
    callSysexCallBacks = false;
    sysexLongId = false;
    recvIds[0] = 0;
    recvIds[1] = 0;
    recvIds[2] = 0;
    startRecord();
  }

  void start() {
    /*
    for (int i = 0; i < NUM_SYSEX_SLAVES; i++) {
      if (isListenerActive(listeners[i])) {
        listeners[i]->start();
      }
    }
    */
  }
  ALWAYS_INLINE() void abort() {
    // don't reset len, leave at maximum when aborted
    //  len = 0;
    aborted = true;
    recording = false;
    for (int i = 0; i < NUM_SYSEX_SLAVES; i++) {
      if (isListenerActive(listeners[i]))
        listeners[i]->abort();
    }
  }

  // Handled by main loop
  void end() {
    callSysexCallBacks = false;
    for (int i = 0; i < NUM_SYSEX_SLAVES; i++) {
      if (isListenerActive(listeners[i])) {
        listeners[i]->end();
      }
    }
  }

  // Handled by interrupts
  ALWAYS_INLINE() void end_immediate() {
    stopRecord();
    recvIds[0] = getByte(0);
    if (recvIds[0] == 0x00) {
      sysexLongId = true;
    } else {
      sysexLongId = false;
    }
    if (sysexLongId) {
        recvIds[1] = getByte(1);
        recvIds[2] = getByte(2);
    }
    for (int i = 0; i < NUM_SYSEX_SLAVES; i++) {
      if (isListenerActive(listeners[i])) {
        listeners[i]->end_immediate();
      }
    }
  }

  ALWAYS_INLINE() void handleByte(uint8_t byte) {
    if (recording) {
      recordByte(byte);
    }
    // XXX listener handleByte ignored
  }

  /* @} */
};

// extern MidiSysexClass MidiSysex;
// extern MidiSysexClass MidiSysex2;
#define MidiSysex Midi.midiSysex
#define MidiSysex2 Midi2.midiSysex

/* @} @} */

#endif /* MIDISYSEX_H__ */
