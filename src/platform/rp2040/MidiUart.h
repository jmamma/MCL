#pragma once

#include "RingBuffer.h"
#include "memory.h"
#include "pico.h"
#include "platform.h"
#include <MidiUartParent.h>
#include "RP2040USB.h"
#include <arduino/midi/Adafruit_USBD_MIDI.h>
#include "hardware.h"

#define UART_BAUDRATE 31250

// Timer check macros for RP2040
#define TIMER_CHECK_INT(timer_num) ((timer_hw->intr & (1u << timer_num)) != 0)
#define TIMER_CLEAR_INT(timer_num) timer_hw->intr = (1u << timer_num)
#define UART_MIDI 0
#define UART_SERIAL 1

class MidiUartClass : public MidiUartParent {
private:
  uart_inst_t *uart_hw;
  uint8_t mode;

  ALWAYS_INLINE() bool write_char(uint8_t c) {
    if (!uart_hw) {
      return false;
    }
#ifdef RUNNING_STATUS_OUT
    if (!running_status_enabled) {
      uart_get_hw(uart_hw)->dr = c;
      return true;
    }
    if (MIDI_IS_STATUS_BYTE(c) && MIDI_IS_VOICE_STATUS_BYTE(c)) {
      if (c != running_status) {
        running_status = c;
        uart_get_hw(uart_hw)->dr = c;
        return true;
      }
      return false;
    }
#endif
    uart_get_hw(uart_hw)->dr = c;
    return true;
  }

  ALWAYS_INLINE() uint8_t read_char() { return uart_get_hw(uart_hw)->dr; }

public:
  // Ring buffers with compile-time sizes
  int8_t in_message_tx;
  volatile RingBuffer<> *rxRb;
  volatile RingBuffer<> *txRb;
  volatile RingBuffer<> *txRb_sidechannel;

#ifdef RUNNING_STATUS_OUT
  uint8_t running_status;
  bool running_status_enabled;
#endif

  MidiUartClass(uart_inst_t *uart_hw, RingBuffer<> *_rxRb = nullptr,
                RingBuffer<> *_txRb = nullptr);

  void realtime_isr(uint8_t c);
  void rx_isr();
  void tx_isr();
  ALWAYS_INLINE() void enable_tx_irq() {
    uart_set_irq_enables(uart_hw, true, true);
  }

  ALWAYS_INLINE() void disable_tx_irq() {
    uart_set_irq_enables(uart_hw, true, false);
  }

  // Basic MIDI UART operations
  ALWAYS_INLINE() bool avail() { return !rxRb->isEmpty(); }
  ALWAYS_INLINE() uint8_t m_getc() { return rxRb->get(); }
  void init();
  void set_speed(uint32_t speed);
  void m_putc_immediate(uint8_t c);
  ALWAYS_INLINE() bool check_empty_tx() {
    return uart_hw && uart_is_writable(uart_hw);
  }
  // Interrupt handlers
  ALWAYS_INLINE() void m_recv(uint8_t *src, uint16_t size) {
    rxRb->put_h_isr(src, size);
  }
  ALWAYS_INLINE() void tx_flush() {
    if (uart_hw) { // Important, don't allow flush for non-hw uarts.
      if (uart_is_writable(uart_hw)) {
        tx_isr();
      } else {
        enable_tx_irq();
      }
    }
  }
  ALWAYS_INLINE() void m_putc(uint8_t *src, uint16_t size) {
    LOCK();
    txRb->put_h_isr(src, size);
    tx_flush();
    CLEAR_LOCK();
  }

  ALWAYS_INLINE() void m_putc(uint8_t c) {
    LOCK();
    txRb->put_h_isr(c);
    tx_flush();
    CLEAR_LOCK();
  }

  void handle_realtime_message(uint8_t c);
};

class MidiUartUSBClass : public MidiUartClass {
public:
  Adafruit_USBD_MIDI usb_midi;
  bool usb_ready;

  MidiUartUSBClass(uart_inst_t *uart_hw, RingBuffer<> *_rxRb = nullptr,
                   RingBuffer<> *_txRb = nullptr)
      : MidiUartClass(uart_hw, _rxRb, _txRb) {

    usb_ready = false;
  }

  void init() {
    TinyUSBDevice.setID(0x1209, 0x3070); // Your pid.codes VID/PID
    TinyUSBDevice.setManufacturerDescriptor("MegaCMD (www.megacmd.com)");
    TinyUSBDevice.setProductDescriptor("MegaCommand");
    // Use TinyUSB MIDI interface
    usb_ready = false;
    usb_midi.begin();
  }
  void poll() {
    if (!usb_ready && TinyUSBDevice.mounted()) {
      usb_ready = true;
    }
    if (!usb_ready)
      return;
     if (mutex_try_enter(&__usb_mutex, nullptr)) {
  //     tud_task();
       receive();
       flush();
       mutex_exit(&__usb_mutex);
     }
  }

  void receive() {
    // Process incoming USB MIDI
    uint8_t packet[4];

    while (usb_midi.readPacket(packet)) {
      uint8_t cin = packet[0] & 0x0F;
      uint8_t len;

      // Determine length based on CIN
      switch (cin) {
      case 0x5: // Single-byte System Common or SysEx ends with 1 byte
      case 0xF: // Single Byte
        len = 1;
        break;

      case 0x2: // Two-byte System Common
      case 0x6: // SysEx ends with 2 bytes
      case 0xC: // Program Change
      case 0xD: // Channel Pressure
        len = 2;
        break;

      case 0x3: // Three-byte System Common
      case 0x4: // SysEx Start or Continue
      case 0x7: // SysEx ends with 3 bytes
      case 0x8: // Note Off
      case 0x9: // Note On
      case 0xA: // Poly-KeyPress
      case 0xB: // Control Change
      case 0xE: // PitchBend
        len = 3;
        break;

      default:
        continue;
      }

      // Put the actual MIDI bytes into the ring buffer
      for (uint8_t i = 0; i < len; i++) {
        if (rxRb)
          rxRb->put_h_isr(packet[i + 1]);
      }

      // Handle realtime messages immediately
      if (packet[1] >= 0xF8) {
        handle_realtime_message(packet[1]);
      }
    }
  }

  void flush() {

    // Process side channel first - takes precedence
    if (txRb_sidechannel && in_message_tx == 0) {
      while (!txRb_sidechannel->isEmpty()) {
        uint8_t c = txRb_sidechannel->peek();
        uint8_t packet[4] = {0x05, c, 0, 0};

        if (!usb_midi.writePacket(packet)) {
          return;
        }
        txRb_sidechannel->get(); // Only advance if send succeeded
      }
    }

    // Only process main TX buffer if sidechannel is empty
    while (txRb && !txRb->isEmpty()) {
      uint8_t c = txRb->peek();
      uint8_t packet[4] = {0x05, c, 0, 0};

      if (!usb_midi.writePacket(packet)) {
        break;
      }
      txRb->get(); // Only advance if send succeeded
    }
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
