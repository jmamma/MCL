#define IS_ISR_ROUTINE

#include "WProgram.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

#include <MidiUart.h>
#include <MidiUartParent.h>
#include <midi-common.h>

#include <MidiClock.h>
// extern MidiClockClass MidiClock;
#include <avr/io.h>
uint16_t clock_measure = 0;

MidiUartClassCommon::MidiUartClassCommon(volatile uint8_t *rx_buf,
                                         uint16_t rx_buf_size,
                                         volatile uint8_t *tx_buf,
                                         uint16_t tx_buf_size)
    : MidiUartParent() {
  rxRb.ptr = rx_buf;
  rxRb.len = rx_buf_size;
  txRb.ptr = tx_buf;
  txRb.len = tx_buf_size;
  // ignore side channel;
  txRb_sidechannel = nullptr;
  initSerial();
}

void MidiUartClassCommon::initSerial() {
  running_status = 0;
  set_speed(31250, 1);

#ifdef MEGACOMMAND
  UCSR1C = (3 << UCSZ00);

  /** enable receive, transmit and receive and transmit interrupts. **/
  UCSR1B = _BV(RXEN0) | _BV(TXEN0) | _BV(RXCIE0);
#else
  UCSR0C = (3 << UCSZ00);

  UCSR0B = _BV(RXEN0) | _BV(TXEN0) | _BV(RXCIE0);
#endif
  set_speed(31250, 2);
#ifdef MEGACOMMAND
  UCSR2C = (3 << UCSZ00);

  /** enable receive, transmit and receive and transmit interrupts. **/
  UCSR2B = _BV(RXEN1) | _BV(TXEN1) | _BV(RXCIE1);
#else

#ifdef UART2_TX
  USCR1B = _BV(RXEN1) | _BV(TXEN1) | _BV(RXCIE1);
#else
  UCSR1B = _BV(RXEN1) | _BV(RXCIE1);
#endif
#endif
}

void MidiUartClassCommon::set_speed(uint32_t speed, uint8_t port) {
#ifdef TX_IRQ
  // empty TX buffer before switching speed
  if (port == 1) {
    while (!txRb.isEmpty())
      ;
  }
  if (port == 2) {
    while (!MidiUart2.txRb.isEmpty())
      ;
  }
#endif

  uint32_t cpu = (F_CPU / 16);
  cpu /= speed;
  cpu--;

  // uint32_t cpu = (F_CPU / 16);
  // cpu /= speed;
  // cpu--;
  // UBRR0H = ((cpu >> 8));
#ifdef MEGACOMMAND
  if (port == 1) {
    UBRR1H = ((cpu >> 8) & 0xFF);
    UBRR1L = (cpu & 0xFF);
    MidiUart.speed = speed;
  }
  if (port == 2) {
    UBRR2H = ((cpu >> 8) & 0xFF);
    UBRR2L = (cpu & 0xFF);
    MidiUart2.speed = speed;
  }
#else
  if (port == 1) {
    UBRR0H = ((cpu >> 8) & 0xFF);
    UBRR0L = (cpu & 0xFF);
    MidiUart.speed = speed;
  }
  if (port == 2) {
    UBRR1H = ((cpu >> 8) & 0xFF);
    UBRR1L = (cpu & 0xFF);
    MidiUart2.speed = speed;
  }

#endif
}

void MidiUartClass2::m_putc_immediate(uint8_t c) {

#ifdef UART2_TX
  if (!IN_IRQ()) {
    USE_LOCK();
    SET_LOCK();
    // block interrupts
    while (!UART2_CHECK_EMPTY_BUFFER()) {
#ifdef MEGACOMMAND
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
#endif
    }

    MidiUart2.sendActiveSenseTimer = MidiUart2.sendActiveSenseTimeout;
    UART2_WRITE_CHAR(c);
    CLEAR_LOCK();
  } else {
    while (!UART2_CHECK_EMPTY_BUFFER()) {
#ifdef MEGACOMMAND
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
#endif
    }

    MidiUart2.sendActiveSenseTimer = MidiUart2.sendActiveSenseTimeout;
    UART2_WRITE_CHAR(c);
  }
#endif
}

