//#include "MCLSeq.h"

#include "Arduino.h"
#include "MidiUart.h"
#include "Midi.h"
#include "MidiClock.h"
#include "pico.h"
#include "global.h"

MidiUartClass::MidiUartClass(uart_inst_t *uart_hw_, RingBuffer *_rxRb , RingBuffer *_txRb)
    : MidiUartParent() {
  uart_hw = uart_hw;
  mode = UART_MIDI;
  rxRb = _rxRb;
  txRb = _txRb;
  txRb_sidechannel = nullptr;
}

void MidiUartClass::initSerial() {
  // Initialize UART for MIDI
  uart_init(uart_hw, UART_BAUDRATE);

  // 8 bits, 1 stop bit, no parity - standard MIDI format
  uart_set_format(uart_hw, 8, 1, UART_PARITY_NONE);

  // Set up and enable RX interrupt
  irq_set_enabled(uart_hw == uart0 ? UART0_IRQ : UART1_IRQ, true);
  uart_set_irq_enables(uart_hw, true, false);

#ifdef RUNNING_STATUS_OUT
  running_status_enabled = true;
  running_status = 0;
#endif
}

void MidiUartClass::set_speed(uint32_t speed_) {
  // Wait for TX buffer to empty before changing speed
  while (!txRb->isEmpty())
    ;
  while (!check_empty_tx())
    ;

  uart_set_baudrate(uart_hw, speed_);
  speed = speed_;
}

void MidiUartClass::m_putc_immediate(uint8_t c) {
  uint32_t save = save_and_disable_interrupts();

  while (!check_empty_tx()) {
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
    rxRb->put_h_isr(c);
  }
}

void MidiUartClass::rx_isr() {
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

void MidiUartClass::tx_isr() {
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
