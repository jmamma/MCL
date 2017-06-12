#include "WProgram.h"
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
#define UART2_CHECK_EMPTY_BUFFER() IS_BIT_SET8(UCSR0A, UDRE)

#define UART_CHECK_RX() IS_BIT_SET8(UCSR0A, RXC)
#define UART_CHECK_TX() IS_BIT_SET8(UCSR0A, TXC)

#define UART_WRITE_CHAR(c) (UDR0 = (c))
#define UART_READ_CHAR() (UDR0)

#define UART2_CHECK_RX() IS_BIT_SET8(UCSR1A, RXC)
#define UART2_READ_CHAR() (UDR1)

#include <avr/io.h>
void midi_start();
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

  //uint32_t cpu = (F_CPU / 16);
  //cpu /= speed;
  //cpu--;
//UBRR0H = ((cpu >> 8));
  
  UBRR0H = ((cpu >> 8) & 0xFF);
  UBRR0L = (cpu & 0xFF);
}

void MidiUartClass::putc_immediate(uint8_t c) {
//  putc(c);
        if (!IN_IRQ()) {
  USE_LOCK();
  SET_LOCK();
  // block interrupts
  while (!UART_CHECK_EMPTY_BUFFER())
          ;
 MidiUart.sendActiveSenseTimer = MidiUart.sendActiveSenseTimeout;
          UART_WRITE_CHAR(c);
  CLEAR_LOCK();
        }
        else {
       while (!UART_CHECK_EMPTY_BUFFER())
       ;

                MidiUart.sendActiveSenseTimer = MidiUart.sendActiveSenseTimeout;
UART_WRITE_CHAR(c);

        }

}

void MidiUartClass::putc(uint8_t c) {
#ifdef TX_IRQ
//#ifdef BLAH
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
          MidiUart.sendActiveSenseTimer = MidiUart.sendActiveSenseTimeout;
          UART_WRITE_CHAR(MidiUart.txRb.get());

        }
      } else {
        SET_BIT(UCSR0B, UDRIE);
      }
    }
    goto again;
  } else {
    clearLed2();
//MidiUart.sendActiveSenseTimer = MidiUart.sendActiveSenseTimeout;

    txRb.put(c);
    // enable interrupt on empty data register to start transfer
    SET_BIT(UCSR0B, UDRIE);
  }
#else
  while (!UART_CHECK_EMPTY_BUFFER());
//Microseconds(20);
//MidiUart.sendActiveSenseTimer = 300; 
MidiUart.sendActiveSenseTimer = MidiUart.sendActiveSenseTimeout;
  MidiUart.sendActiveSenseTimer = MidiUart.sendActiveSenseTimeout;
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
isr_usart0(0);
}
void isr_usart0(uint8_t caller) {
  while (UART_CHECK_RX()) {
  uint8_t c = UART_READ_CHAR();
  uint8_t stopme = 0;
  uint16_t countme = 0;
 if (c != MIDI_ACTIVE_SENSE) {
  //  setLed();
  if (MIDI_IS_REALTIME_STATUS_BYTE(c) && MidiClock.mode == MidiClock.EXTERNAL) {
    switch (c) {
    case MIDI_CLOCK:
      MidiClock.handleClock();
      break;

    case MIDI_START:
      MidiClock.handleMidiStart();
      midi_start();
      break;

    case MIDI_STOP:
      MidiClock.handleMidiStop();
      break;

    case MIDI_CONTINUE:
      MidiClock.handleMidiContinue();
      midi_start();
      break;
   /* case MIDI_SYSEX_START:
      setLed();
     MidiUart.rxRb.put(c); 
      while (stopme == 0) {
        if (UART_CHECK_RX()) { 
           c = UART_READ_CHAR();
           if (!MIDI_IS_REALTIME_STATUS_BYTE(c)) { MidiUart.rxRb.put(c); }
           countme++;
           if (c == MIDI_SYSEX_END) { 
      //      GUI.flash_printf("SYS %b", countme);
            stopme = 1;
           }

        }
      }
      clearLed();
      break;*/
    default:
      MidiUart.rxRb.put(c);
      break;
    }
  } else {
    MidiUart.rxRb.put(c);
  }
 }

if (UART_CHECK_EMPTY_BUFFER() && !MidiUart.txRb.isEmpty()) {
MidiUart.sendActiveSenseTimer = MidiUart.sendActiveSenseTimeout;
UART_WRITE_CHAR(MidiUart.txRb.get());
}
if (UART2_CHECK_RX() && (caller == 0)) { isr_usart1(0); }
}
#if 0
    // show overflow debug
    if (MidiUart.rxRb.overflow) {
      setLed2();
    }
#endif

 // }
  //  clearLed();
}

#ifdef TX_IRQ
SIGNAL(USART0_UDRE_vect) {
//uint16_t count = 0;
uint8_t c;
//  while (!MidiUart.txRb.isEmpty()) {
if (!MidiUart.txRb.isEmpty()) {
while (!UART_CHECK_EMPTY_BUFFER());
          //Microseconds(20);
    //MidiUart.sendActiveSenseTimer = 300;
MidiUart.sendActiveSenseTimer = MidiUart.sendActiveSenseTimeout;
//    if (UART_CHECK_EMPTY_BUFFER()) { 
    c = MidiUart.txRb.get();
  // if ((c != MIDI_ACTIVE_SENSE) || (count == 0)) {
    UART_WRITE_CHAR(c);
    //count++;
   //  }
  //  }
  }

  if (MidiUart.txRb.isEmpty()) {
    CLEAR_BIT(UCSR0B, UDRIE);
  }
  clearLed2();
  if (UART_CHECK_RX()) { isr_usart0(3); }
  if (UART2_CHECK_RX()) { isr_usart1(3); }

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
isr_usart1(1);
}

void isr_usart1(uint8_t caller) {  
  
// while (UART2_CHECK_RX()) { 
  
 uint8_t c = UART2_READ_CHAR();
 if (c != MIDI_ACTIVE_SENSE) { 
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
 //   if (Midi2.midiActive) { MidiUart2.rxRb.put(c); }
  }

// }
if (UART_CHECK_EMPTY_BUFFER() && !MidiUart.txRb.isEmpty()) {
        MidiUart.sendActiveSenseTimer = MidiUart.sendActiveSenseTimeout;
        UART_WRITE_CHAR(MidiUart.txRb.get());
}
 if (UART_CHECK_RX() && (caller == 1)) { isr_usart0(1); }
//if (UART_CHECK_EMPTY_BUFFER() && !MidiUart.txRb.isEmpty()) {
//MidiUart.sendActiveSenseTimer = MidiUart.sendActiveSenseTimeout;
//UART_WRITE_CHAR(c);
//}
 }
#if 0
  // show overflow debug
  if (MidiUart.rxRb.overflow) {
    setLed();
  }
#endif
//}
}