void MidiUartClass::m_putc_immediate(uint8_t c) {
  //  m_putc(c);
  if (!IN_IRQ()) {
    USE_LOCK();
    SET_LOCK();
    // block interrupts
    while (!UART_CHECK_EMPTY_BUFFER()) {
#ifdef MEGACOMMAND
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
#endif
    }

    MidiUart.sendActiveSenseTimer = MidiUart.sendActiveSenseTimeout;
    UART_WRITE_CHAR(c);
    CLEAR_LOCK();
  } else {
    while (!UART_CHECK_EMPTY_BUFFER()) {
#ifdef MEGACOMMAND
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
#endif
    }

    MidiUart.sendActiveSenseTimer = MidiUart.sendActiveSenseTimeout;
    UART_WRITE_CHAR(c);
  }
}
#ifdef MEGACOMMAND
ISR(USART1_RX_vect) {
#else
ISR(USART0_RX_vect) {
#endif
  select_bank(0);
  uint8_t c = UART_READ_CHAR();
  if (MIDI_IS_REALTIME_STATUS_BYTE(c)) {

    MidiUart.recvActiveSenseTimer = 0;
    if (MidiClock.mode == MidiClock.EXTERNAL_UART1) {

      if (c == MIDI_CLOCK) {
        MidiClock.handleClock();
        MidiClock.callCallbacks(true);
      } else {
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
        MidiUart.rxRb.put_h_isr(c);
      }
    }
    return;
  }

  if (MIDI_IS_STATUS_BYTE(c)) {
    MidiUart.recvActiveSenseTimer = 0;
  }
  switch (Midi.live_state) {
  case midi_wait_sysex: {

    if (MIDI_IS_STATUS_BYTE(c)) {
      if (c != MIDI_SYSEX_END) {
        Midi.midiSysex.abort();
        MidiUart.rxRb.put_h_isr(c);
      } else {
        Midi.midiSysex.end_immediate();
      }
      Midi.live_state = midi_wait_status;
    } else {
      // record
      Midi.midiSysex.handleByte(c);
    }
    break;
  }

  case midi_wait_status: {
    if (c == MIDI_SYSEX_START) {
      Midi.live_state = midi_wait_sysex;
      Midi.midiSysex.reset();
    } else {
      MidiUart.rxRb.put_h_isr(c);
    }
  } break;
  default: {
    MidiUart.rxRb.put_h_isr(c);
    break;
  }
  }
}

#ifdef MEGACOMMAND
ISR(USART2_RX_vect) {
#else
ISR(USART1_RX_vect) {
#endif
  select_bank(0);

  uint8_t c = UART2_READ_CHAR();
  if (MIDI_IS_REALTIME_STATUS_BYTE(c)) {

    MidiUart2.recvActiveSenseTimer = 0;
    if (((MidiClock.mode == MidiClock.EXTERNAL_UART2))) {

      if (c == MIDI_CLOCK) {
        MidiClock.handleClock();
        MidiClock.callCallbacks(true);
      } else {
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
        MidiUart2.rxRb.put_h_isr(c);
      }
    }
    return;
  }

  if (MIDI_IS_STATUS_BYTE(c)) {
    MidiUart2.recvActiveSenseTimer = 0;
  }
  switch (Midi2.live_state) {
  case midi_wait_sysex: {

    if (MIDI_IS_STATUS_BYTE(c)) {
      if (c != MIDI_SYSEX_END) {
        Midi2.midiSysex.abort();
        MidiUart2.rxRb.put_h_isr(c);
      } else {
        Midi2.midiSysex.end_immediate();
      }
      Midi2.live_state = midi_wait_status;
    } else {
      // record
      Midi2.midiSysex.handleByte(c);
    }
    break;
  }
  case midi_wait_status: {
    if (c == MIDI_SYSEX_START) {
      Midi2.live_state = midi_wait_sysex;
      Midi2.midiSysex.reset();
    } else {
      MidiUart2.rxRb.put_h_isr(c);
    }
    break;
  }
  default: {
    MidiUart2.rxRb.put_h_isr(c);
    break;
  }
  }
}

#ifdef TX_IRQ

#ifdef MEGACOMMAND
ISR(USART1_UDRE_vect) {
#else
ISR(USART0_UDRE_vect) {
#endif
  select_bank(0);
  if ((MidiUart.txRb_sidechannel != nullptr) && (MidiUart.in_message_tx == 0)) {

    if (!MidiUart.txRb_sidechannel->isEmpty_isr()) {
      MidiUart.sendActiveSenseTimer = MidiUart.sendActiveSenseTimeout;
      uint8_t c = MidiUart.txRb_sidechannel->get_h_isr();
      UART_WRITE_CHAR(c);
    }
    if (MidiUart.txRb_sidechannel->isEmpty_isr()) {
      MidiUart.txRb_sidechannel = nullptr;
    }
  } else {
    if (!MidiUart.txRb.isEmpty_isr()) {
      MidiUart.sendActiveSenseTimer = MidiUart.sendActiveSenseTimeout;
      uint8_t c = MidiUart.txRb.get_h_isr();
      UART_WRITE_CHAR(c);
      if ((MidiUart.in_message_tx > 0) && (c < 128)) {
        MidiUart.in_message_tx--;
      }
      if (c < 0xF0) {
        switch (c & 0xF0) {
        case MIDI_CHANNEL_PRESSURE:
        case MIDI_PROGRAM_CHANGE:
          MidiUart.in_message_tx = 1;
          break;
        case MIDI_NOTE_OFF:
        case MIDI_NOTE_ON:
        case MIDI_AFTER_TOUCH:
        case MIDI_CONTROL_CHANGE:
        case MIDI_PITCH_WHEEL:
          MidiUart.in_message_tx = 2;
          break;
        }
      } else {
        switch (c) {
        case MIDI_SYSEX_START:
          MidiUart.in_message_tx = -1;
          break;
        case MIDI_SYSEX_END:
          MidiUart.in_message_tx = 0;
          break;
        }
      }
    }
  }
  if (MidiUart.txRb.isEmpty_isr() && (MidiUart.txRb_sidechannel == nullptr)) {
    UART_CLEAR_ISR_TX_BIT();
  }
}

#ifdef MEGACOMMAND
ISR(USART2_UDRE_vect) {
#elif UART2_TX
ISR(USART1_UDRE_vect) {
#endif
#ifdef UART2_TX
  select_bank(0);
  if ((MidiUart2.txRb_sidechannel != nullptr) &&
      (MidiUart2.in_message_tx == 0)) {

    if (!MidiUart2.txRb_sidechannel->isEmpty_isr()) {
      MidiUart2.sendActiveSenseTimer = MidiUart2.sendActiveSenseTimeout;
      uint8_t c = MidiUart2.txRb_sidechannel->get_h_isr();
      UART2_WRITE_CHAR(c);
    }
    if (MidiUart2.txRb_sidechannel->isEmpty_isr()) {
      MidiUart2.txRb_sidechannel = nullptr;
    }
  } else {
    if (!MidiUart2.txRb.isEmpty_isr()) {
      MidiUart2.sendActiveSenseTimer = MidiUart2.sendActiveSenseTimeout;
      uint8_t c = MidiUart2.txRb.get_h_isr();
      UART2_WRITE_CHAR(c);
      if ((MidiUart2.in_message_tx > 0) && (c < 128)) {
        MidiUart2.in_message_tx--;
      }
      if (c < 0xF0) {
        switch (c & 0xF0) {
        case MIDI_CHANNEL_PRESSURE:
        case MIDI_PROGRAM_CHANGE:
          MidiUart2.in_message_tx = 1;
          break;
        case MIDI_NOTE_OFF:
        case MIDI_NOTE_ON:
        case MIDI_AFTER_TOUCH:
        case MIDI_CONTROL_CHANGE:
        case MIDI_PITCH_WHEEL:
          MidiUart2.in_message_tx = 2;
          break;
        }
      } else {
        switch (c) {
        case MIDI_SYSEX_START:
          MidiUart2.in_message_tx = -1;
          break;
        case MIDI_SYSEX_END:
          MidiUart2.in_message_tx = 0;
          break;
        }
      }
    }
  }
  if (MidiUart2.txRb.isEmpty_isr() && (MidiUart2.txRb_sidechannel == nullptr)) {
    UART2_CLEAR_ISR_TX_BIT();
  }
}
#endif
#endif
