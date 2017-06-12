#ifndef MIDI_UART_H__
#define MIDI_UART_H__

class MidiUartClass;

#include <inttypes.h>
#include <../../libraries/Midi/src/MidiUartParent.hh>
#include "../../libraries/CommonTools/src/RingBuffer.h"
#include <avr/io.h>
//#define TXEN 3
//#define RXEN 4
//#define RXCIE 7

#define TX_IRQ 1
#define RX_BUF_SIZE 6000

//#define RX_BUF_SIZE 2048
#if (RX_BUF_SIZE >= 256)
#define RX_BUF_TYPE uint16_t
#else
#define RX_BUF_TYPE uint8_t
#endif
#define TX_BUF_SIZE 6000
//define TX_BUF_SIZE 512

#if (TX_BUF_SIZE >= 256)
#define TX_BUF_TYPE uint16_t
#else
#define TX_BUF_TYPE uint8_t
#endif
void isr_usart1(uint8_t caller);
void isr_usart2(uint8_t caller);
        
class MidiUartClass : public MidiUartParent {
  virtual void initSerial();
  
 public:
  MidiUartClass();
  virtual void m_putc(uint8_t c);
  virtual void m_putc_immediate(uint8_t c);
  virtual bool avail();
  virtual uint8_t m_getc();

	void setSpeed(uint32_t speed);

  volatile RingBuffer<RX_BUF_SIZE, RX_BUF_TYPE> rxRb;

#ifdef TX_IRQ
  volatile RingBuffer<TX_BUF_SIZE, TX_BUF_TYPE> txRb;
#endif

};

extern MidiUartClass MidiUart;

class MidiUartClass2 : public MidiUartParent {
  virtual void initSerial();
  
 public:
  MidiUartClass2();
  virtual bool avail();
  virtual uint8_t m_getc();

  volatile RingBuffer<RX_BUF_SIZE, RX_BUF_TYPE> rxRb;
};

extern MidiUartClass2 MidiUart2;

#endif /* MIDI_UART_H__ */
