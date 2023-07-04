#ifndef MIDI_UART_H__
#define MIDI_UART_H__

#include "RingBuffer.h"
#include <MidiUartParent.h>
#include <avr/io.h>
#include <inttypes.h>
#include "Midi.h"
// #define TXEN 3
// #define RXEN 4
// #define RXCIE 7

#ifdef MEGACOMMAND
#define TIMER1_CHECK_INT() IS_BIT_SET8(TIFR1, OCF1A)
#define TIMER1_CLEAR_INT() TIFR1 |= (1 << OCF1A);
#define TIMER3_CHECK_INT() IS_BIT_SET8(TIFR3, OCF3A)
#define TIMER3_CLEAR_INT() TIFR3 |= (1 << OCF3A);
#else
#define TIMER1_CHECK_INT() IS_BIT_SET8(TIFR, OCF1A)
#define TIMER1_CLEAR_INT() TIFR |= (1 << OCF1A);
#define TIMER3_CHECK_INT() IS_BIT_SET8(ETIFR, OCF3A)
#define TIMER3_CLEAR_INT() ETIFR |= (1 << OCF3A);
#endif

#define UART_BAUDRATE 31250
#define UART_BAUDRATE_REG (((F_CPU / 16) / (UART_BAUDRATE)) - 1)

#ifdef MEGACOMMAND

#define UART_USB 1

#define UART_USB_CHECK_EMPTY_BUFFER() IS_BIT_SET8(UCSR0A, UDRE0)

#define UART_CHECK_EMPTY_BUFFER() IS_BIT_SET8(UCSR1A, UDRE1)
#define UART2_CHECK_EMPTY_BUFFER() IS_BIT_SET8(UCSR2A, UDRE2)

#define UART_CHECK_RX() IS_BIT_SET8(UCSR1A, RXC1)
#define UART_CHECK_TX() IS_BIT_SET8(UCSR1A, TXC1)

#define UART_WRITE_CHAR(c) (UDR1 = (c))
#define UART_READ_CHAR() (UDR1)

#define UART2_TX 1

#define UART2_CHECK_RX() IS_BIT_SET8(UCSR2A, RXC1)
#define UART2_CHECK_TX() IS_BIT_SET8(UCSR2A, tXC1)

#define UART2_WRITE_CHAR(c) (UDR2 = (c))
#define UART2_READ_CHAR() (UDR2)

#define UART_USB_CHECK_RX() IS_BIT_SET8(UCSR0A, RXC1)
#define UART_USB_CHECK_TX() IS_BIT_SET8(UCSR0A, tXC1)

#define UART_USB_WRITE_CHAR(c) (UDR0 = (c))
#define UART_USB_READ_CHAR() (UDR0)
#define UART_USB_CHECK_OVERRUN() IS_BIT_SET8(UCSR0A, DOR0)

#define UART_USB_CLEAR_ISR_TX_BIT() CLEAR_BIT(UCSR0B, UDRIE1)
#define UART_CLEAR_ISR_TX_BIT() CLEAR_BIT(UCSR1B, UDRIE1)
#define UART2_CLEAR_ISR_TX_BIT() CLEAR_BIT(UCSR2B, UDRIE1)

#define UART_USB_SET_ISR_TX_BIT() SET_BIT(UCSR0B, UDRIE1)
#define UART_SET_ISR_TX_BIT() SET_BIT(UCSR1B, UDRIE1)
#define UART2_SET_ISR_TX_BIT() SET_BIT(UCSR2B, UDRIE1)

#else

#define UART_CHECK_EMPTY_BUFFER() IS_BIT_SET8(UCSR0A, UDRE1)
#define UART2_CHECK_EMPTY_BUFFER() IS_BIT_SET8(UCSR1A, UDRE2)

#define UART_CHECK_RX() IS_BIT_SET8(UCSR0A, RXC)
#define UART_CHECK_TX() IS_BIT_SET8(UCSR0A, TXC)

#define UART_WRITE_CHAR(c) (UDR0 = (c))
#define UART_READ_CHAR() (UDR0)

#define UART2_CHECK_RX() IS_BIT_SET8(UCSR1A, RXC)
#define UART2_READ_CHAR() (UDR1)

