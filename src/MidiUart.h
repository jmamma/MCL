#pragma once

#include "pico.h"
#include "memory.h"
#include "RingBuffer.h"
#include <MidiUartParent.h>
#include "Midi.h"  // Now safe to include

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
  ALWAYS_INLINE() void rx_isr() {
    while (uart_is_readable(uart_hw)) {
      uint8_t c = read_char();
      if (MIDI_IS_REALTIME_STATUS_BYTE(c)) {
        realtime_isr(c);
        continue;
      }

      switch (midi->live_state) {
      case midi_wait_sysex:
        if (MIDI_IS_STATUS_BYTE(c)) {
          if (c != MIDI_SYSEX_END) {
            midi->midiSysex->abort();
            rxRb->put_h_isr(c);
          } else {
            midi->midiSysex->end_immediate();
          }
          midi->live_state = midi_wait_status;
        } else {
          midi->midiSysex->handleByte(c);
        }
        break;

      case midi_wait_status:
        if (c == MIDI_SYSEX_START) {
          midi->live_state = midi_wait_sysex;
          midi->midiSysex->reset();
          break;
        }
        [[fallthrough]];
      default:
        rxRb->put_h_isr(c);
        break;
      }
    }
  }

  ALWAYS_INLINE() void tx_isr() {
#ifdef RUNNING_STATUS_OUT
    bool rs = 1;
  again:
#endif
    if ((txRb_sidechannel != nullptr) && (in_message_tx == 0)) {
      if (!txRb_sidechannel->isEmpty()) {
        uint8_t c = txRb_sidechannel->get();
#ifdef RUNNING_STATUS_OUT
        rs = write_char(c);
#else
        write_char(c);
#endif
      }
      if (txRb_sidechannel->isEmpty()) {
        txRb_sidechannel = nullptr;
      }
    } else if (!txRb->isEmpty()) {
      uint8_t c = txRb->get();
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
      disable_tx_irq();
    }
#ifdef RUNNING_STATUS_OUT
    if (!rs) {
      goto again;
    }
#endif
    if (txRb->isEmpty() && (txRb_sidechannel == nullptr)) {
      disable_tx_irq();
    }
  }


  // Basic MIDI UART operations
  ALWAYS_INLINE() bool avail() { return !rxRb->isEmpty(); }
  ALWAYS_INLINE() uint8_t m_getc() { return rxRb->get(); }
  void initSerial();
  void set_speed(uint32_t speed);
  void m_putc_immediate(uint8_t c);

  // Interrupt handlers
  ALWAYS_INLINE() void realtime_isr(uint8_t c);

  // MIDI message handling
  ALWAYS_INLINE() void m_recv(uint8_t *src, uint16_t size) {
    rxRb->put_h_isr(src, size);
  }

  ALWAYS_INLINE() void m_putc(uint8_t *src, uint16_t size) {
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
