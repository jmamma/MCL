#pragma once

#include "RingBuffer.h"
#include "memory.h"
#include "pico.h"
#include "platform.h"
#include <MidiUartParent.h>
#include <pico/mutex.h>
#include <arduino/midi/Adafruit_USBD_MIDI.h>
#include "tusb.h"
#include "hardware.h"

#if !defined(USE_TINYUSB)
#error "RP2040 USB MIDI requires the Adafruit TinyUSB stack"
#endif

extern mutex_t __usb_mutex;

#define UART_BAUDRATE 31250

// Timer check macros for RP2040
#define TIMER_CHECK_INT(timer_num) ((timer_hw->intr & (1u << timer_num)) != 0)
#define TIMER_CLEAR_INT(timer_num) timer_hw->intr = (1u << timer_num)
#define UART_MIDI 0
#define UART_SERIAL 1
#define UART_P4_SPI 2

#ifdef PLATFORM_TBD
class TbdP4RealtimeTransport;
#endif

class MidiUartClass : public MidiUartParent {
private:
  uart_inst_t *uart_hw;
  uint8_t mode;
#ifdef PLATFORM_TBD
  TbdP4RealtimeTransport *p4_transport;
#endif

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
  volatile RingBuffer<> *txRb_realtime;

#ifdef RUNNING_STATUS_OUT
  uint8_t running_status;
  bool running_status_enabled;
#endif

  MidiUartClass(uart_inst_t *uart_hw, RingBuffer<> *_rxRb = nullptr,
                RingBuffer<> *_txRb = nullptr, RingBuffer<> *_txRb_realtime = nullptr);

#ifdef PLATFORM_TBD
  void attach_p4_transport(TbdP4RealtimeTransport *transport);
  void flush_p4_transport();
#endif

  void realtime_isr(uint8_t c);
  void rx_isr();
  void tx_isr();
  ALWAYS_INLINE() void enable_tx_irq() {
    if (!uart_hw) return;
    uart_set_irq_enables(uart_hw, true, true);
  }

  ALWAYS_INLINE() void disable_tx_irq() {
    if (!uart_hw) return;
    uart_set_irq_enables(uart_hw, true, false);
    uart_get_hw(uart_hw)->icr = UART_UARTICR_TXIC_BITS;
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
#ifdef PLATFORM_TBD
    if (p4_transport) {
      flush_p4_transport();
      return;
    }
#endif
    if (uart_hw) { // Important, don't allow flush for non-hw uarts.
      if (uart_is_writable(uart_hw)) {
        tx_isr();
      } else {
        enable_tx_irq();
      }
    }
  }

