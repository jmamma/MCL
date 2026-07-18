// MidiUart.cpp — desktop platform MIDI transport. Backed by MCL's own
// RingBuffer<> over private byte arrays. Single-threaded; the host pumps
// `desktop_ingress` / `desktop_egress` against its MIDI buffers.
#include "MidiUart.h"
#include "Midi.h"
#include "MidiClock.h"
#include "MidiSysex.h"
#include "midi-common.h"
#include "Sequencer/MCLSeq.h"

namespace {

void update_tx_message_state(int8_t& in_message_tx, uint8_t c) {
    if ((in_message_tx > 0) && (c < 128)) {
        in_message_tx--;
        return;
    }
    if (c < 0xF0) {
        int8_t data_len = 0;
        switch (c & 0xF0) {
        case MIDI_CHANNEL_PRESSURE:
        case MIDI_PROGRAM_CHANGE:
            data_len = 1;
            break;
        case MIDI_NOTE_OFF:
        case MIDI_NOTE_ON:
        case MIDI_AFTER_TOUCH:
        case MIDI_CONTROL_CHANGE:
        case MIDI_PITCH_WHEEL:
            data_len = 2;
            break;
        }
        if (data_len > 0)
            in_message_tx = data_len;
    } else {
        switch (c) {
        case MIDI_SYSEX_START:
            in_message_tx = -1;
            break;
        case MIDI_SYSEX_END:
            in_message_tx = 0;
            break;
        case MIDI_MTC_QUARTER_FRAME:
        case MIDI_SONG_SELECT:
            in_message_tx = 1;
            break;
        case MIDI_SONG_POSITION_PTR:
            in_message_tx = 2;
            break;
        }
    }
}

}

MidiUartClass::MidiUartClass(uint8_t* external_tx_buffer,
                             uint16_t external_tx_size)
    : rx_storage_(rx_buf_, RX_RING_SIZE),
      tx_storage_(external_tx_buffer ? external_tx_buffer : tx_buf_,
                  external_tx_buffer && external_tx_size
                      ? external_tx_size
                      : TX_RING_SIZE),
      rt_storage_(rt_buf_, RT_RING_SIZE) {
    rxRb             = &rx_storage_;
    txRb             = &tx_storage_;
    txRb_realtime    = &rt_storage_;
    // No sidechannel on desktop; left null. MCL only reads it when set.
    speed = 31250;
    sendActiveSenseTimer = 0;
    sendActiveSenseTimeout = 0;
    // No peer has spoken yet. Starting at zero makes MidiActivePeering treat
    // the first 100 ms of a fast hosted boot as recent MIDI traffic.
    recvActiveSenseTimer = 0xffff;
    activeSenseEnabled = false;
    mode = 0;
    midi = nullptr;
}

void MidiUartClass::init() {
    rxRb->init();
    txRb->init();
    txRb_realtime->init();
    txRb_sidechannel = nullptr;
    recvActiveSenseTimer = 0xffff;
    in_message_tx_ = 0;
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

bool MidiUartClass::desktop_ingress_byte(uint8_t c) {
    recvActiveSenseTimer = 0;

    if (MIDI_IS_REALTIME_STATUS_BYTE(c)) {
        if (c == MIDI_CLOCK) {
            if (MidiClock.uart_clock_recv == this) {
                MidiClock.handleClock();
                if (MidiClock.state == MidiClockClass::STARTED &&
                    !MidiClock.inCallback) {
                    MidiClock.inCallback = true;
                    uint8_t old_lock = MidiUartParent::handle_midi_lock;
                    MidiUartParent::handle_midi_lock = 1;
                    mcl_seq.seq();
                    MidiUartParent::handle_midi_lock = old_lock;
                    MidiClock.inCallback = false;
                }
            }
            // Hardware consumes clock from non-selected inputs.
            return true;
        }

        // Check capacity before applying transport side effects.
        if (rxRb->isFull())
            return false;
        if (MidiClock.uart_transport_recv1 == this ||
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
        }
        rxRb->put_h_isr(c);
        return true;
    }

    if (midi && midi->midiSysex) {
        switch (live_state) {
        case midi_wait_sysex:
            if (MIDI_IS_STATUS_BYTE(c)) {
                if (c != MIDI_SYSEX_END) {
                    if (rxRb->isFull())
                        return false;
                    midi->midiSysex->abort();
                    rxRb->put_h_isr(c);
                } else {
                    midi->midiSysex->end_immediate();
                }
                live_state = midi_wait_status;
            } else {
                midi->midiSysex->handleByte(c);
            }
            return true;

        case midi_wait_status:
            if (c == MIDI_SYSEX_START) {
                live_state = midi_wait_sysex;
                midi->midiSysex->reset();
                return true;
            }
            break;

        default:
            break;
        }
    }

    if (rxRb->isFull())
        return false;
    rxRb->put_h_isr(c);
    return true;
}

size_t MidiUartClass::desktop_ingress(const uint8_t* data, size_t len) {
    size_t accepted = 0;
    while (accepted < len && desktop_ingress_byte(data[accepted]))
        ++accepted;
    return accepted;
}

size_t MidiUartClass::desktop_egress(uint8_t* dst, size_t cap) {
    size_t n = 0;
    // Match the hardware UART ISR priority:
    //   realtime can interrupt any stream,
    //   side-channel can start only between normal MIDI messages,
    //   normal TX advances the message-boundary tracker.
    while (n < cap) {
        if (!txRb_realtime->isEmpty()) {
            dst[n++] = txRb_realtime->get();
            continue;
        }

        if (txRb_sidechannel && in_message_tx_ == 0) {
            if (!txRb_sidechannel->isEmpty()) {
                const uint8_t c = txRb_sidechannel->get();
                dst[n++] = c;
                continue;
            }
            txRb_sidechannel = nullptr;
        }

        if (!txRb->isEmpty()) {
            const uint8_t c = txRb->get();
            dst[n++] = c;
            update_tx_message_state(in_message_tx_, c);
            continue;
        }

        break;
    }
    return n;
}
