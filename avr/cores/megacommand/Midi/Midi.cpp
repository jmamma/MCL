/* Copyright (c) 2009 - http://ruinwesen.com/ */

#include "WProgram.h"

#include "Midi.h"
#include "MidiClock.h"

const midi_parse_t midi_parse[] = {{MIDI_NOTE_OFF, midi_wait_byte_2},
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
                                   {0, midi_ignore_message}};

MidiClass::MidiClass(MidiUartParent *_uart, uint16_t _sysexBufLen,
                     volatile uint8_t *ptr)
    : midiSysex(_uart, _sysexBufLen, ptr) {
  midiActive = true;
  uart = _uart;
  receiveChannel = 0xFF;
  init();
}

void MidiClass::init() {
  last_status = running_status = 0;
  in_state = midi_ignore_message;
  in_state = midi_wait_status;
  live_state = midi_wait_status;
}

void MidiClass::sysexEnd(uint8_t msg_rd) {
  midiSysex.rd_cur = msg_rd;
  uint16_t len = midiSysex.get_recordLen();
  DEBUG_CHECK_STACK();
  DEBUG_PRINTLN("processing");
  DEBUG_PRINTLN(SP);
  DEBUG_PRINTLN(msg_rd);
  DEBUG_PRINTLN(midiSysex.msg_wr);

  DEBUG_PRINTLN(uart_forward->txRb.len - uart_forward->txRb.size());
  //if (len == 0 || midiSysex.get_ptr() == nullptr) { DEBUG_PRINTLN("returning"); return; }

  if (uart_forward && ((len + 2) < (uart_forward->txRb.len -
                                           uart_forward->txRb.size()))) {
   const uint16_t size = 2048;
    uint8_t buf[size];
    uint16_t n = 0;
    midiSysex.Rb.rd = (uint16_t) midiSysex.get_ptr() - (uint16_t) midiSysex.Rb.ptr;
    uart_forward->txRb.put_h_isr(0xF0);
    while (len) {
      if (len > size) {
        n = size;
        len -= n;
      } else {
        n = len;
        len = 0;
      }
      midiSysex.Rb.get_h_isr(buf, n); //we don't worry about the Rb.rd increase, as it wont be used anywhere else
      uart_forward->txRb.put_h_isr(buf, n);
    }
    uart_forward->m_putc(0xF7);
  }
  midiSysex.end();
}

void MidiClass::handleByte(uint8_t byte) {
again:
  if (MIDI_IS_REALTIME_STATUS_BYTE(byte)) {

    //    if (MidiClock.mode == MidiClock.EXTERNAL_MIDI) {
    switch (byte) {
      //  case MIDI_CLOCK:
      //			MidiClock.handleClock();
      //			break;

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
    //  }
    if (byte == MIDI_ACTIVE_SENSE) {
      uint8_t tmp_msg[1];
      tmp_msg[0] = uart->uart_port;

      uart->recvActiveSenseCallbacks.call((uint8_t *)&tmp_msg);
      uart->recvActiveSenseTimer = 0;
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
    /*
      case midi_wait_sysex:

        if (MIDI_IS_STATUS_BYTE(byte)) {
          if (byte != MIDI_SYSEX_END) {
                                    in_state = midi_wait_status;
                                    midiSysex.abort();
                                    goto again;
           } else {
                  midiSysex.end();
          }
        } else {
          midiSysex.handleByte(byte);
        }
        break;
    */
  case midi_wait_status: {
    //   if (byte == MIDI_SYSEX_START) {
    //			in_state = midi_wait_sysex;
    //			midiSysex.reset();
    //			last_status = running_status = 0;
    //			return;
    // }

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
      callback =
          0; // XXX ugly hack to recgnize NOTE on with velocity 0 as Note Off
    }

    uint8_t buf[3];
    memcpy(buf, msg, 3);
    if (uart_forward) {
      uart_forward->m_putc(buf, in_msg_len);
    }

#ifdef HOST_MIDIDUINO
    messageCallback.call(msg, in_msg_len);
#endif

    if (callback < 7) {
      midiCallbacks[callback].call(msg);
#if 0
			Vector<midi_callback_func_t, 4> *vec = midiCallbackFunctions + callback;
			for (int i = 0; i < vec->size; i++) {
				MidiUart.printfString("callback %b, vec %b", callback, i);
				if (vec->arr[i] != NULL) {
					MidiUart.printfString("not empty");
					(vec->arr[i])(msg);
				}
			}
#endif
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
