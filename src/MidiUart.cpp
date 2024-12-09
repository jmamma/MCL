//#include "MCLSeq.h"

#include "MidiClock.h"
#include "MidiUart.h"
#include "pico.h"

MidiUartClass::MidiUartClass(uart_inst_t *uart_hw, volatile uint8_t *rx_buf,
                             uint16_t rx_buf_size, volatile uint8_t *tx_buf,
                             uint16_t tx_buf_size)
    : MidiUartParent() {
  uart = uart_hw;
  mode = UART_MIDI;
  rxRb.ptr = rx_buf;
  rxRb.len = rx_buf_size;
  txRb.ptr = tx_buf;
  txRb.len = tx_buf_size;
  txRb_sidechannel = nullptr;
  initSerial();
}

void MidiUartClass::initSerial() {
  // Initialize UART for MIDI
  uart_init(uart, UART_BAUDRATE);

  // 8 bits, 1 stop bit, no parity - standard MIDI format
  uart_set_format(uart, 8, 1, UART_PARITY_NONE);

  // Set up and enable RX interrupt
  irq_set_enabled(uart == uart0 ? UART0_IRQ : UART1_IRQ, true);
  uart_set_irq_enables(uart, true, false);

#ifdef RUNNING_STATUS_OUT
  running_status_enabled = true;
  running_status = 0;
#endif
}

void MidiUartClass::set_speed(uint32_t speed_) {
  // Wait for TX buffer to empty before changing speed
  while (!txRb.isEmpty())
    ;
  while (!check_empty_tx())
    ;

  uart_set_baudrate(uart, speed_);
  speed = speed_;
}

void MidiUartClass::m_putc_immediate(uint8_t c) {
  uint32_t save = save_and_disable_interrupts();

  while (!check_empty_tx()) {
    if (TIMER_CHECK_INT(0)) { // Assuming timer 0 for clock
      clock++;
      TIMER_CLEAR_INT(0);
    }
    if (TIMER_CHECK_INT(1)) { // Assuming timer 1 for slowclock
      slowclock++;
      TIMER_CLEAR_INT(1);
    }
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
  //    mcl_seq.seq();
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
    rxRb.put_h_isr(c);
  }
}

// Interrupt handlers - these need to be set up in main()
extern "C" void uart0_irq_handler() {
  // Handle RX
  if (uart_is_readable(uart0)) {
    MidiUart.rx_isr();
  }

  // Handle TX
  if (uart_is_writable(uart0)) {
    MidiUart.tx_isr();
  }
}

extern "C" void uart1_irq_handler() {
  if (uart_is_readable(uart1)) {
    MidiUart2.rx_isr();
  }

  if (uart_is_writable(uart1)) {
    MidiUart2.tx_isr();
  }
}

// Global instances
MidiUartClass MidiUart(uart0, midi_rx_buf, RX_BUF_SIZE, midi_tx_buf,
                       TX_BUF_SIZE);
MidiUartClass MidiUart2(uart1, midi2_rx_buf, RX_BUF_SIZE, midi2_tx_buf,
                        TX_BUF_SIZE);
