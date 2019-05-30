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

//#define RX_BUF_SIZE 2048
#if (RX_BUF_SIZE >= 256)
#define RX_BUF_TYPE uint16_t
#else
#define RX_BUF_TYPE uint8_t
#endif
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
  MidiUartClass(volatile uint8_t *rx_buf = NULL, uint16_t rx_buf_size = 0, volatile uint8_t *tx_buf = NULL, uint16_t tx_buf_size = 0);

  inline void m_putc(uint8_t c) {
  if (c == 0xF0) {
    uart_block = 1;
  }
  if (c == 0xF7) {
    uart_block = 0;
  }
   txRb.put_h(c);
   SET_BIT(UCSR1B, UDRIE1);
  }
  inline bool avail() { return !rxRb.isEmpty(); }
  inline uint8_t m_getc() { return rxRb.get(); }

  virtual void m_putc_immediate(uint8_t c);

  void set_speed(uint32_t speed, uint8_t port);

  volatile RingBuffer<0, RX_BUF_TYPE> rxRb;
  volatile RingBuffer<0, TX_BUF_TYPE> txRb;

};

extern MidiUartClass MidiUart;
extern uint16_t midiclock_last;
extern bool enable_clock_callbacks;

class MidiUartClass2 : public MidiUartParent {
  virtual void initSerial();

 public:
  MidiUartClass2(volatile uint8_t *rx_buf = NULL, uint16_t rx_buf_size = 0, volatile uint8_t *tx_buf = NULL, uint16_t tx_buf_size = 0);
  inline bool avail() { return !rxRb.isEmpty(); }
  inline uint8_t m_getc() { return rxRb.get(); }

  inline void m_putc(uint8_t c) {
  if (c == 0xF0) {
    uart_block = 1;
  }
  if (c == 0xF7) {
    uart_block = 0;
  }
   txRb.put_h(c);
    SET_BIT(UCSR2B, UDRIE1);
  }
  virtual void m_putc_immediate(uint8_t c);
  volatile RingBuffer<0, RX_BUF_TYPE> rxRb;
  volatile RingBuffer<0, TX_BUF_TYPE> txRb;
};

extern MidiUartClass2 MidiUart2;

#endif /* MIDI_UART_H__ */