  ALWAYS_INLINE() void m_putc_realtime(uint8_t c) {
    LOCK();
    txRb_realtime->put_h_isr(c);
    tx_flush();
    CLEAR_LOCK();
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

#ifdef PLATFORM_TBD
class MidiUartP4Class : public MidiUartClass {
public:
  MidiUartP4Class(RingBuffer<> *_rxRb = nullptr, RingBuffer<> *_txRb = nullptr,
                  RingBuffer<> *_txRb_realtime = nullptr)
      : MidiUartClass(nullptr, _rxRb, _txRb, _txRb_realtime) {}

  void init();
  void poll();
  void service_irq();
  void service_background();
  void set_speed(uint32_t speed) { (void)speed; }
  void m_putc_immediate(uint8_t c) { m_putc(c); }
};
#endif

class MidiUartUSBClass : public MidiUartClass {
public:
  Adafruit_USBD_MIDI usb_midi;
  bool usb_ready;
  bool in_sysex;
  bool usb_midi_started;

  // TX packetization state
  bool tx_in_sysex;
  uint8_t tx_data_cnt;
  int8_t tx_message_len;
  uint8_t tx_packet[4];
  bool sof_service_enabled;

  static constexpr uint8_t USB_REALTIME_FLUSH_BUDGET = 16;


  MidiUartUSBClass(uart_inst_t *uart_hw, RingBuffer<> *_rxRb = nullptr,
                   RingBuffer<> *_txRb = nullptr, RingBuffer<> *_txRb_realtime = nullptr)
      : MidiUartClass(uart_hw, _rxRb, _txRb, _txRb_realtime) {

    usb_ready = false;
    in_sysex = false;
    usb_midi_started = false;
    tx_in_sysex = false;
    tx_data_cnt = 0;
    tx_message_len = -1;
    sof_service_enabled = false;
    memset(tx_packet, 0, 4);
  }

  void enable_sof_service() {
    if (usb_midi_started && !sof_service_enabled && tud_inited()) {
      tud_sof_cb_enable(true);
      sof_service_enabled = true;
    }
  }

  void init() {
    TinyUSBDevice.detach();
    delay(10);
    TinyUSBDevice.setID(0x1209, 0x3070); // Your pid.codes VID/PID
    TinyUSBDevice.setManufacturerDescriptor("MegaCMD (www.megacmd.com)");
    TinyUSBDevice.setProductDescriptor("MegaCommand");
    // Use TinyUSB MIDI interface
    usb_ready = false;
    usb_midi_started = usb_midi.begin();
    TinyUSBDevice.attach();
    enable_sof_service();
  }
  void poll() {
    if (__get_current_exception()) {
      service_irq();
      return;
    }
    service_background();
  }

  void service_irq() {
    // TinyUSB endpoint APIs are serviced from the USB task/main loop. Incoming
    // MIDI is already drained by tud_midi_rx_cb(), including realtime clock.
  }

  void service_sof() {
    if (!usb_midi_started || !tud_mounted()) {
      return;
    }
    usb_ready = true;
    flush_realtime();
  }

  void service_background() {
    if (!usb_midi_started) {
      return;
    }
    if (!usb_ready && TinyUSBDevice.mounted()) {
      usb_ready = true;
      enable_sof_service();
    }
    if (!usb_ready)
      return;
    if (mutex_try_enter(&__usb_mutex, nullptr)) {
      tud_task();
      receive();
      flush();
      mutex_exit(&__usb_mutex);
    }
  }

  void m_putc_immediate(uint8_t c) {
    // Route through ring buffer to avoid state machine conflicts with flush()
    m_putc(c);
  }

  void set_speed(uint32_t speed) {
    // No-op for USB MIDI — speed is fixed by USB
  }

  void receive() {
    // Process incoming USB MIDI
    uint8_t packet[4];

    while (usb_midi.readPacket(packet)) {
      recvActiveSenseTimer = 0;
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

      // Handle sysex via CIN codes, non-sysex via rxRb
      switch (cin) {
      case 0x4: // SysEx Start or Continue
        for (uint8_t j = 1; j <= 3; j++) {
          uint8_t c = packet[j];
          if (c == MIDI_SYSEX_START) {
            midi->midiSysex->reset();
            in_sysex = true;
          } else {
            midi->midiSysex->handleByte(c);
          }
        }
        continue;
      case 0x5: // SysEx ends with 1 byte (F7)
        if (in_sysex) {
          midi->midiSysex->end_immediate();
          in_sysex = false;
        }
        continue;
      case 0x6: // SysEx ends with 2 bytes (data + F7)
        if (in_sysex) {
          midi->midiSysex->handleByte(packet[1]);
          midi->midiSysex->end_immediate();
          in_sysex = false;
        }
        continue;
      case 0x7: // SysEx ends with 3 bytes (data + data + F7)
        if (in_sysex) {
          midi->midiSysex->handleByte(packet[1]);
          midi->midiSysex->handleByte(packet[2]);
          midi->midiSysex->end_immediate();
          in_sysex = false;
        }
        continue;
      case 0xF: // Single Byte — macOS may send mid-sysex data with this CIN
        if (packet[1] == 0xF0) {
          midi->midiSysex->reset();
          in_sysex = true;
          continue;
        }
        if (in_sysex) {
          if (packet[1] == 0xF7) {
            midi->midiSysex->end_immediate();
            in_sysex = false;
          } else if (!MIDI_IS_STATUS_BYTE(packet[1])) {
            midi->midiSysex->handleByte(packet[1]);
          }
          continue;
        }
        break;
      default:
        break;
      }

      // Non-sysex: route to rxRb
      for (uint8_t i = 0; i < len; i++) {
        uint8_t c = packet[i + 1];
        if (MIDI_IS_REALTIME_STATUS_BYTE(c)) {
          handle_realtime_message(c);
        } else {
          rxRb->put_h_isr(c);
        }
      }
    }
  }

  bool flush_byte(uint8_t c) {
    // Realtime bytes: send immediately as CIN 0xF
    if (MIDI_IS_REALTIME_STATUS_BYTE(c)) {
      uint8_t pkt[4] = {0x0F, c, 0, 0};
      return usb_midi.writePacket(pkt);
    }

    if (MIDI_IS_STATUS_BYTE(c)) {
      tx_message_len = -1;
      if (c < 0xF0) {
        // Channel voice messages
        tx_in_sysex = false;
        switch (c & 0xF0) {
        case 0x80: tx_packet[0] = 0x08; tx_message_len = 2; break; // Note Off
        case 0x90: tx_packet[0] = 0x09; tx_message_len = 2; break; // Note On
        case 0xA0: tx_packet[0] = 0x0A; tx_message_len = 2; break; // Poly Aftertouch
        case 0xB0: tx_packet[0] = 0x0B; tx_message_len = 2; break; // CC
        case 0xC0: tx_packet[0] = 0x0C; tx_message_len = 1; break; // Program Change
        case 0xD0: tx_packet[0] = 0x0D; tx_message_len = 1; break; // Channel Pressure
        case 0xE0: tx_packet[0] = 0x0E; tx_message_len = 2; break; // Pitch Bend
        }
      } else {
        // System common
        switch (c) {
        case 0xF0: // SysEx Start
          tx_packet[0] = 0x04;
          tx_in_sysex = true;
          tx_data_cnt = 0;
          tx_packet[1] = c;
          tx_data_cnt++;
          return true;
        case 0xF7: // SysEx End
          if (!tx_in_sysex) break;
          // End packet CIN depends on how many data bytes buffered
          if (tx_data_cnt == 0) {
            uint8_t pkt[4] = {0x05, 0xF7, 0, 0};
            if (!usb_midi.writePacket(pkt)) return false;
          } else if (tx_data_cnt == 1) {
            tx_packet[0] = 0x06;
            tx_packet[2] = 0xF7;
            tx_packet[3] = 0;
            if (!usb_midi.writePacket(tx_packet)) return false;
          } else { // tx_data_cnt == 2
            tx_packet[0] = 0x07;
            tx_packet[3] = 0xF7;
            if (!usb_midi.writePacket(tx_packet)) return false;
          }
          tx_in_sysex = false;
          tx_data_cnt = 0;
          return true;
        case 0xF1: // MTC Quarter Frame
          tx_packet[0] = 0x02; tx_message_len = 1; break;
        case 0xF2: // Song Position
          tx_packet[0] = 0x03; tx_message_len = 2; break;
        case 0xF3: // Song Select
          tx_packet[0] = 0x02; tx_message_len = 1; break;
        default:   // Tune Request, etc
          tx_packet[0] = 0x05;
          tx_in_sysex = false;
          tx_data_cnt = 0;
          tx_message_len = 0;
          break;
        }
      }
      tx_data_cnt = 0;
      if (tx_message_len >= 0) {
        tx_in_sysex = false;
        tx_packet[1] = c;
        tx_packet[2] = 0;
        tx_packet[3] = 0;
        tx_data_cnt++;
        if (tx_message_len == 0) {
          tx_data_cnt = 0;
          return usb_midi.writePacket(tx_packet);
        }
      }
      return true;
    }

    // Data byte
    if (tx_in_sysex) {
      tx_packet[1 + tx_data_cnt] = c;
      tx_data_cnt++;
      if (tx_data_cnt == 3) {
        tx_packet[0] = 0x04; // SysEx continue
        if (!usb_midi.writePacket(tx_packet)) {
          tx_data_cnt--; // undo; byte will be re-peeked
          return false;
        }
        tx_data_cnt = 0;
      }
      return true;
    }

    if (tx_message_len > 0) {
      tx_packet[1 + tx_data_cnt] = c;
      tx_data_cnt++;
      tx_message_len--;
      if (tx_message_len == 0) {
        if (!usb_midi.writePacket(tx_packet)) {
          tx_data_cnt--;
          tx_message_len++;
          return false;
        }
        tx_data_cnt = 0;
      }
    }
    return true;
  }

  bool flush_realtime(uint8_t budget = USB_REALTIME_FLUSH_BUDGET) {
    while (txRb_realtime && budget-- && !txRb_realtime->isEmpty()) {
      uint8_t c = txRb_realtime->peek();
      if (!flush_byte(c)) {
        return false;
      }
      txRb_realtime->get();
    }
    return true;
  }

  void flush() {
    if (!flush_realtime()) {
      return;
    }

    // Process side channel first - takes precedence
    if (txRb_sidechannel && in_message_tx == 0) {
      while (!txRb_sidechannel->isEmpty()) {
        uint8_t c = txRb_sidechannel->peek();
        if (!flush_byte(c)) {
          return;
        }
        txRb_sidechannel->get();
      }
    }

    // Only process main TX buffer if sidechannel is empty
    while (txRb && !txRb->isEmpty()) {
      uint8_t c = txRb->peek();
      if (!flush_byte(c)) {
        break;
      }
      txRb->get();
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
