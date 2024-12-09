#ifndef MIDI_UART_H__
#define MIDI_UART_H__

#include "pico.h"
#include "RingBuffer.h"
#include <MidiUartParent.h>
#include "Midi.h"

// RP2040 has 2 UARTs
#define UART_MIDI 0
#define UART_SERIAL 1

#define UART_BAUDRATE 31250

// Buffer size definitions same as original
#if (RX_BUF_SIZE >= 256)
#define RX_BUF_TYPE uint16_t
#else
#define RX_BUF_TYPE uint8_t
#endif

#if (TX_BUF_SIZE >= 256)
#define TX_BUF_TYPE uint16_t
#else
#define TX_BUF_TYPE uint8_t
#endif

// Timer check macros for RP2040
#define TIMER_CHECK_INT(timer_num) ((timer_hw->intr & (1u << timer_num)) != 0)
#define TIMER_CLEAR_INT(timer_num) timer_hw->intr = (1u << timer_num)

class MidiUartClass : public MidiUartParent {
public:
    MidiUartClass(uart_inst_t* uart_hw, volatile uint8_t *rx_buf,
                  uint16_t rx_buf_size, volatile uint8_t *tx_buf,
                  uint16_t tx_buf_size);

    ALWAYS_INLINE() bool avail() { return !rxRb.isEmpty(); }
    ALWAYS_INLINE() uint8_t m_getc() { return rxRb.get(); }

    void initSerial();
    void set_speed(uint32_t speed);
    void m_putc_immediate(uint8_t c);

    // Interrupt handlers
    ALWAYS_INLINE() void rx_isr();
    ALWAYS_INLINE() void tx_isr();
    ALWAYS_INLINE() void realtime_isr(uint8_t c);

    // MIDI message handling
    ALWAYS_INLINE() void m_recv(uint8_t *src, uint16_t size) {
        rxRb.put_h_isr(src, size);
    }

    ALWAYS_INLINE() void m_putc(uint8_t *src, uint16_t size) {
        txRb.put_h_isr(src, size);
        enable_tx_irq();
    }

    ALWAYS_INLINE() void m_putc(uint8_t c) {
        txRb.put_h_isr(c);
        enable_tx_irq();
    }

#ifdef RUNNING_STATUS_OUT
    uint8_t running_status;
    bool running_status_enabled;
#endif

private:
    uart_inst_t* uart;
    int8_t in_message_tx;

    ALWAYS_INLINE() bool write_char(uint8_t c) {
#ifdef RUNNING_STATUS_OUT
        if (!running_status_enabled) {
            uart_write_blocking(uart, &c, 1);
            return true;
        }
        if (MIDI_IS_STATUS_BYTE(c) && MIDI_IS_VOICE_STATUS_BYTE(c)) {
            if (c != running_status) {
                running_status = c;
                uart_write_blocking(uart, &c, 1);
                return true;
            }
            return false;
        }
#endif
        uart_write_blocking(uart, &c, 1);
        return true;
    }

    ALWAYS_INLINE() uint8_t read_char() {
        return uart_getc(uart);
    }

    ALWAYS_INLINE() bool check_empty_tx() {
        return uart_is_writable(uart);
    }

    ALWAYS_INLINE() void enable_tx_irq() {
        uart_set_irq_enables(uart, true, true);
    }

    ALWAYS_INLINE() void disable_tx_irq() {
        uart_set_irq_enables(uart, true, false);
    }

public:
    volatile RingBuffer<tx_buf_size, RX_BUF_TYPE> rxRb;
    volatile RingBuffer<tx_buf_size, TX_BUF_TYPE> txRb;
    volatile RingBuffer<tx_buf_size, TX_BUF_TYPE> *txRb_sidechannel;
};

// Global instances
extern MidiUartClass MidiUart;      // UART0
extern MidiUartClass MidiUart2;     // UART1

#endif /* MIDI_UART_H__ */
