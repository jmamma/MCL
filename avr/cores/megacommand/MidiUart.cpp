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
#define TIMER1_CHECK_INT() IS_BIT_SET8(TIFR1,OCF1A)
#define TIMER2_CHECK_INT() IS_BIT_SET8(TIFR2,OCF2A)
#define TIMER1_CLEAR_INT() TIFR1 |= (1 << OCF1A);
#define TIMER2_CLEAR_INT() TIFR2 |= (1 << OCF2A);

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
  MidiUart.speed = speed;
  }
  if (port == 2) {
  UBRR2H = ((cpu >> 8) & 0xFF);
  UBRR2L = (cpu & 0xFF);  
  MidiUart2.speed = speed;
  }
}


void MidiUartClass2::m_putc(uint8_t c) {
#ifdef TX_IRQ
//#ifdef BLAH
  again:
  bool isEmpty = txRb.isEmpty();

  if (txRb.isFull()) {
    while (txRb.isFull()) {
      if (IN_IRQ()) {
        // if we are in an irq, we need to do the sending ourselves as the TX irq is blocked
        setLed2();
        while (!UART2_CHECK_EMPTY_BUFFER())
          ;
        if (!MidiUart2.txRb.isEmpty()) {
          MidiUart2.sendActiveSenseTimer = MidiUart2.sendActiveSenseTimeout;
          UART2_WRITE_CHAR(MidiUart2.txRb.get());

        }
      } else {
        SET_BIT(UCSR2B, UDRIE1);
      }
    }
    goto again;
  } else {
    clearLed2();
//MidiUart.sendActiveSenseTimer = MidiUart.sendActiveSenseTimeout;

    txRb.put(c);
   if (IN_IRQ() && UART2_CHECK_EMPTY_BUFFER()) {
       MidiUart2.sendActiveSenseTimer = MidiUart2.sendActiveSenseTimeout;
       UART2_WRITE_CHAR(MidiUart2.txRb.get()); 
   }
 // else {
   if (!txRb.isEmpty()) { SET_BIT(UCSR2B, UDRIE1); }
 // }
  }
#else
  while (!UART2_CHECK_EMPTY_BUFFER()) {
  if (TIMER1_CHECK_INT()) { TCNT1 = 0;  clock++; TIMER1_CLEAR_INT()  }
   if (TIMER2_CHECK_INT()) {TCNT2 = 0; slowclock++; TIMER2_CLEAR_INT() } 
  }
//Microseconds(20);
//MidiUart.sendActiveSenseTimer = 300; 
MidiUart2.sendActiveSenseTimer = MidiUart2.sendActiveSenseTimeout;
          UART2_WRITE_CHAR(c);
  
#endif
}


void MidiUartClass2::m_putc_immediate(uint8_t c) {

        if (!IN_IRQ()) {
  USE_LOCK();
  SET_LOCK();
  // block interrupts
  while (!UART2_CHECK_EMPTY_BUFFER()) {
  if (TIMER1_CHECK_INT()) { TCNT1 = 0;  clock++; TIMER1_CLEAR_INT()  }
   if (TIMER2_CHECK_INT()) { TCNT2 = 0; slowclock++; TIMER2_CLEAR_INT() } 
        }
          
 MidiUart2.sendActiveSenseTimer = MidiUart2.sendActiveSenseTimeout;
          UART2_WRITE_CHAR(c);
  CLEAR_LOCK();
        }
        else {
       while (!UART2_CHECK_EMPTY_BUFFER()) {
  if (TIMER1_CHECK_INT()) { TCNT1 = 0;  clock++; TIMER1_CLEAR_INT()  }
   if (TIMER2_CHECK_INT()) {TCNT2 = 0; slowclock++; TIMER2_CLEAR_INT() } 
        }


                MidiUart2.sendActiveSenseTimer = MidiUart2.sendActiveSenseTimeout;
UART2_WRITE_CHAR(c);

        }


}
void MidiUartClass::m_putc_immediate(uint8_t c) {
//  m_putc(c);
        if (!IN_IRQ()) {
  USE_LOCK();
  SET_LOCK();
  // block interrupts
  while (!UART_CHECK_EMPTY_BUFFER()) {
           if (TIMER1_CHECK_INT()) { TCNT1 = 0;  clock++; TIMER1_CLEAR_INT()  }
           if (TIMER2_CHECK_INT()) {TCNT2 = 0; slowclock++; TIMER2_CLEAR_INT() } 
  } 
        
 MidiUart.sendActiveSenseTimer = MidiUart.sendActiveSenseTimeout;
          UART_WRITE_CHAR(c);
  CLEAR_LOCK();
        }
        else {
       while (!UART_CHECK_EMPTY_BUFFER()) {

     if (TIMER1_CHECK_INT()) { TCNT1 = 0;  clock++; TIMER1_CLEAR_INT()  }
     if (TIMER2_CHECK_INT()) {TCNT2 = 0; slowclock++; TIMER2_CLEAR_INT() }  
       }
       

                MidiUart.sendActiveSenseTimer = MidiUart.sendActiveSenseTimeout;
UART_WRITE_CHAR(c);

        }

}

