#include "WProgram.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

#include <MidiUart.h>
#include <MidiUartParent.hh>
#include <midi-common.hh>

#include <MidiClock.h>
MidiUartClass MidiUart((volatile uint8_t *)BANK1_UART1_RX_BUFFER_START,
                       UART1_RX_BUFFER_LEN,
                       (volatile uint8_t *)BANK1_UART1_TX_BUFFER_START,
                       UART1_TX_BUFFER_LEN);
MidiUartClass2 MidiUart2((volatile uint8_t *)BANK1_UART2_RX_BUFFER_START,
                         UART2_RX_BUFFER_LEN,
                         (volatile uint8_t *)BANK1_UART2_TX_BUFFER_START,
                         UART2_TX_BUFFER_LEN);

// extern MidiClockClass MidiClock;
#include <avr/io.h>

MidiUartClass::MidiUartClass(volatile uint8_t *rx_buf, uint16_t rx_buf_size,
                             volatile uint8_t *tx_buf, uint16_t tx_buf_size)
    : MidiUartParent() {
  if (rx_buf) {
    rxRb.ptr = rx_buf;
    rxRb.len = rx_buf_size;
  }
  if (tx_buf) {
    txRb.ptr = tx_buf;
    txRb.len = tx_buf_size;
  }
  initSerial();
}

void MidiUartClass::initSerial() {
  running_status = 0;
  set_speed(31250, 1);
  set_speed(31250, 2);

  #ifdef MEGACOMMAND
  UCSR1C = (3 << UCSZ00);

  /** enable receive, transmit and receive and transmit interrupts. **/
  UCSR1B = _BV(RXEN0) | _BV(TXEN0) | _BV(RXCIE0);
  #else
  UCSR0C = (3<<UCSZ00);

  UCSR0B = _BV(RXEN0) | _BV(TXEN0) | _BV(RXCIE0);
  #endif
}

void MidiUartClass::set_speed(uint32_t speed, uint8_t port) {
#ifdef TX_IRQ
  // empty TX buffer before switching speed
  while (!txRb.isEmpty())
    ;
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

    MidiUart2.sendActiveSenseTimer = MidiUart2.sendActiveSenseTimeout;
    UART2_WRITE_CHAR(c);
    CLEAR_LOCK();
  } else {
    while (!UART2_CHECK_EMPTY_BUFFER()) {
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

    MidiUart.sendActiveSenseTimer = MidiUart.sendActiveSenseTimeout;
    UART_WRITE_CHAR(c);
    CLEAR_LOCK();
  } else {
    while (!UART_CHECK_EMPTY_BUFFER()) {

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
        MidiClock.callCallbacks();
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
  } else {

    if (MIDI_IS_STATUS_BYTE(c)) {
      MidiUart.recvActiveSenseTimer = 0;
    }
    if (Midi.forward) {
      MidiUart2.m_putc(c);
    }
    switch (Midi.live_state) {
    case midi_wait_sysex: {

      if (MIDI_IS_STATUS_BYTE(c)) {
        if (c != MIDI_SYSEX_END) {
          Midi.live_state = midi_wait_status;
          Midi.midiSysex.abort();

          MidiUart.rxRb.put_h_isr(c);

        } else {
          // handle sysex end here
          //     GUI.flash_string("OKAY");
          Midi.midiSysex.callSysexCallBacks = true;
          Midi.live_state = midi_wait_status;
          Midi.midiSysex.end_immediate();
        }
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
        // Midi_->last_status = Midi_->running_status = 0;
      } else {
        MidiUart.rxRb.put_h_isr(c);
      }
    } break;
    default:
      MidiUart.rxRb.put_h_isr(c);

      break;
    }
  }
  /*
    if (UART_CHECK_EMPTY_BUFFER() && !MidiUart.txRb.isEmpty()) {
      MidiUart.sendActiveSenseTimer = MidiUart.sendActiveSenseTimeout;
      UART_WRITE_CHAR(MidiUart.txRb.get());
    }
    if (UART2_CHECK_EMPTY_BUFFER() && !MidiUart2.txRb.isEmpty()) {
     MidiUart2.sendActiveSenseTimer = MidiUart2.sendActiveSenseTimeout;
      UART2_WRITE_CHAR(MidiUart2.txRb.get());
    } */
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
        MidiClock.callCallbacks();
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
  } else {

    if (MIDI_IS_STATUS_BYTE(c)) {
      MidiUart2.recvActiveSenseTimer = 0;
    }
    if (Midi2.forward) {
      MidiUart.m_putc(c);
    }
    switch (Midi2.live_state) {
    case midi_wait_sysex: {

      if (MIDI_IS_STATUS_BYTE(c)) {
        if (c != MIDI_SYSEX_END) {
          Midi2.live_state = midi_wait_status;
          Midi2.midiSysex.abort();

          MidiUart2.rxRb.put_h_isr(c);

        } else {
          // handle sysex end here
          //     GUI.flash_string("OKAY");
          Midi2.midiSysex.callSysexCallBacks = true;
          Midi2.live_state = midi_wait_status;
          Midi2.midiSysex.end_immediate();

          // if (s == 0) { MidiUart.rxRb.put(c); }
          // else { MidiUart2.rxRb.put(c); }
        }
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
        // Midi_->last_status = Midi_->running_status = 0;
      } else {
        MidiUart2.rxRb.put_h_isr(c);
      }
    } break;
    default:
      MidiUart2.rxRb.put_h_isr(c);

      break;
    }
  }
  /*
  if (UART_CHECK_EMPTY_BUFFER() && !MidiUart.txRb.isEmpty()) {
    MidiUart.sendActiveSenseTimer = MidiUart.sendActiveSenseTimeout;
    UART_WRITE_CHAR(MidiUart.txRb.get());
  }
  if (UART2_CHECK_EMPTY_BUFFER() && !MidiUart2.txRb.isEmpty()) {
    MidiUart2.sendActiveSenseTimer = MidiUart2.sendActiveSenseTimeout;
    UART2_WRITE_CHAR(MidiUart2.txRb.get());
  }
  */
}

#ifdef TX_IRQ

#ifdef MEGACOMMAND
ISR(USART1_UDRE_vect) {
#else
ISR(USART0_UDRE_vect) {
#endif
  select_bank(0);
  if (!MidiUart.txRb.isEmpty_isr()) {
    MidiUart.sendActiveSenseTimer = MidiUart.sendActiveSenseTimeout;
    UART_WRITE_CHAR(MidiUart.txRb.get_h_isr());
  }
  if (MidiUart.txRb.isEmpty_isr()) {
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
  if (!MidiUart2.txRb.isEmpty_isr()) {
    MidiUart2.sendActiveSenseTimer = MidiUart2.sendActiveSenseTimeout;
    UART2_WRITE_CHAR(MidiUart2.txRb.get_h_isr());
  }
  if (MidiUart2.txRb.isEmpty_isr()) {
    UART2_CLEAR_ISR_TX_BIT();
  }
}
#endif
#endif

MidiUartClass2::MidiUartClass2(volatile uint8_t *rx_buf, uint16_t rx_buf_size,
                               volatile uint8_t *tx_buf, uint16_t tx_buf_size)
    : MidiUartParent() {
  if (rx_buf) {
    rxRb.ptr = rx_buf;
    rxRb.len = rx_buf_size;
  }
  #ifdef UART2_TX
  if (tx_buf) {
    txRb.ptr = tx_buf;
    txRb.len = tx_buf_size;
  }
  #endif
  initSerial();
}

void MidiUartClass2::initSerial() {
  running_status = 0;
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
