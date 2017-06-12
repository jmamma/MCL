/*
 * MidiCtrl - Midi Uart implementation for the minicommand2
 * use interrupt driven UART receive and sending
 *
 * (c) July 2011 - Manuel Odendahl - wesen@ruinwesen.com
 */

#include "Platform.h"

#include "helpers.h"

#include <avr/interrupt.h>
#include <util/delay.h>

#include <MidiUart.h>
#include <midi-common.hh>
#include <MidiUartParent.hh>

MidiUartClass MidiUart;
MidiUartClass2 MidiUart2;

#define UART_BAUDRATE 31250
#define UART_BAUDRATE_REG (((F_CPU / 16)/(UART_BAUDRATE)) - 1)

#define UART_CHECK_EMPTY_BUFFER() IS_BIT_SET8(UCSR0A, UDRE)
#define UART_CHECK_RX()           IS_BIT_SET8(UCSR0A, RXC)
#define UART_WRITE_CHAR(c)        (UDR0 = (c))
#define UART_READ_CHAR()          (UDR0)

#define UART2_CHECK_RX()          IS_BIT_SET8(UCSR1A, RXC)
#define UART2_READ_CHAR()         (UDR1)

#include <avr/io.h>

MidiUartClass::MidiUartClass() : MidiUartParent() {
  initSerial();
}

void MidiUartClass::initSerial() {
  running_status = 0;
  setSpeed(31250);

  //  UBRR0H = (UART_BAUDRATE_REG >> 8);
  //  UBRR0L = (UART_BAUDRATE_REG & 0xFF);

  UCSR0C = (3<<UCSZ00); 

  /** enable receive, transmit and receive and transmit interrupts. **/
  //  UCSRB = _BV(RXEN) | _BV(TXEN) | _BV(RXCIE);
  UCSR0B = _BV(RXEN) | _BV(TXEN) | _BV(RXCIE);
#ifdef TX_IRQ
#endif
}

void MidiUartClass::setSpeed(uint32_t speed) {
#ifdef TX_IRQ
  // empty TX buffer before switching speed
  while (!txRb.isEmpty())
    ;
#endif

  uint32_t cpu = (F_CPU / 16);
  cpu /= speed;
  cpu--;
  UBRR0H = ((cpu >> 8) & 0xFF);
  UBRR0L = (cpu & 0xFF);
}

void MidiUartClass::putc_immediate(uint8_t c) {
  USE_LOCK();
  SET_LOCK();
  // block interrupts
  while (!UART_CHECK_EMPTY_BUFFER())
    ;
  UART_WRITE_CHAR(c);
  CLEAR_LOCK();
}

void MidiUartClass::putc(uint8_t c) {
#ifdef TX_IRQ
  again:
  bool isEmpty = txRb.isEmpty();

  if (txRb.isFull()) {
    while (txRb.isFull()) {
      if (IN_IRQ()) {
        // if we are in an irq, we need to do the sending ourselves as the TX irq is blocked
        setLed2();
        while (!UART_CHECK_EMPTY_BUFFER())
          ;
        if (!MidiUart.txRb.isEmpty()) {
          UART_WRITE_CHAR(MidiUart.txRb.get());
        }
      } else {
        SET_BIT(UCSR0B, UDRIE);
      }
    }
    goto again;
  } else {
    clearLed2();
    txRb.put(c);
    // enable interrupt on empty data register to start transfer
    SET_BIT(UCSR0B, UDRIE);
  }
#else
  while (!UART_CHECK_EMPTY_BUFFER())
    ;
  UART_WRITE_CHAR(c);
#endif
}

bool MidiUartClass::avail() {
  return !rxRb.isEmpty();
}

uint8_t MidiUartClass::getc() {
  return rxRb.get();
}

SIGNAL(USART0_RX_vect) {
  uint8_t c = UART_READ_CHAR();

  //  setLed();
  if (MIDI_IS_REALTIME_STATUS_BYTE(c) && MidiClock.mode == MidiClock.EXTERNAL) {
    switch (c) {
    case MIDI_CLOCK:
      MidiClock.handleClock();
      break;

    case MIDI_START:
      MidiClock.handleMidiStart();
      break;

    case MIDI_STOP:
      MidiClock.handleMidiStop();
      break;

    case MIDI_CONTINUE:
      MidiClock.handleMidiContinue();
      break;

    default:
      MidiUart.rxRb.put(c);
      break;
    }
  } else {
    MidiUart.rxRb.put(c);

#if 1
    // show overflow debug
    if (MidiUart.rxRb.overflow) {
//      setLed2();
    }
#endif

  }
  //  clearLed();
}

#ifdef TX_IRQ
SIGNAL(USART0_UDRE_vect) {
  if (!MidiUart.txRb.isEmpty()) {
    UART_WRITE_CHAR(MidiUart.txRb.get());
  }

  if (MidiUart.txRb.isEmpty()) {
    CLEAR_BIT(UCSR0B, UDRIE);
  }
  clearLed2();
}
#endif

MidiUartClass2::MidiUartClass2() : MidiUartParent() {
  initSerial();
}

void MidiUartClass2::initSerial() {
  running_status = 0;
  UBRR1H = (UART_BAUDRATE_REG >> 8);
  UBRR1L = (UART_BAUDRATE_REG & 0xFF);
  //  UBRRH = 0;
  //  UBRRL = 15;

  UCSR1C = (3<<UCSZ00); 

  /** enable receive, transmit and receive and transmit interrupts. **/
  //  UCSRB = _BV(RXEN) | _BV(TXEN) | _BV(RXCIE);
  UCSR1B = _BV(RXEN) | _BV(RXCIE);
}

bool MidiUartClass2::avail() {
  return !rxRb.isEmpty();
}

uint8_t MidiUartClass2::getc() {
  return rxRb.get();
}

SIGNAL(USART1_RX_vect) {
  uint8_t c = UART2_READ_CHAR();

  // XXX clock on second input
  if (MIDI_IS_REALTIME_STATUS_BYTE(c) && MidiClock.mode == MidiClock.EXTERNAL_UART2) {
    switch (c) {
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

    default:
      MidiUart2.rxRb.put(c);
      break;
    }
  } else {
    if (Midi2.midiActive)
      MidiUart2.rxRb.put(c);
  }

#if 0
  // show overflow debug
  if (MidiUart.rxRb.overflow) {
    setLed();
  }
#endif
}