#define UART_CLEAR_ISR_TX_BIT() CLEAR_BIT(UCSR0B, UDRIE1)
#define UART2_CLEAR_ISR_TX_BIT() CLEAR_BIT(UCSR1B, UDRIE1)

#define UART_SET_ISR_TX_BIT() SET_BIT(UCSR0B, UDRIE1)
#define UART2_SET_ISR_TX_BIT() SET_BIT(UCSR1B, UDRIE1)

#endif

#define TX_IRQ 1

// #define RX_BUF_SIZE 2048
#if (RX_BUF_SIZE >= 256)
#define RX_BUF_TYPE uint16_t
#else
#define RX_BUF_TYPE uint8_t
#endif
// define TX_BUF_SIZE 512

#if (TX_BUF_SIZE >= 256)
#define TX_BUF_TYPE uint16_t
#else
#define TX_BUF_TYPE uint8_t
#endif

#define UART_MIDI 0
#define UART_SERIAL 1

#ifdef DEBUGMODE
class MidiUartClass : public MidiUartParent, public Stream {
#else
class MidiUartClass : public MidiUartParent {
#endif

public:
  MidiUartClass(volatile uint8_t *udr_, volatile uint8_t *rx_buf,
                uint16_t rx_buf_size, volatile uint8_t *tx_buf,
                uint16_t tx_buf_size);

  ALWAYS_INLINE() bool avail() { return !rxRb.isEmpty(); }
  ALWAYS_INLINE() uint8_t m_getc() { return rxRb.get(); }

  int8_t in_message_tx;

#ifdef RUNNING_STATUS_OUT
  uint8_t running_status;
  bool running_status_enabled;
#endif

  volatile uint8_t *udr;
  volatile uint8_t *ubrrh() { return udr - 1; }
  volatile uint8_t *ubrrl() { return udr - 2; }
  volatile uint8_t *ucsrc() { return udr - 4; }
  volatile uint8_t *ucsrb() { return udr - 5; }
  volatile uint8_t *ucsra() { return udr - 6; }

#ifdef RUNNING_STATUS_OUT
  ALWAYS_INLINE() bool write_char(uint8_t c) {
    if (!running_status_enabled) {
      *udr = c;
      return true;
    }
    if (MIDI_IS_STATUS_BYTE(c) && MIDI_IS_VOICE_STATUS_BYTE(c)) {
      if (c != running_status) {
        running_status = c;
        *udr = c;
        return true;
      }
      return false;
    } else {
      *udr = c;
      return true;
    }
  }
#else
  ALWAYS_INLINE() void write_char(uint8_t c) { *udr = c; }
#endif

  uint8_t read_char() { return *udr; }
  bool check_empty_tx() {
    volatile uint8_t *ptr = ucsra();
    return *ptr & (1 << UDRE0);
  }

  void set_tx() {
    volatile uint8_t *ptr = ucsrb();
    *ptr |= (1 << UDRIE0);
  }
  void clear_tx() {
    volatile uint8_t *ptr = ucsrb();
    *ptr &= ~(1 << UDRIE0);
  }

  void set_speed(uint32_t speed);

  void initSerial();

  void m_putc_immediate(uint8_t c);
  size_t write(uint8_t c) {
    m_putc(c);
    return 1;
  }

  ALWAYS_INLINE() void realtime_isr(uint8_t c);

