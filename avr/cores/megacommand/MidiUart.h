#ifndef MIDI_UART_H__
#define MIDI_UART_H__
class MidiUartClass;

#include <inttypes.h>
#include <MidiUartParent.hh>
#include "RingBuffer.h"
#include <avr/io.h>
//#define TXEN 3
//#define RXEN 4
//#define RXCIE 7

#define TIMER1_CHECK_INT() IS_BIT_SET8(TIFR1, OCF1A)
#define TIMER2_CHECK_INT() IS_BIT_SET8(TIFR2, OCF2A)
#define TIMER1_CLEAR_INT() TIFR1 |= (1 << OCF1A);
#define TIMER2_CLEAR_INT() TIFR2 |= (1 << OCF2A);

#define UART_BAUDRATE 31250
#define UART_BAUDRATE_REG (((F_CPU / 16) / (UART_BAUDRATE)) - 1)

#define UART_USB_CHECK_EMPTY_BUFFER() IS_BIT_SET8(UCSR0A, UDRE0)

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

#define UART_USB_CHECK_RX() IS_BIT_SET8(UCSR0A, RXC1)
#define UART_USB_CHECK_TX() IS_BIT_SET8(UCSR0A, tXC1)

#define UART_USB_WRITE_CHAR(c) (UDR0 = (c))
#define UART_USB_READ_CHAR() (UDR0)


#define TX_IRQ 1
#define RX_BUF_SIZE 128

//#define RX_BUF_SIZE 2048
#if (RX_BUF_SIZE >= 256)
#define RX_BUF_TYPE uint16_t
#else
#define RX_BUF_TYPE uint8_t
#endif
#define TX_BUF_SIZE 3000
//define TX_BUF_SIZE 512

#if (TX_BUF_SIZE >= 256)
#define TX_BUF_TYPE uint16_t
#else
#define TX_BUF_TYPE uint8_t
#endif
void isr_usart1(uint8_t caller);
void isr_usart2(uint8_t caller);
void isr_midi();        
class MidiUartClass : public MidiUartParent {
  virtual void initSerial();
  
 public:
  MidiUartClass();
  virtual void m_putc(uint8_t c);
  virtual void m_putc_immediate(uint8_t c);
  virtual bool avail();
  virtual uint8_t m_getc();

	void set_speed(uint32_t speed, uint8_t port);

  volatile RingBuffer<RX_BUF_SIZE, RX_BUF_TYPE> rxRb;

#ifdef TX_IRQ
  volatile RingBuffer<TX_BUF_SIZE, TX_BUF_TYPE> txRb;
#endif

};

extern MidiUartClass MidiUart;
extern uint16_t midiclock_last;
class MidiUartClass2 : public MidiUartParent {
  virtual void initSerial();
  
 public:
  MidiUartClass2();
  virtual bool avail();
  virtual uint8_t m_getc();
  virtual void m_putc(uint8_t c);
  virtual void m_putc_immediate(uint8_t c);
  volatile RingBuffer<RX_BUF_SIZE, RX_BUF_TYPE> rxRb;

  volatile RingBuffer<TX_BUF_SIZE, TX_BUF_TYPE> txRb;
};

extern MidiUartClass2 MidiUart2;

#endif /* MIDI_UART_H__ */
