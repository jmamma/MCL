/* Copyright (c) 2009 - http://ruinwesen.com/ */

#include "WProgram.h"

#include "Midi.h"
#include "MidiClock.h"

const midi_parse_t midi_parse[] = {
  { MIDI_NOTE_OFF,         midi_wait_byte_2 },
  { MIDI_NOTE_ON,          midi_wait_byte_2 },
  { MIDI_AFTER_TOUCH,      midi_wait_byte_2 },
  { MIDI_CONTROL_CHANGE,   midi_wait_byte_2 },
  { MIDI_PROGRAM_CHANGE,   midi_wait_byte_1 },
  { MIDI_CHANNEL_PRESSURE, midi_wait_byte_1 },
  { MIDI_PITCH_WHEEL,      midi_wait_byte_2 },
  /* special handling for SYSEX */
  { MIDI_MTC_QUARTER_FRAME, midi_wait_byte_1 },
  { MIDI_SONG_POSITION_PTR, midi_wait_byte_2 },
  { MIDI_SONG_SELECT,       midi_wait_byte_1 },
  { MIDI_TUNE_REQUEST,      midi_wait_status },
  { 0, midi_ignore_message}
};

MidiClass::MidiClass(MidiUartParent *_uart, uint8_t *_sysexBuf, uint16_t _sysexBufLen)
	: midiSysex(_sysexBuf, _sysexBufLen) {
	sysexBuf = _sysexBuf;
	sysexBufLen = _sysexBufLen;
  midiActive = true;
  uart = _uart;
  receiveChannel = 0xFF;
  init();
}

void MidiClass::init() {
  last_status = running_status = 0;
  in_state = midi_ignore_message;
}

void MidiClass::handleByte(uint8_t byte) {
 again:
  if (MIDI_IS_REALTIME_STATUS_BYTE(byte)) {

#ifdef HOST_MIDIDUINO
    USE_LOCK();
    SET_LOCK();
    
    if (MidiClock.mode == MidiClock.EXTERNAL_MIDI) {
      switch (byte) {
      case MIDI_CLOCK:
				MidiClock.handleClock();
				break;
	
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
    }

    CLEAR_LOCK();
#endif

		if (byte == MIDI_ACTIVE_SENSE) {
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

  case midi_wait_status:
    {
      if (byte == MIDI_SYSEX_START) {
				in_state = midi_wait_sysex;
				midiSysex.reset();
				last_status = running_status = 0;
				return;
      }

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
    }
    break;

  case midi_wait_byte_1:
    // trying to fix bug that causes midi messages to overlap
    // if a midicallback triggered another midi event then the status was not update in time
    // and collision occured between data streamss
    in_state = midi_wait_status;
 
    msg[in_msg_len++] = byte;
    if (midi_parse[callback].midi_status == MIDI_NOTE_ON && msg[2] == 0) {
      callback = 0; // XXX ugly hack to recgnize NOTE on with velocity 0 as Note Off
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
