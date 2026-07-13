/* Copyright (c) 2009 - http://ruinwesen.com/ */

//#include "platform.h"

#include "Midi.h"
#include "MidiUart.h"
#include "MidiSysex.h"
#include "MidiClock.h"
#include "Devices/DeviceManager.h"

MidiClass::MidiClass(MidiUartClass *_uart, MidiSysexClass *_sysex) {
  midiActive = true;
  uart = _uart;
  midiSysex = _sysex;
  midiSysex->uart = uart;
  receiveChannel = 0xFF;
  init();
  uart->midi = this;
}

void MidiClass::init() {
  last_status = 0;
  in_state = midi_ignore_message;
  in_state = midi_wait_status;
  // live_state now lives in MidiUartClass for ISR performance
  uart->live_state = midi_wait_status;
}

void MidiClass::processSysex() {
    while (midiSysex->avail()) {
        uint8_t msg_rd = midiSysex->msg_rd;
        midiSysex->get_next_msg();
        sysexEnd(msg_rd);
    }
}

void MidiClass::processMidi() {
    while (((MidiUartParent*)uart)->avail()) {
        handleByte(((MidiUartParent*)uart)->m_getc());
    }
}

void MidiClass::sysexEnd(uint8_t msg_rd) {
  midiSysex->rd_cur = msg_rd;
  uint16_t len = midiSysex->get_recordLen();

  for (uint8_t n = 0; n < NUM_FORWARD_PORTS; n++) {
    MidiUartClass *fwd_uart = uart_forward[n];
    if (fwd_uart &&
        ((len + 2) < (fwd_uart->txRb->len - fwd_uart->txRb->size()))) {
      const uint16_t size = 256;
      uint8_t buf[size];
      uint16_t n = 0;
      volatile uint8_t* sysex_ptr = midiSysex->get_ptr();
      volatile uint8_t* rb_base = midiSysex->rb->buf;
      midiSysex->rb->rd = (uint16_t)(sysex_ptr - rb_base);
      fwd_uart->txRb->put_h_isr(0xF0);
      while (len) {
        if (len > size) {
          n = size;
          len -= n;
        } else {
          n = len;
          len = 0;
        }
        midiSysex->rb->get_h_isr(buf, n);
        fwd_uart->txRb->put_h_isr(buf, n);
      }
      fwd_uart->m_putc(0xF7);
    }
  }
  midiSysex->end();
}

void MidiClass::handleByte(uint8_t byte) {
again:
  if (MIDI_IS_REALTIME_STATUS_BYTE(byte)) {
    switch (byte) {
    case MIDI_START:
      MidiClock.handleMidiStart();
      break;

    case MIDI_CONTINUE:
      MidiClock.handleMidiContinue();
      break;

    case MIDI_STOP:
      MidiClock.handleMidiStop();
      break;
    }
    return;
  }

  if (!midiActive)
    return;

  switch (in_state) {
  case midi_wait_sysex:
    break;

  case midi_ignore_message:
    if (MIDI_IS_STATUS_BYTE(byte)) {
      in_state = midi_wait_status;
      goto again;
    } else {
      /* ignore */
    }
    break;

  case midi_wait_status: {
    bool running_status = false;
    if (MIDI_IS_STATUS_BYTE(byte)) {
      last_status = byte;
    } else {
      if (last_status == 0)
        break;
      running_status = true;
    }

    uint8_t status = last_status;
    if (MIDI_IS_VOICE_STATUS_BYTE(status)) {
      status = MIDI_VOICE_TYPE_NIBBLE(status);
      callback = (status >> 4) - 8;
      in_state = (status == MIDI_PROGRAM_CHANGE ||
                  status == MIDI_CHANNEL_PRESSURE)
                     ? midi_wait_byte_1
                     : midi_wait_byte_2;
    } else if (status >= MIDI_MTC_QUARTER_FRAME &&
               status <= MIDI_SONG_SELECT) {
      // F1-F3 are contiguous and keep the old midi_parse[] callback order.
      callback = MIDI_MTC_QUARTER_FRAME_CB + (status - MIDI_MTC_QUARTER_FRAME);
      in_state = status == MIDI_SONG_POSITION_PTR ? midi_wait_byte_2
                                                  : midi_wait_byte_1;
    } else if (status == MIDI_TUNE_REQUEST) {
      callback = MIDI_TUNE_REQUEST_CB;
      in_state = midi_wait_status;
    } else {
      in_state = midi_ignore_message;
      return;
    }
    msg[0] = last_status;
    in_msg_len = 1;

    if (running_status)
      goto again;
  } break;

  case midi_wait_byte_1: {
    // trying to fix bug that causes midi messages to overlap
    // if a midicallback triggered another midi event then the status was not
    // update in time and collision occured between data streamss
    in_state = midi_wait_status;

    msg[in_msg_len++] = byte;
    if (callback == MIDI_NOTE_ON_CB && msg[2] == 0) {
      callback = 0; // XXX ugly hack to recgnize NOTE on with velocity 0 as Note Off
    }

    // The AVR UART TX rings live in bank 1.  Their bulk writer switches banks
    // before copying, so its source must not be a bank-0 global such as this
    // MidiClass instance's msg buffer.  Keep a stack-local copy, as in 4.70;
    // the internal SRAM stack remains visible while bank 1 is selected.
    uint8_t forward_msg[3];
    memcpy(forward_msg, msg, sizeof(forward_msg));

    bool forwarded_cc = callback == MIDI_CC_CB;
    for (uint8_t n = 0; n < NUM_FORWARD_PORTS; n++) {
      MidiUartClass *forward_uart = uart_forward[n];
      if (forward_uart) {
        forward_uart->m_putc(forward_msg, in_msg_len);
        if (forwarded_cc) {
          device_manager.on_forwarded_cc(forward_uart, forward_msg);
        }
      }
    }

#ifdef HOST_MIDIDUINO
    messageCallback.call(msg, in_msg_len);
#endif

    if (callback < 7) {
      midiCallbacks[callback].call(msg);
    } else if (last_status == MIDI_SONG_POSITION_PTR) {
#ifndef HOST_MIDIDUINO
      MidiClock.handleSongPositionPtr(msg);
#endif
    }

    in_state = midi_wait_status;
    break;
  }

  case midi_wait_byte_2:
    msg[in_msg_len++] = byte;
    in_state = midi_wait_byte_1;
    break;
  }
}