  ALWAYS_INLINE() void rx_isr() {
    uint8_t c = read_char();
    recvActiveSenseTimer = 0;
    if (MIDI_IS_REALTIME_STATUS_BYTE(c)) {
      realtime_isr(c);
      return;
    }

    switch (midi->live_state) {
    case midi_wait_sysex:

      if (MIDI_IS_STATUS_BYTE(c)) {
        if (c != MIDI_SYSEX_END) {
          midi->midiSysex.abort();
          rxRb.put_h_isr(c);
        } else {
          midi->midiSysex.end_immediate();
        }
        midi->live_state = midi_wait_status;
      } else {
        // record
        midi->midiSysex.handleByte(c);
      }
      break;

    case midi_wait_status:
      if (c == MIDI_SYSEX_START) {
        midi->live_state = midi_wait_sysex;
        midi->midiSysex.reset();
        break;
      }
    default:
      rxRb.put_h_isr(c);
      break;
    }
  }
  ALWAYS_INLINE() void tx_isr() {
#ifdef RUNNING_STATUS_OUT
    bool rs = 1;
#endif
  again:
    if ((txRb_sidechannel != nullptr) && (in_message_tx == 0)) {
      // sidechannel mounted, and no active messages in normal channel
      // ==> flush the sidechannel now
      if (!txRb_sidechannel->isEmpty_isr()) {
        sendActiveSenseTimer = sendActiveSenseTimeout;
        uint8_t c = txRb_sidechannel->get_h_isr();
#ifdef RUNNING_STATUS_OUT
        rs = write_char(c);
#else
        write_char(c);
#endif
      }
      // unmount sidechannel if drained
      if (txRb_sidechannel->isEmpty_isr()) {
        txRb_sidechannel = nullptr;
      }
    } else if (!txRb.isEmpty_isr()) {
      // 1. either sidechannel is unmounted, or an active message is in normal
      // channel
      // 2. -and- a normal channel byte is queued
      // ==> flush the normal channel now
      sendActiveSenseTimer = sendActiveSenseTimeout;
      uint8_t c = txRb.get_h_isr();
#ifdef RUNNING_STATUS_OUT
      rs = write_char(c);
#else
      write_char(c);
#endif

      if ((in_message_tx > 0) && (c < 128)) {
        in_message_tx--;
      }
      if (c < 0xF0) {
        switch (c & 0xF0) {
        case MIDI_CHANNEL_PRESSURE:
        case MIDI_PROGRAM_CHANGE:
        case MIDI_MTC_QUARTER_FRAME:
        case MIDI_SONG_SELECT:
#ifdef RUNNING_STATUS_OUT
          running_status = 0;
#endif
          in_message_tx = 1;
          break;
        case MIDI_NOTE_OFF:
        case MIDI_NOTE_ON:
        case MIDI_AFTER_TOUCH:
        case MIDI_CONTROL_CHANGE:
        case MIDI_PITCH_WHEEL:
        case MIDI_SONG_POSITION_PTR:
          in_message_tx = 2;
          break;
        }
      } else {
        switch (c) {
        case MIDI_SYSEX_START:
          in_message_tx = -1;
#ifdef RUNNING_STATUS_OUT
          running_status = 0;
#endif
          break;
        case MIDI_SYSEX_END:
          in_message_tx = 0;
#ifdef RUNNING_STATUS_OUT
          running_status = 0;
#endif
          break;
        }
      }
    } else {
      // 1. either sidechannel is unmounted, or an active message is in normal
      // channel
      // 2. -and- normal channel is drained
      // ==> clear active bit and wait for normal channel to be re-supplied
      clear_tx();
    }
#ifdef RUNNING_STATUS_OUT
    if (!rs) {
      goto again;
    }
#endif
    if (txRb.isEmpty_isr() && (txRb_sidechannel == nullptr)) {
      clear_tx();
    }
  }

  ALWAYS_INLINE() void m_recv(uint8_t *src, uint16_t size) {
    rxRb.put_h_isr(src, size);
  }

  ALWAYS_INLINE() void m_putc(uint8_t *src, uint16_t size) {
    txRb.put_h_isr(src, size);
    set_tx();
  }

  ALWAYS_INLINE() void m_putc(uint8_t c) {
    txRb.put_h_isr(c);
    set_tx();
  }

#ifdef DEBUGMODE
  // Stream pure functions
  int available() { return 0; }
  int read() { return 0; }
  int peek() { return 0; }
  void flush() { return; }
#endif

  volatile RingBuffer<0, RX_BUF_TYPE> rxRb;
  volatile RingBuffer<0, TX_BUF_TYPE> txRb;
  volatile RingBuffer<0, TX_BUF_TYPE> *txRb_sidechannel;
};

extern MidiUartClass seq_tx1;
extern MidiUartClass seq_tx2;
extern MidiUartClass seq_tx3;
extern MidiUartClass seq_tx4;

extern MidiUartClass MidiUart;
extern MidiUartClass MidiUart2;
extern MidiUartClass MidiUartUSB;
#endif /* MIDI_UART_H__ */