void MidiUartClass::m_putc(uint8_t c) {
//#ifdef BLAH
  again:
  bool isEmpty = txRb.isEmpty();

  if (txRb.isFull()) {
    while (txRb.isFull()) {
      if (IN_IRQ()) {
        // if we are in an irq, we need to do the sending ourselves as the TX irq is blocked
        setLed2();
        while (!UART_CHECK_EMPTY_BUFFER()) {
  if (TIMER1_CHECK_INT()) { TCNT1 = 0;  clock++; TIMER1_CLEAR_INT()  }
   if (TIMER2_CHECK_INT()) {TCNT2 = 0; slowclock++; TIMER2_CLEAR_INT() } 
        }
          
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
    if (UART_CHECK_EMPTY_BUFFER() && IN_IRQ()) {
         MidiUart.sendActiveSenseTimer = MidiUart.sendActiveSenseTimeout;
         UART_WRITE_CHAR(MidiUart.txRb.get()); 
    }
    if (!txRb.isEmpty()) { SET_BIT(UCSR1B, UDRIE1); }
    }

  }

bool MidiUartClass::avail() {
  return !rxRb.isEmpty();
}

uint8_t MidiUartClass::m_getc() {
  return rxRb.get();
}

SIGNAL(USART1_RX_vect) {
isr_midi();
}
SIGNAL(USART2_RX_vect) {
isr_midi();
}

void isr_midi() {
  uint8_t c, s; 
  MidiClass *Midi_;
  while (UART_CHECK_RX() || UART2_CHECK_RX()) {
  if (UART_CHECK_RX()) {
  c = UART_READ_CHAR();
  s = 0;
  
Midi_ = &Midi;
  MidiUart.recvActiveSenseTimer = 0;
  }
  else {
  c = UART2_READ_CHAR();
  s = 1;
 
Midi_ = &Midi2;
  MidiUart2.recvActiveSenseTimer = 0;
  }
  if (TIMER1_CHECK_INT()) { TCNT1 = 0;  clock++; TIMER1_CLEAR_INT()  }
 if (TIMER2_CHECK_INT()) {TCNT2 = 0; slowclock++; TIMER2_CLEAR_INT() }  


  //  setLed();
  if (MIDI_IS_REALTIME_STATUS_BYTE(c)) {
    if  (((MidiClock.mode == MidiClock.EXTERNAL_UART1) && (s == 0)) || ((MidiClock.mode == MidiClock.EXTERNAL_UART2) && (s==1))) {
    switch (c) {
    case MIDI_CLOCK:
      MidiClock.handleClock();
  //    MidiClock.callCallbacks();
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
       if (s == 0) { MidiUart.rxRb.put(c); }
       else { MidiUart2.rxRb.put(c); } 
      break;    
    }
    }
  } else {

switch (Midi_->live_state) {  
case midi_wait_sysex: { 

    if (MIDI_IS_STATUS_BYTE(c)) {
      if (c != MIDI_SYSEX_END) {
                Midi_->live_state = midi_wait_status;
                Midi_->midiSysex.abort();
      
       if (s == 0) { MidiUart.rxRb.put(c); }

       else { MidiUart2.rxRb.put(c); } 
 
 } else {
      //handle sysex end here
 //     GUI.flash_string("OKAY");
       Midi_->midiSysex.callSysexCallBacks = true;
       Midi_->live_state = midi_wait_status; 
       Midi_->midiSysex.end();
       
       //if (s == 0) { MidiUart.rxRb.put(c); }
       //else { MidiUart2.rxRb.put(c); } 
     }
   }
  else {
      //record
       Midi_->midiSysex.handleByte(c);
    }   
    break;
  }
 

  case midi_wait_status: {
      if (c == MIDI_SYSEX_START) {
                Midi_->live_state = midi_wait_sysex;
                Midi_->midiSysex.reset();
                //Midi_->last_status = Midi_->running_status = 0;
      }
     else {
   if (s == 0) { MidiUart.rxRb.put(c); }
              else { MidiUart2.rxRb.put(c); }
     }
     }
    break;   
 default:       
        if (s == 0) { MidiUart.rxRb.put(c); }
        else { MidiUart2.rxRb.put(c); } 
    
break;
 
} 

}
}
if (UART_CHECK_EMPTY_BUFFER() && !MidiUart.txRb.isEmpty()) {
MidiUart.sendActiveSenseTimer = MidiUart.sendActiveSenseTimeout;
UART_WRITE_CHAR(MidiUart.txRb.get());
}
if (UART2_CHECK_EMPTY_BUFFER() && !MidiUart2.txRb.isEmpty()) {
MidiUart2.sendActiveSenseTimer = MidiUart2.sendActiveSenseTimeout;
 UART2_WRITE_CHAR(MidiUart2.txRb.get());
}


}


#ifdef TX_IRQ
SIGNAL(USART1_UDRE_vect) {
//uint16_t count = 0;

  isr_midi();
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
}

SIGNAL(USART2_UDRE_vect) {
isr_midi();
uint8_t c;
if (!MidiUart2.txRb.isEmpty()) {
while (!UART2_CHECK_EMPTY_BUFFER());
MidiUart2.sendActiveSenseTimer = MidiUart2.sendActiveSenseTimeout;
    c = MidiUart2.txRb.get();
    UART2_WRITE_CHAR(c);
  }

  if (MidiUart2.txRb.isEmpty()) {
    CLEAR_BIT(UCSR2B, UDRIE1);
  }
  clearLed2();

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



