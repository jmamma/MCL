/*c Copyright (c) 2009 - http://ruinwesen.com/ */

#ifndef MIDISYSEX_H__
#define MIDISYSEX_H__

#include "MidiUart.h"
#include <inttypes.h>

#ifndef SYSEX_BUF_SIZE
#define SYSEX_BUF_SIZE 1024
#endif

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

  MidiSysexListenerClass(MidiSysexClass *_sysex = NULL) {
    sysex = _sysex;
    ids[0] = 0;
    ids[1] = 0;
    ids[2] = 0;
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
  uint8_t *data;
  uint8_t *recordBuf;
  volatile uint8_t *sysex_highmem_buf;

public:
  ALWAYS_INLINE() void startRecord(uint8_t *buf = NULL, uint16_t maxLen = 0) {
    resetRecord(buf, maxLen);
    recording = true;
  }

  ALWAYS_INLINE() void stopRecord() { recording = false; }

  ALWAYS_INLINE() void resetRecord(uint8_t *buf = NULL, uint16_t maxLen = 0) {
    if ((buf == NULL) && (data != NULL)) {
      recordBuf = data;
      maxRecordLen = max_len;
    } else if (buf) {
      recordBuf = buf;
      maxRecordLen = maxLen;
    } else {
      recordBuf = NULL;
      maxRecordLen = max_len;
    }
    recording = false;
    recordLen = 0;
  }

  ALWAYS_INLINE() void putByte(uint16_t offset, uint8_t c) {
    put_byte_bank1(sysex_highmem_buf + offset, c);
  }

  uint8_t getByte(uint16_t n) {
    if (n < maxRecordLen) {
      // Retrieve data from specified memory buffer
      if (recordBuf != NULL) {
        return recordBuf[n];

      } else {
        // Read from sysex buffers in HIGH membank
        return get_byte_bank1(sysex_highmem_buf + n);
      }
    }
    return 255;
  }

  ALWAYS_INLINE() bool recordByte(uint8_t c) {
    if (recordLen < maxRecordLen) {
      // Record data to specified memory buffer
      if (recordBuf != NULL) {
        recordBuf[recordLen++] = c;
      } else {
        // Write to sysex buffers in HIGH membank
        putByte(recordLen++, c);
      }
      return true;
    }
    return false;
  }
  bool callSysexCallBacks;
  uint16_t max_len;
  uint16_t recordLen;
  uint16_t maxRecordLen;

  uint16_t len;

  MidiUartParent *uart;

  MidiSysexListenerClass *listeners[NUM_SYSEX_SLAVES];

  MidiSysexClass(uint8_t *_data, uint16_t size, volatile uint8_t *ptr) {
    sysex_highmem_buf = ptr;
    data = _data;
    max_len = size;
    len = 0;
    aborted = false;
    recording = false;
    recordBuf = NULL;
    maxRecordLen = 0;
    sysexLongId = false;
  }

  void initSysexListeners() {
    for (int i = 0; i < NUM_SYSEX_SLAVES; i++)
      listeners[i] = NULL;
  }
  bool addSysexListener(MidiSysexListenerClass *listener) {
    for (int i = 0; i < NUM_SYSEX_SLAVES; i++) {
      if (listeners[i] == NULL || listeners[i] == listener) {
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
    len = 0;
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
  void end();
  // Handled by interrupts
  ALWAYS_INLINE() void end_immediate() {
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
      len++;
      recordByte(byte);
    }
  }

  /* @} */
};

class MididuinoSysexListenerClass : public MidiSysexListenerClass {
  /**
   * \addtogroup midi_sysex
   *
   * @{
   **/

public:
  MididuinoSysexListenerClass();
  virtual void handleByte(uint8_t byte);

#ifdef HOST_MIDIDUINO
  virtual ~MididuinoSysexListenerClass() {}
#endif

  /* @} */
};

// extern MidiSysexClass MidiSysex;
// extern MidiSysexClass MidiSysex2;
#define MidiSysex Midi.midiSysex
#define MidiSysex2 Midi2.midiSysex
extern MididuinoSysexListenerClass MididuinoSysexListener;

/* @} @} */

#endif /* MIDISYSEX_H__ */
