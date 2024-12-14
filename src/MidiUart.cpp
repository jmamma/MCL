// #include "MCLSeq.h"

#include "Arduino.h"
#include "MidiUart.h"
#include "Midi.h"
#include "MidiClock.h"
#include "pico.h"
#include "global.h"
#include "hardware/uart.h"

MidiUartClass::MidiUartClass(uart_inst_t *uart_hw_, RingBuffer *_rxRb,
                             RingBuffer *_txRb)
    : MidiUartParent() {
  uart_hw = uart_hw_;
  mode = UART_MIDI;
  rxRb = _rxRb;
  txRb = _txRb;
  txRb_sidechannel = nullptr;
}

void MidiUartClass::initSerial() {
  // Initialize GPIO pins for UART
  DEBUG_FUNC();

  if (uart_hw == uart0) {
    gpio_set_function(0, GPIO_FUNC_UART); // UART0 TX is GP0
    gpio_set_function(1, GPIO_FUNC_UART); // UART0 RX is GP1
    irq_set_exclusive_handler(UART0_IRQ, uart0_irq_handler);
  } else {
    gpio_set_function(4, GPIO_FUNC_UART); // UART1 TX is GP4
    gpio_set_function(5, GPIO_FUNC_UART); // UART1 RX is GP5
    irq_set_exclusive_handler(UART1_IRQ, uart1_irq_handler);
  }
  // Initialize UART for MIDI
  uart_init(uart_hw, UART_BAUDRATE);
  // 8 bits, 1 stop bit, no parity - standard MIDI format
  uart_set_format(uart_hw, 8, 1, UART_PARITY_NONE);
  // Disable FIFOs (realtime messages are 1 byte)
  uart_set_fifo_enabled(uart_hw, false);
  // Trigger interrupt clear for RX and TXs
  uart_get_hw(uart_hw)->icr = UART_UARTICR_RXIC_BITS | UART_UARTICR_TXIC_BITS;
  // Set high priority for UART interrupt
  irq_set_priority(uart_hw == uart0 ? UART0_IRQ : UART1_IRQ,
                   uart_hw == uart0 ? 0x40 : 0x40);
  // Set high priority for UART interrupt, uart0 has higher priority (as per
  // atmega2560)
  // irq_set_priority(uart_hw == uart0 ? UART0_IRQ : UART1_IRQ, uart_hw == uart0
  // ? 0x20 : 0x30);
  // Enable UART interrupt globally
  irq_set_enabled(uart_hw == uart0 ? UART0_IRQ : UART1_IRQ, true);
  // Enable RX interrupt only (TX enabled when needed)
  uart_set_irq_enables(uart_hw, true, false);
  // Give it some time to transmit
#ifdef RUNNING_STATUS_OUT
  running_status_enabled = true;
  running_status = 0;
#endif
}

void MidiUartClass::set_speed(uint32_t speed_) {
  // Wait for TX buffer to empty before changing speed
  while (!txRb->isEmpty())
    ;
  while (!uart_is_writable(uart_hw))
    ;

  uart_set_baudrate(uart_hw, speed_);
  speed = speed_;
}

void MidiUartClass::m_putc_immediate(uint8_t c) {
  uint32_t save = save_and_disable_interrupts();

  while (!uart_is_writable(uart_hw)) {
    /*
      if (TIMER_CHECK_INT(0)) { // Assuming timer 0 for clock
        g_fast_ticks++;
        TIMER_CLEAR_INT(0);
      }
      if (TIMER_CHECK_INT(1)) { // Assuming timer 1 for slowclock
        g_ms_ticks++;
        TIMER_CLEAR_INT(1);
      }
  */
  }

  sendActiveSenseTimer = sendActiveSenseTimeout;
  write_char(c);
  restore_interrupts(save);
}

void MidiUartClass::realtime_isr(uint8_t c) {
  if (c == MIDI_CLOCK) {
    if (MidiClock.uart_clock_recv == this) {
      MidiClock.handleClock();
      if (MidiClock.state != 2 || MidiClock.inCallback) {
        return;
      }
      MidiClock.inCallback = true;
      uint8_t _midi_lock_tmp = MidiUartParent::handle_midi_lock;
      MidiUartParent::handle_midi_lock = 1;
      //    mcl_seq.seq(); todo
      MidiUartParent::handle_midi_lock = _midi_lock_tmp;
      MidiClock.inCallback = false;
    }
  } else if (MidiClock.uart_transport_recv1 == this ||
             MidiClock.uart_transport_recv2 == this) {
    switch (c) {
    case MIDI_START:
      MidiClock.handleImmediateMidiStart();
      break;
    case MIDI_STOP:
      MidiClock.handleImmediateMidiStop();
      break;
    case MIDI_CONTINUE:
      MidiClock.handleImmediateMidiContinue();
      break;
    }
    rxRb->put_h_isr(c);
  }
}

void MidiUartClass::rx_isr() {
  uint32_t dr = uart_get_hw(uart_hw)->dr;
  uint8_t c = dr & 0xff; // Get the actual data byte

  const uint32_t ERROR_MASK = 0xf00; // Bits 8-11 are error flags
  bool has_errors = (dr & ERROR_MASK) != 0;
  if (has_errors) {
    return;
  }
  if (MIDI_IS_REALTIME_STATUS_BYTE(c)) {
    realtime_isr(c);
    return;
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
__attribute__((used, noinline)) void MidiUartClass::tx_isr() {
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
  } else {
    enable_tx_irq();
  }
}

extern "C" void uart0_irq_handler() {
  LOCK();
  uint32_t status =
      uart_get_hw(uart0)->mis; // Reading MIS clears the interrupts
  if (status & UART_UARTMIS_RXMIS_BITS) {
    MidiUart.rx_isr();
  }
  if (status & UART_UARTMIS_TXMIS_BITS) {
    MidiUart.tx_isr();
  }
  CLEAR_LOCK();
}

extern "C" void uart1_irq_handler() {
  LOCK();
  uint32_t status =
      uart_get_hw(uart1)->mis; // Reading MIS clears the interrupts
  if (status & UART_UARTMIS_RXMIS_BITS) {
    MidiUart2.rx_isr();
  }
  if (status & UART_UARTMIS_TXMIS_BITS) {
    MidiUart2.tx_isr();
  }
  CLEAR_LOCK();
}
