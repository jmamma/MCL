// MidiUart.cpp — desktop platform MIDI transport. Backed by MCL's own
// RingBuffer<> over private byte arrays. Single-threaded; the host pumps
// `desktop_ingress` / `desktop_egress` against its MIDI buffers.
#include "MidiUart.h"
#include "Midi.h"
#include "MidiSysex.h"
#include "midi-common.h"

MidiUartClass::MidiUartClass()
    : rx_storage_(rx_buf_, RX_RING_SIZE),
      tx_storage_(tx_buf_, TX_RING_SIZE),
      rt_storage_(rt_buf_, RT_RING_SIZE) {
    rxRb             = &rx_storage_;
    txRb             = &tx_storage_;
    txRb_realtime    = &rt_storage_;
    // No sidechannel on desktop; left null. MCL only reads it when set.
    speed = 31250;
    sendActiveSenseTimer = 0;
    sendActiveSenseTimeout = 0;
    recvActiveSenseTimer = 0;
    activeSenseEnabled = false;
    mode = 0;
    midi = nullptr;
}

void MidiUartClass::init() {
    rxRb->init();
    txRb->init();
    txRb_realtime->init();
}

bool MidiUartClass::avail() {
    return !rxRb->isEmpty();
}

uint8_t MidiUartClass::m_getc() {
    return rxRb->get();
}

void MidiUartClass::m_putc(uint8_t c) {
    txRb->put_h_isr(c);
}

void MidiUartClass::m_putc(uint8_t* src, uint16_t size) {
    txRb->put_h_isr(src, size);
}

void MidiUartClass::m_putc_realtime(uint8_t c) {
    txRb_realtime->put_h_isr(c);
}

void MidiUartClass::m_recv(uint8_t* src, uint16_t size) {
    rxRb->put_h_isr(src, size);
}

void MidiUartClass::desktop_ingress(const uint8_t* data, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        const uint8_t c = data[i];
        recvActiveSenseTimer = 0;

        if (MIDI_IS_REALTIME_STATUS_BYTE(c)) {
            rxRb->put_h_isr(c);
            continue;
        }

        if (midi && midi->midiSysex) {
            switch (live_state) {
            case midi_wait_sysex:
                if (MIDI_IS_STATUS_BYTE(c)) {
                    if (c != MIDI_SYSEX_END) {
                        midi->midiSysex->abort();
                        rxRb->put_h_isr(c);
                    } else {
                        midi->midiSysex->end_immediate();
                    }
                    live_state = midi_wait_status;
                } else {
                    midi->midiSysex->handleByte(c);
                }
                continue;

            case midi_wait_status:
                if (c == MIDI_SYSEX_START) {
                    live_state = midi_wait_sysex;
                    midi->midiSysex->reset();
                    continue;
                }
                break;

            default:
                break;
            }
        }

        rxRb->put_h_isr(c);
    }
}

size_t MidiUartClass::desktop_egress(uint8_t* dst, size_t cap) {
    size_t n = 0;
    // Drain realtime ring first; then main tx ring.
    while (n < cap && !txRb_realtime->isEmpty()) {
        dst[n++] = txRb_realtime->get();
    }
    while (n < cap && !txRb->isEmpty()) {
        dst[n++] = txRb->get();
    }
    return n;
}
