#pragma once

#include "pico.h"
#include "memory.h"
#include "RingBuffer.h"
#include <MidiUartParent.h>
#include "platform.h"

// RP2040 has 2 UARTs
#define UART_MIDI 0
#define UART_SERIAL 1
#define UART_BAUDRATE 31250

// Timer check macros for RP2040
#define TIMER_CHECK_INT(timer_num) ((timer_hw->intr & (1u << timer_num)) != 0)
#define TIMER_CLEAR_INT(timer_num) timer_hw->intr = (1u << timer_num)

class MidiUartClass : public MidiUartParent {
private:
  uart_inst_t *uart_hw;
  int8_t in_message_tx;
  uint8_t mode;

  ALWAYS_INLINE() bool write_char(uint8_t c) {
#ifdef RUNNING_STATUS_OUT
    if (!running_status_enabled) {
      uart_write_blocking(uart_hw, &c, 1);
      return true;
    }
    if (MIDI_IS_STATUS_BYTE(c) && MIDI_IS_VOICE_STATUS_BYTE(c)) {
      if (c != running_status) {
        running_status = c;
        uart_write_blocking(uart_hw, &c, 1);
        return true;
      }
      return false;
    }
#endif
    uart_write_blocking(uart_hw, &c, 1);
    return true;
  }

  ALWAYS_INLINE() uint8_t read_char() { return uart_getc(uart_hw); }

  ALWAYS_INLINE() bool check_empty_tx() { return uart_is_writable(uart_hw); }

  ALWAYS_INLINE() void enable_tx_irq() {
    uart_set_irq_enables(uart_hw, true, true);
  }

  ALWAYS_INLINE() void disable_tx_irq() {
    uart_set_irq_enables(uart_hw, true, false);
  }
public:
  // Ring buffers with compile-time sizes
  volatile RingBuffer *rxRb;
  volatile RingBuffer *txRb;
  volatile RingBuffer *txRb_sidechannel;

#ifdef RUNNING_STATUS_OUT
  uint8_t running_status;
  bool running_status_enabled;
#endif

  MidiUartClass(uart_inst_t *uart_hw, RingBuffer *_rxRb = nullptr,
                RingBuffer *_txRb = nullptr);

  ALWAYS_INLINE() void realtime_isr(uint8_t c);
  ALWAYS_INLINE() void rx_isr();
  ALWAYS_INLINE() void tx_isr();

  // Basic MIDI UART operations
  ALWAYS_INLINE() bool avail() { return !rxRb->isEmpty(); }
  ALWAYS_INLINE() uint8_t m_getc() { return rxRb->get(); }
  void initSerial();
  void set_speed(uint32_t speed);
  void m_putc_immediate(uint8_t c);

  // Interrupt handlers

  // MIDI message handling
  ALWAYS_INLINE() void m_recv(uint8_t *src, uint16_t size) {
    rxRb->put_h_isr(src, size);
  }

  ALWAYS_INLINE() void m_putc(uint8_t *src, uint16_t size) {
    DEBUG_FUNC();
    txRb->put_h_isr(src, size);
    enable_tx_irq();
  }

  ALWAYS_INLINE() void m_putc(uint8_t c) {
    txRb->put_h_isr(c);
    enable_tx_irq();
  }
};

#ifdef __cplusplus
extern "C" {
#endif

void uart0_irq_handler(void);
void uart1_irq_handler(void);

#ifdef __cplusplus
}
#endif
