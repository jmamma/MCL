//#define IS_ISR_ROUTINE

#include "WProgram.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

#include <MidiUart.h>
#include <MidiUartParent.h>
#include <midi-common.h>

#include <MidiClock.h>
#include <avr/io.h>

#include "MCLSeq.h"

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
  set_speed(31250);
  volatile uint8_t *src = ucsrc();
  volatile uint8_t *srb = ucsrb();

  #ifdef RUNNING_STATUS_OUT
  running_status_enabled = true;
  running_status = 0;
  #endif

  *src = (3 << UCSZ00);
  *srb = _BV(RXEN0) | _BV(TXEN0) | _BV(RXCIE0);
}

void MidiUartClass::set_speed(uint32_t speed_) {
  // empty TX buffer before switching speed
  // UBRR1H = ((cpu >> 8) & 0xFF);
  // UBRR1L = (cpu & 0xFF);
  while (!txRb.isEmpty())
    ;
  while (!(check_empty_tx()));

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

void MidiUartClass::realtime_isr(uint8_t c) {
  if (c == MIDI_CLOCK) {
    if (MidiClock.uart_clock_recv == this) {
      MidiClock.handleClock();
      if (MidiClock.state != 2 || MidiClock.inCallback) { return; }
      MidiClock.inCallback = true;
      uint8_t _midi_lock_tmp = MidiUartParent::handle_midi_lock;
      MidiUartParent::handle_midi_lock = 1;
      sei();
      mcl_seq.seq();
      MidiUartParent::handle_midi_lock = _midi_lock_tmp;
      MidiClock.inCallback = false;
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

ISR(USART0_RX_vect) {
  select_bank(BANK0);
  MidiUartUSB.rx_isr();
}

ISR(USART0_UDRE_vect) {
  select_bank(BANK0);
  MidiUartUSB.tx_isr();
}

ISR(USART1_RX_vect) {
  select_bank(BANK0);
  MidiUart.rx_isr();
}

ISR(USART1_UDRE_vect) {
  select_bank(BANK0);
  MidiUart.tx_isr();
}

ISR(USART2_RX_vect) {
  select_bank(BANK0);
  MidiUart2.rx_isr();
}

ISR(USART2_UDRE_vect) {
  select_bank(BANK0);
  MidiUart2.tx_isr();
}
