/* Copyright (c) 2009 - http://ruinwesen.com/ */

//#include "WProgram.h"

#include "Midi.h"
#include "MidiUart.h"
#include "MidiSysex.h"
#include "MidiClock.h"

const midi_parse_t midi_parse[] = {
    {MIDI_NOTE_OFF, midi_wait_byte_2},
    {MIDI_NOTE_ON, midi_wait_byte_2},
    {MIDI_AFTER_TOUCH, midi_wait_byte_2},
    {MIDI_CONTROL_CHANGE, midi_wait_byte_2},
    {MIDI_PROGRAM_CHANGE, midi_wait_byte_1},
    {MIDI_CHANNEL_PRESSURE, midi_wait_byte_1},
    {MIDI_PITCH_WHEEL, midi_wait_byte_2},
    /* special handling for SYSEX */
    {MIDI_MTC_QUARTER_FRAME, midi_wait_byte_1},
    {MIDI_SONG_POSITION_PTR, midi_wait_byte_2},
    {MIDI_SONG_SELECT, midi_wait_byte_1},
    {MIDI_TUNE_REQUEST, midi_wait_status},
    {0, midi_ignore_message}
};

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
  last_status = running_status = 0;
  in_state = midi_ignore_message;
  in_state = midi_wait_status;
  live_state = midi_wait_status;
}

void MidiClass::processSysex() {
    while (midiSysex->avail()) {
        sysexEnd(midiSysex->msg_rd);
        midiSysex->get_next_msg();
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
  DEBUG_FUNC();
  DEBUG_PRINTLN(byte);
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
  case midi_ignore_message:
    if (MIDI_IS_STATUS_BYTE(byte)) {
      in_state = midi_wait_status;
      goto again;
    } else {
      /* ignore */
    }
    break;

  case midi_wait_status: {
    if (MIDI_IS_STATUS_BYTE(byte)) {
      last_status = byte;
      running_status = 0;
    } else {
      if (last_status == 0)
        break;
      running_status = 1;
    }

    uint8_t status = last_status;
    if (MIDI_IS_VOICE_STATUS_BYTE(status)) {
      status = MIDI_VOICE_TYPE_NIBBLE(status);
    }

    uint8_t i;
    for (i = 0; midi_parse[i].midi_status != 0; i++) {
      if (midi_parse[i].midi_status == status) {
        in_state = midi_parse[i].next_state;
        msg[0] = last_status;
        in_msg_len = 1;
        break;
      }
    }
    callback = i;

    if (midi_parse[i].midi_status == 0) {
      in_state = midi_ignore_message;
      return;
    }
    if (running_status)
      goto again;
  } break;

  case midi_wait_byte_1:
    // trying to fix bug that causes midi messages to overlap
    // if a midicallback triggered another midi event then the status was not
    // update in time and collision occured between data streamss
    in_state = midi_wait_status;

    msg[in_msg_len++] = byte;
    if (midi_parse[callback].midi_status == MIDI_NOTE_ON && msg[2] == 0) {
      callback = 0; // XXX ugly hack to recgnize NOTE on with velocity 0 as Note Off
    }

    uint8_t buf[3];
    memcpy(buf, msg, 3);

    for (uint8_t n = 0; n < NUM_FORWARD_PORTS; n++) {
      if (uart_forward[n]) {
        uart_forward[n]->m_putc(buf, in_msg_len);
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

  case midi_wait_byte_2:
    msg[in_msg_len++] = byte;
    in_state = midi_wait_byte_1;
    break;
  }
}
