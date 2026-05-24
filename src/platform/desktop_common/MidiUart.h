// MidiUart.h — desktop platform MIDI transport.
//
// Inherits MCL's cross-platform MidiUartParent (src/mcl/Midi/MidiUartParent.h)
// and backs the pure-virtual `m_getc / m_putc / m_putc_realtime / m_recv` with
// MCL's own RingBuffer<> templates over internal byte arrays. Exposes `rxRb`
// publicly because MCL's MidiClock.h reads it directly to inject realtime
// bytes (MidiClock.h:567,580,601). The host pumps bytes in/out via the
// desktop_entry.h C API (`mcl_inject_midi`, `mcl_drain_midi_out`).
#pragma once

#include "Arduino.h"
#include "platform.h"
#include "MidiUartParent.h"
#include "RingBuffer.h"

#include <stddef.h>
#include <stdint.h>

class MidiUartClass : public MidiUartParent {
public:
    static constexpr uint16_t RX_RING_SIZE = 1024;
    static constexpr uint16_t TX_RING_SIZE = 1024;
    static constexpr uint16_t RT_RING_SIZE = 64;

    MidiUartClass();

    // MidiUartParent contract.
    void init() override;
    uint8_t m_getc() override;
    void    m_putc(uint8_t* src, uint16_t size) override;
    void    m_putc(uint8_t c) override;
    void    m_putc_realtime(uint8_t c) override;
    void    m_recv(uint8_t* src, uint16_t size) override;
    bool    avail() override;

    // Desktop-only: bridge to the host's MIDI buffers.
    void   desktop_ingress(const uint8_t* data, size_t len);
    size_t desktop_egress(uint8_t* dst, size_t cap);

    // Hardware/USB stubs that the rp2040 MidiUart defines and some MCL paths
    // call through. Desktop has no baud-rate hardware, but keep `speed`
    // coherent because MCL's port setup uses it to avoid repeat TurboMIDI
    // negotiation.
    void set_speed(uint32_t bps) { speed = bps ? bps : 31250; }
    void poll() {}
    void service_irq() {}
    void service_background() {}
    void service_sof() {}
    void enable_sof_service() {}
    void flush() {}
    bool check_empty_tx() { return true; }
    void tx_flush() {}

    // ISR enable/disable — no IRQs on desktop. MCLSeq.cpp calls these.
    void enable_tx_irq()  {}
    void disable_tx_irq() {}

    // Exposed because MidiClock.h:567/580/601 reads `uart->rxRb->put_h_isr(...)`
    // to inject realtime bytes from the platform clock ISR.
    volatile RingBuffer<>* rxRb = nullptr;
    volatile RingBuffer<>* txRb = nullptr;
    volatile RingBuffer<>* txRb_realtime = nullptr;
    volatile RingBuffer<>* txRb_sidechannel = nullptr;

private:
    int8_t           in_message_tx_ = 0;
    uint8_t          rx_buf_[RX_RING_SIZE];
    uint8_t          tx_buf_[TX_RING_SIZE];
    uint8_t          rt_buf_[RT_RING_SIZE];
    RingBuffer<>     rx_storage_;
    RingBuffer<>     tx_storage_;
    RingBuffer<>     rt_storage_;
};

// USB MIDI subclass — adds the running_status_out flag MCL's setup() checks.
class MidiUartUSBClass : public MidiUartClass {
public:
    uint8_t running_status_out = 0;
};
