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
  uint8_t msg_rd;

  MidiSysexListenerClass(MidiSysexClass *_sysex = NULL) {
    sysex = _sysex;
    ids[0] = 0;
    ids[1] = 0;
    ids[2] = 0;
    msgType = 255;
    msg_rd = 255;
  };
  /* Point MidiSysexClass to the last sysex message matching this listener's
   * message ids */
  virtual void start() {}
  virtual void end() {}
  virtual void end_immediate() {}

#ifdef HOST_MIDIDUINO
  virtual ~MidiSysexListenerClass() {}
#endif

  /* @} */
};

#define NUM_SYSEX_SLAVES 4

#define NUM_SYSEX_MSGS 24

class MidiSysexLedger {
public:
  uint8_t state : 2;
  uint16_t recordLen : 14; // 16383 max record length
  uint8_t *ptr;
};

#define SYSEX_STATE_NULL 0
#define SYSEX_STATE_REC 1
#define SYSEX_STATE_FIN 2

class MidiSysexClass {
  /**
   * \addtogroup midi_sysex
   *
   * @{
   **/

protected:
  bool recording;
  uint8_t recvIds[3];
  bool sysexLongId;

public:
  volatile uint8_t msg_wr;
  volatile uint8_t msg_rd;

  volatile RingBuffer<0, uint16_t> Rb;

  volatile uint8_t *sysex_highmem_buf;
  uint16_t sysex_bufsize;
  MidiUartParent *uart;

  MidiSysexLedger ledger[NUM_SYSEX_MSGS];
  volatile uint8_t rd_cur;

  MidiSysexListenerClass *listeners[NUM_SYSEX_SLAVES];

  MidiSysexClass(MidiUartParent *_uart, uint16_t size, volatile uint8_t *ptr) {
    uart = _uart;
    recording = false;
    sysexLongId = false;

    memset(ledger, 0, sizeof(ledger));
    msg_wr = 0;
    msg_rd = 0;

    Rb.ptr = ptr;
    Rb.len = size;
#ifdef DEBUGMODE
    Rb.check = false;
#endif
  }
  ALWAYS_INLINE() uint16_t get_recordLen() { return ledger[rd_cur].recordLen; }
  ALWAYS_INLINE() volatile uint8_t *get_ptr() { return ledger[rd_cur].ptr; }

  bool avail() {
    return ((msg_wr != msg_rd) && ledger[msg_rd].state == SYSEX_STATE_FIN &&
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
    uint8_t n = msg_rd;
    msg_rd++;
    if (msg_rd == NUM_SYSEX_MSGS) {
      msg_rd = 0;
    }
  }

  ALWAYS_INLINE() void startRecord() {
    recording = true;
    ledger[msg_wr].recordLen = 0;
    ledger[msg_wr].ptr = (Rb.ptr + Rb.wr);
    ledger[msg_wr].state = SYSEX_STATE_REC;
  }

  ALWAYS_INLINE() void stopRecord() {
    recording = false;
    ledger[msg_wr].state = SYSEX_STATE_FIN;
    if (is_full()) {
      DEBUG_PRINTLN("WRITE FULL!!!!");
      return;
    }
    // DEBUG_PRINTLN("record fin");
    // DEBUG_PRINTLN(ledger[msg_wr].recordLen);
    msg_wr++;

    if (msg_wr == NUM_SYSEX_MSGS) {
      msg_wr = 0;
    }
  }

  ALWAYS_INLINE() void putByte(uint16_t n, uint8_t c) {
    uint16_t r = (uint16_t) ledger[rd_cur].ptr - (uint16_t) Rb.ptr;
    if (r + n > Rb.len - 1) {
      n = n - Rb.len;
    }
    volatile uint8_t *dst = ledger[rd_cur].ptr + n;
    DEBUG_PRINTLN("HEREEE");
    put_bank1(dst, c);
  }
  ALWAYS_INLINE() void putByte(uint8_t c) { Rb.put_h_isr(c); }

  ALWAYS_INLINE() uint8_t getByte(uint16_t n) {
    uint16_t r = (uint16_t) ledger[rd_cur].ptr - (uint16_t) Rb.ptr;
    if (r + n > Rb.len - 1) {
      n = n - Rb.len;
    }
    volatile uint8_t *src = ledger[rd_cur].ptr + n;
    return get_bank1(src);
  }

  ALWAYS_INLINE() bool recordByte(uint8_t c) {
    putByte(c);
    ledger[msg_wr].recordLen++;
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
    } else {
      if (recvIds[0] == listener->ids[0])
        return true;
    }
    return false;
  }

  ALWAYS_INLINE() void abort() {
    DEBUG_PRINTLN("aborting");
    recording = false;
    memset(&ledger[msg_wr], 0, sizeof(MidiSysexLedger));
  }

  ALWAYS_INLINE() void reset() { startRecord(); }

  // Handled by main loop
  void end() {
    recvIds[0] = getByte(0);
    sysexLongId = false;
    if (recvIds[0] == 0x00) {
      sysexLongId = true;
      recvIds[1] = getByte(1);
      recvIds[2] = getByte(2);
    }
    for (int i = 0; i < NUM_SYSEX_SLAVES; i++) {
      if (isListenerActive(listeners[i])) {
        listeners[i]->msg_rd = rd_cur;
        listeners[i]->end();
      }
    }
  }

  // Handled by interrupts
  ALWAYS_INLINE() void end_immediate() {

    uint8_t old_msg = rd_cur;
    rd_cur = msg_wr;

    stopRecord();

    recvIds[0] = getByte(0);

    sysexLongId = false;
    if (recvIds[0] == 0x00) {
      sysexLongId = true;
      recvIds[1] = getByte(1);
      recvIds[2] = getByte(2);
    }

    for (int i = 0; i < NUM_SYSEX_SLAVES; i++) {
      if (isListenerActive(listeners[i])) {
        listeners[i]->msg_rd = rd_cur;
        listeners[i]->end_immediate();
      }
    }
    rd_cur = old_msg;
  }

  ALWAYS_INLINE() void handleByte(uint8_t byte) {
    if (recording) {
      //if (ledger[msg_rd].state == SYSEX_STATE_FIN && ledger[msg_rd].ptr == Rb.ptr + Rb.wr) { abort(); return; }
      //memset(&ledger[msg_rd],0,sizeof(MidiSysexLedger));
      //get_next_msg();
      recordByte(byte);
    }
   }

  /* @} */
};

// extern MidiSysexClass MidiSysex;
// extern MidiSysexClass MidiSysex2;
#define MidiSysex Midi.midiSysex
#define MidiSysex2 Midi2.midiSysex

/* @} @} */

#endif /* MIDISYSEX_H__ */
