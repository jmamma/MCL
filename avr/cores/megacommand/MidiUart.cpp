#define IS_ISR_ROUTINE

#include "WProgram.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

#include <MidiUart.h>
#include <MidiUartParent.h>
#include <midi-common.h>

#include <MidiClock.h>
#include <avr/io.h>

MidiUartClass::MidiUartClass(volatile uint8_t *udr_, volatile uint8_t *rx_buf,
                             uint16_t rx_buf_size, volatile uint8_t *tx_buf,
                             uint16_t tx_buf_size)
    : MidiUartParent() {
  udr = udr_;
  mode = UART_MIDI;
  rxRb.ptr = rx_buf;
  rxRb.len = rx_buf_size;
  txRb.ptr = tx_buf;
  txRb.len = tx_buf_size;
  // ignore side channel;
  txRb_sidechannel = nullptr;
  initSerial();
}

void MidiUartClass::initSerial() {
  running_status = 0;
  set_speed(31250);
  volatile uint8_t *src = ucsrc();
  volatile uint8_t *srb = ucsrb();

  *src = (3 << UCSZ00);
  *srb = _BV(RXEN0) | _BV(TXEN0) | _BV(RXCIE0);
}

void MidiUartClass::set_speed(uint32_t speed_) {
  // empty TX buffer before switching speed
  // UBRR1H = ((cpu >> 8) & 0xFF);
  // UBRR1L = (cpu & 0xFF);
  while (!txRb.isEmpty())
    ;

  uint32_t cpu = (F_CPU / 16);
  cpu /= speed_;
  cpu--;

  volatile uint8_t *ubrrh_ = ubrrh();
  volatile uint8_t *ubrrl_ = ubrrl();

  *ubrrh_ = ((cpu >> 8) & 0xFF);
  *ubrrl_ = (cpu & 0xFF);

  speed = speed_;
}

void MidiUartClass::m_putc_immediate(uint8_t c) {
  USE_LOCK();
  SET_LOCK();
  while (!check_empty_tx()) {
    if (TIMER1_CHECK_INT()) {
      TCNT1 = 0;
      clock++;
      TIMER1_CLEAR_INT()
    }
    if (TIMER3_CHECK_INT()) {
      TCNT2 = 0;
      slowclock++;
      TIMER3_CLEAR_INT()
    }
  }
  sendActiveSenseTimer = sendActiveSenseTimeout;
  write_char(c);
  CLEAR_LOCK();
}

void MidiUartClass::rx_isr() {
  uint8_t c = read_char();
  if (MIDI_IS_REALTIME_STATUS_BYTE(c)) {
    recvActiveSenseTimer = 0;
    if (c == MIDI_CLOCK) {
      if (MidiClock.uart_clock_recv == this) {
        MidiClock.handleClock();
        MidiClock.callCallbacks(true);
      }
    } else if (MidiClock.uart_transport_recv1 == this ||
               MidiClock.uart_transport_recv2 == this) {
      switch (c) {
      case MIDI_START:
        MidiClock.handleImmediateMidiStart();
        break;

      case MIDI_STOP:
        MidiClock.handleImmediateMidiStop();
        break;

      case MIDI_CONTINUE:
        MidiClock.handleImmediateMidiContinue();
        break;
      }
      rxRb.put_h_isr(c);
    }
    return;
  }

  if (MIDI_IS_STATUS_BYTE(c)) {
    recvActiveSenseTimer = 0;
  }
  switch (midi->live_state) {
  case midi_wait_sysex: {

    if (MIDI_IS_STATUS_BYTE(c)) {
      if (c != MIDI_SYSEX_END) {
        midi->midiSysex.abort();
        rxRb.put_h_isr(c);
      } else {
        midi->midiSysex.end_immediate();
      }
      midi->live_state = midi_wait_status;
    } else {
      // record
      recvActiveSenseTimer = 0;
      midi->midiSysex.handleByte(c);
    }
    break;
  }

  case midi_wait_status: {
    if (c == MIDI_SYSEX_START) {
      midi->live_state = midi_wait_sysex;
      midi->midiSysex.reset();
    } else {
      rxRb.put_h_isr(c);
    }
  } break;
  default: {
    rxRb.put_h_isr(c);
    break;
  }
  }
}

void MidiUartClass::tx_isr() {
  if ((txRb_sidechannel != nullptr) && (in_message_tx == 0)) {
    // sidechannel mounted, and no active messages in normal channel
    // ==> flush the sidechannel now
    if (!txRb_sidechannel->isEmpty_isr()) {
      sendActiveSenseTimer = sendActiveSenseTimeout;
      uint8_t c = txRb_sidechannel->get_h_isr();
      write_char(c);
    }
    // unmount sidechannel if drained
    if (txRb_sidechannel->isEmpty_isr()) {
      txRb_sidechannel = nullptr;
    }
  } else if (!txRb.isEmpty_isr()) {
    // 1. either sidechannel is unmounted, or an active message is in normal
    // channel
    // 2. -and- a normal channel byte is queued
    // ==> flush the normal channel now
    sendActiveSenseTimer = sendActiveSenseTimeout;
    uint8_t c = txRb.get_h_isr();
    write_char(c);
    if ((in_message_tx > 0) && (c < 128)) {
      in_message_tx--;
    }
    if (c < 0xF0) {
      switch (c & 0xF0) {
      case MIDI_CHANNEL_PRESSURE:
      case MIDI_PROGRAM_CHANGE:
      case MIDI_MTC_QUARTER_FRAME:
      case MIDI_SONG_SELECT:
        in_message_tx = 1;
        break;
      case MIDI_NOTE_OFF:
      case MIDI_NOTE_ON:
      case MIDI_AFTER_TOUCH:
      case MIDI_CONTROL_CHANGE:
      case MIDI_PITCH_WHEEL:
      case MIDI_SONG_POSITION_PTR:
        in_message_tx = 2;
        break;
      }
    } else {
      switch (c) {
      case MIDI_SYSEX_START:
        in_message_tx = -1;
        break;
      case MIDI_SYSEX_END:
        in_message_tx = 0;
        break;
      }
    }
  } else {
    // 1. either sidechannel is unmounted, or an active message is in normal
    // channel
    // 2. -and- normal channel is drained
    // ==> clear active bit and wait for normal channel to be re-supplied
    clear_tx();
  }
  if (txRb.isEmpty_isr() && (txRb_sidechannel == nullptr)) {
    clear_tx();
  }
}

ISR(USART0_RX_vect) {
  select_bank(0);
  if (MidiUartUSB.mode == UART_MIDI) {
    MidiUartUSB.rx_isr();
  } else {
    Serial._rx_complete_irq();
  }
}

ISR(USART0_UDRE_vect) {
  select_bank(0);
  if (MidiUartUSB.mode == UART_MIDI) {
    MidiUartUSB.tx_isr();
  } else {
    Serial._tx_udr_empty_irq();
  }
}

ISR(USART1_RX_vect) {
  select_bank(0);
  MidiUart.rx_isr();
}

ISR(USART1_UDRE_vect) {
  select_bank(0);
  MidiUart.tx_isr();
}

ISR(USART2_RX_vect) {
  select_bank(0);
  MidiUart2.rx_isr();
}

ISR(USART2_UDRE_vect) {
  select_bank(0);
  MidiUart2.tx_isr();
}
