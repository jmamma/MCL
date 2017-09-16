#include "WProgram.h"
//#include "../../libraries/CommonTools/src/helpers.h"

//#include "helpers.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <MidiUart.h>
#include <midi-common.hh>
#include <MidiUartParent.hh>

#include <MidiClock.h>
MidiUartClass MidiUart;
MidiUartClass2 MidiUart2;

//extern MidiClockClass MidiClock;
#define UART_BAUDRATE 31250
#define UART_BAUDRATE_REG (((F_CPU / 16)/(UART_BAUDRATE)) - 1)

#define UART_CHECK_EMPTY_BUFFER() IS_BIT_SET8(UCSR1A, UDRE1)
#define UART2_CHECK_EMPTY_BUFFER() IS_BIT_SET8(UCSR2A, UDRE2)

#define UART_CHECK_RX() IS_BIT_SET8(UCSR1A, RXC1)
#define UART_CHECK_TX() IS_BIT_SET8(UCSR1A, TXC1)

#define UART_WRITE_CHAR(c) (UDR1 = (c))
#define UART_READ_CHAR() (UDR1)

#define UART2_CHECK_RX() IS_BIT_SET8(UCSR2A, RXC1)
#define UART2_CHECK_TX() IS_BIT_SET8(UCSR2A, tXC1)

#define UART2_WRITE_CHAR(c) (UDR2 = (c))
#define UART2_READ_CHAR() (UDR2)

#include <avr/io.h>
//void midi_start();
MidiUartClass::MidiUartClass() : MidiUartParent() {
  initSerial();
}

void MidiUartClass::initSerial() {
  running_status = 0;
  setSpeed(31250,1); 
  setSpeed(31250,2);
 
  //  UBRR0H = (UART_BAUDRATE_REG >> 8);
  //  UBRR0L = (UART_BAUDRATE_REG & 0xFF);

  UCSR1C = (3<<UCSZ00); 


 
  /** enable receive, transmit and receive and transmit interrupts. **/
  //  UCSRB = _BV(RXEN) | _BV(TXEN) | _BV(RXCIE);
  UCSR1B = _BV(RXEN0) | _BV(TXEN0) | _BV(RXCIE0);


#ifdef TX_IRQ
#endif
}

void MidiUartClass::setSpeed(uint32_t speed, uint8_t port) {
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
  if (port == 1) {
  UBRR1H = ((cpu >> 8) & 0xFF);
  UBRR1L = (cpu & 0xFF);
  }
  if (port == 2) {
  UBRR2H = ((cpu >> 8) & 0xFF);
  UBRR2L = (cpu & 0xFF); 
  }
}

void MidiUartClass2::m_putc(uint8_t c) {
while (!UART2_CHECK_EMPTY_BUFFER());

        UART2_WRITE_CHAR(c);

}
void MidiUartClass::m_putc_immediate(uint8_t c) {
//  m_putc(c);
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

void MidiUartClass::m_putc(uint8_t c) {
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
        SET_BIT(UCSR1B, UDRIE1);
      }
    }
    goto again;
  } else {
    clearLed2();
//MidiUart.sendActiveSenseTimer = MidiUart.sendActiveSenseTimeout;

    txRb.put(c);
    // enable interrupt on empty data register to start transfer
 // if (IN_IRQ() && UART_CHECK_EMPTY_BUFFER()) {
   //        MidiUart.sendActiveSenseTimer = MidiUart.sendActiveSenseTimeout;
     //     UART_WRITE_CHAR(MidiUart.txRb.get()); 
  //  }
 // else {
    SET_BIT(UCSR1B, UDRIE1);
 // }
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

uint8_t MidiUartClass::m_getc() {
  return rxRb.get();
}

SIGNAL(USART1_RX_vect) {
isr_usart1(1);
}
extern void midi_start();

void isr_usart1(uint8_t caller) {
  while (UART_CHECK_RX()) {
  uint8_t c = UART_READ_CHAR();
 if (c != MIDI_ACTIVE_SENSE) {
  //  setLed();
  if (MIDI_IS_REALTIME_STATUS_BYTE(c) && MidiClock.mode == MidiClock.EXTERNAL_UART1) {
    switch (c) {
    case MIDI_CLOCK:
      MidiClock.handleClock();
      //       MidiClock.callCallbacks();
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
if (UART2_CHECK_RX() && (caller == 1)) { isr_usart2(1); }
//   if (MidiClock.div96th_counter != MidiClock.div96th_counter_last) {
  //           MidiClock.div96th_counter_last = MidiClock.div96th_counter;
    //           MidiClock.callCallbacks();
      //           }

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
SIGNAL(USART1_UDRE_vect) {
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
    CLEAR_BIT(UCSR1B, UDRIE1);
  }
  clearLed2();
  if (UART_CHECK_RX()) { isr_usart1(3); }
  if (UART2_CHECK_RX()) { isr_usart2(3); }

}
#endif

MidiUartClass2::MidiUartClass2() : MidiUartParent() {
  initSerial();
}

void MidiUartClass2::initSerial() {
  running_status = 0;
//  UBRR2H = (UART_BAUDRATE_REG >> 8);
//  UBRR2L = (UART_BAUDRATE_REG & 0xFF);
  //  UBRRH = 0;
  //  UBRRL = 15;

  UCSR2C = (3<<UCSZ00); 

  /** enable receive, transmit and receive and transmit interrupts. **/
  //  UCSRB = _BV(RXEN) | _BV(TXEN) | _BV(RXCIE);
  UCSR2B = _BV(RXEN1) | _BV(TXEN1) | _BV(RXCIE1);
  
}

bool MidiUartClass2::avail() {
  return !rxRb.isEmpty();
}

uint8_t MidiUartClass2::m_getc() {
  return rxRb.get();
}

SIGNAL(USART2_RX_vect) {
isr_usart2(2);
}

void isr_usart2(uint8_t caller) {  
  
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
  }
          else {
      MidiUart2.rxRb.put(c);
    
  } 
 //   if (Midi2.midiActive) { MidiUart2.rxRb.put(c); }
  

// }
if (UART_CHECK_EMPTY_BUFFER() && !MidiUart.txRb.isEmpty()) {
        MidiUart.sendActiveSenseTimer = MidiUart.sendActiveSenseTimeout;
        UART_WRITE_CHAR(MidiUart.txRb.get());
}
 if (UART_CHECK_RX() && (caller == 2)) { isr_usart1(2); }
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




