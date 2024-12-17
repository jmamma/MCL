/*c Copyright (c) 2009 - http://ruinwesen.com/ */

#pragma once

#include <inttypes.h>
#include "RingBuffer.h"
#include "memory.h"
class MidiUartParent;
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
public:
  uint8_t ids[3];
  MidiSysexClass* sysex;
  uint8_t msgType;
  uint8_t msg_rd;

  MidiSysexListenerClass(MidiSysexClass* _sysex = NULL) {
    sysex = _sysex;
    ids[0] = 0;
    ids[1] = 0;
    ids[2] = 0;
    msgType = 255;
    msg_rd = 255;
  }

  virtual void start() {}
  virtual void end() {}
  virtual void end_immediate() {}

#ifdef HOST_MIDIDUINO
  virtual ~MidiSysexListenerClass() {}
#endif
};

#define NUM_SYSEX_SLAVES 5
#define NUM_SYSEX_MSGS 24

class MidiSysexLedger {
public:
  uint8_t state : 2;
  uint16_t recordLen : 14; // 16383 max record length
  volatile uint8_t* ptr;
};

#define SYSEX_STATE_NULL 0
#define SYSEX_STATE_REC 1
#define SYSEX_STATE_FIN 2

class MidiSysexClass {
protected:
  bool recording;
  uint8_t recvIds[3];
  bool sysexLongId;

public:
  volatile uint8_t msg_wr;
  volatile uint8_t msg_rd;

  RingBuffer<>* rb;

  volatile uint8_t* sysex_highmem_buf;
  uint16_t sysex_bufsize;
  MidiUartParent* uart;

  MidiSysexLedger ledger[NUM_SYSEX_MSGS];
  volatile uint8_t rd_cur;

  MidiSysexListenerClass* listeners[NUM_SYSEX_SLAVES];

  MidiSysexClass(MidiUartParent* _uart, RingBuffer<>* _rb) {
    uart = _uart;
    recording = false;
    sysexLongId = false;

    memset(ledger, 0, sizeof(ledger));
    msg_wr = 0;
    msg_rd = 0;
    rb = _rb;
  }

  uint16_t get_recordLen() { return ledger[rd_cur].recordLen; }
  volatile uint8_t* get_ptr() { return ledger[rd_cur].ptr; }

  bool avail() {
    return ((msg_wr != msg_rd) &&
            ledger[msg_rd].state == SYSEX_STATE_FIN &&
            ledger[msg_rd].recordLen != 0);
  }

  bool is_full() {
    uint8_t msg_next = msg_wr + 1;
    if (msg_next == NUM_SYSEX_MSGS) {
      msg_next = 0;
    }
    return (msg_next == msg_rd);
  }

  void get_next_msg() {
    msg_rd++;
    if (msg_rd == NUM_SYSEX_MSGS) {
      msg_rd = 0;
    }
  }

  void startRecord() {
    recording = true;
    ledger[msg_wr].recordLen = 0;
    ledger[msg_wr].ptr = rb->buf + rb->wr;
    ledger[msg_wr].state = SYSEX_STATE_REC;
  }

  void stopRecord() {
    recording = false;
    ledger[msg_wr].state = SYSEX_STATE_FIN;
    msg_wr++;
    if (msg_wr == NUM_SYSEX_MSGS) {
      msg_wr = 0;
    }
  }

  void putByte(uint16_t n, uint8_t c) {
    uint16_t r = (uint16_t)((uint8_t*)ledger[rd_cur].ptr - rb->buf);
    if (r + n > rb->len - 1) {
      n = n - rb->len;
    }
    volatile uint8_t* dst = ledger[rd_cur].ptr + n;
    put_bank1(dst, c);
  }

  void putByte(uint8_t c) { rb->put_h_isr(c); }

  uint8_t getByte(uint16_t n) {
    uint16_t r = (uint16_t)((uint8_t*)ledger[rd_cur].ptr - rb->buf);
    if (r + n > rb->len - 1) {
      n = n - rb->len;
    }
    volatile uint8_t* src = ledger[rd_cur].ptr + n;
    return get_bank1(src);
  }

  void recordByte(uint8_t c) {
    putByte(c);
    ledger[msg_wr].recordLen++;
  }

  void initSysexListeners() {
    for (uint8_t i = 0; i < NUM_SYSEX_SLAVES; i++)
      listeners[i] = NULL;
  }

  bool addSysexListener(MidiSysexListenerClass* listener) {
    for (uint8_t i = 0; i < NUM_SYSEX_SLAVES; i++) {
      if (listeners[i] == listener) {
        return true;
      }
      if (listeners[i] == NULL) {
        listeners[i] = listener;
        listener->sysex = this;
        return true;
      }
    }
    return false;
  }

  void removeSysexListener(MidiSysexListenerClass* listener) {
    for (uint8_t i = 0; i < NUM_SYSEX_SLAVES; i++) {
      if (listeners[i] == listener)
        listeners[i] = NULL;
    }
  }

  bool isListenerActive(MidiSysexListenerClass* listener) {
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
    } else {
      if (recvIds[0] == listener->ids[0])
        return true;
    }
    return false;
  }

  void abort() {
    recording = false;
    memset(&ledger[msg_wr], 0, sizeof(MidiSysexLedger));
  }

  void reset() { startRecord(); }

  // Handled by main loop
  void end() {
    recvIds[0] = getByte(0);
    sysexLongId = false;
    if (recvIds[0] == 0x00) {
      sysexLongId = true;
      recvIds[1] = getByte(1);
      recvIds[2] = getByte(2);
    }
    for (uint8_t i = 0; i < NUM_SYSEX_SLAVES; i++) {
      if (isListenerActive(listeners[i])) {
        listeners[i]->msg_rd = rd_cur;
        listeners[i]->end();
      }
    }
  }

  // Handled by interrupts
  void end_immediate() { stopRecord(); }

  void handleByte(uint8_t byte) {
    if (recording) {
      recordByte(byte);
    }
  }
};
