// platform.cpp — wasm implementations for time + GPIO mock state.
//
// Same surface as src/platform/desktop/platform.cpp, but `millis`/`micros`
// route through host imports instead of std::chrono. The wasm runtime
// can't read host wall-clock without the host's help.
#include "platform.h"
#include "desktop_entry.h"
#include "host_imports.h"
#include "wasm_exports.h"
#include "GUI_hardware.h"

extern uint64_t mcl_desktop_button_mask;
extern volatile uint16_t g_clock_ms;
extern void handleIncomingMidi();
__attribute__((visibility("hidden")))
void mcl_wasm_pump_host_midi_input(uint32_t max_bytes_per_port);

namespace {

constexpr uint8_t kMidiSysexStart = 0xF0;
constexpr uint8_t kMidiSysexEnd = 0xF7;
constexpr uint16_t kMaxMidiOutputBytesPerPoll = 256;
constexpr uint16_t kMaxMidiOutputBytesPerWaitPoll = 4096;
constexpr uint16_t kMaxMidiInputBytesPerPoll = 4096;

struct MidiOutputPumpState {
    bool in_sysex = false;
};

MidiOutputPumpState midi_output_pump_state[3];

void pump_host_midi_input() {
    mcl_wasm_pump_host_midi_input(kMaxMidiInputBytesPerPoll);
}

void pump_host_midi_output_port(int port, uint16_t max_bytes,
                                bool stop_after_sysex) {
    MidiOutputPumpState& state = midi_output_pump_state[port - MCL_MIDI_UART];
    uint16_t count = 0;

    while (count < max_bytes) {
        int32_t byte_val = mcl_midi_out_pop(port);
        if (byte_val < 0)
            break;

        uint8_t byte = (uint8_t)byte_val;
        host_midi_out_push(port, byte);
        ++count;

        if (byte == kMidiSysexStart) {
            state.in_sysex = true;
        } else if (byte == kMidiSysexEnd && state.in_sysex) {
            state.in_sysex = false;
            if (stop_after_sysex)
                break;
        }
    }
}

void pump_host_midi_output() {
    for (int port = MCL_MIDI_UART; port <= MCL_MIDI_USB; ++port) {
        pump_host_midi_output_port(port, kMaxMidiOutputBytesPerPoll, true);
    }
}

void pump_host_midi_output_for_wait() {
    for (int port = MCL_MIDI_UART; port <= MCL_MIDI_USB; ++port) {
        pump_host_midi_output_port(port, kMaxMidiOutputBytesPerWaitPoll, false);
    }
}

void drain_pending_audio_time() {
    if (!mcl_desktop_is_setup_done()) {
        g_clock_ms = (uint16_t)millis();
        return;
    }

#ifdef MCL_WASM_DISABLE_SOFTWARE_IRQ
    g_clock_ms = (uint16_t)millis();
    return;
#else
    const uint32_t elapsed_us = host_audio_pending_us();
    if (elapsed_us != 0)
        mcl_tick_audio(elapsed_us);
#endif
}

}  // namespace

unsigned long millis() {
    return host_millis();
}

unsigned long micros() {
    return host_micros();
}

// delay() / delayMicroseconds() are no-ops on wasm. The Arduino loop runs
// cooperatively on the host's timer, never blocking.
void delay(unsigned long /*ms*/) {}
void delayMicroseconds(unsigned int /*us*/) {}

void platform_poll() {
    GUI_hardware.poll();
    drain_pending_audio_time();
    pump_host_midi_input();
#ifdef MCL_WASM_DISABLE_SOFTWARE_IRQ
    handleIncomingMidi();
#endif
    pump_host_midi_output();
#ifdef DEBUGMODE
    mcl_debug::flush();
#endif
    host_yield();
}

void platform_wait_poll() {
    g_clock_ms = (uint16_t)millis();
    // Blocking sysex waits often enter with a request already queued in the
    // MCL UART TX ring. Drain the whole bounded pending batch, not just the
    // first sysex: older status exchanges can otherwise sit in front of the
    // request this wait is actually waiting for.
    pump_host_midi_output_for_wait();
    // Modal waits run inside the service thread's existing WAMR entry. Advance
    // the virtual hardware timers to the next published host sample before
    // exposing timestamped MIDI, exactly as platform_poll() does. Reversing
    // this order makes an on-time 0xF8 appear early or late to MidiClock.
    drain_pending_audio_time();
    pump_host_midi_input();
    // handleIncomingMidi() dispatches already-recorded sysex before it drains
    // UART bytes into the sysex recorder. Prime that first stage here so the
    // caller's normal handleIncomingMidi() can dispatch replies before it checks
    // the blocking timeout.
    handleIncomingMidi();
    pump_host_midi_output_for_wait();
#ifdef DEBUGMODE
    mcl_debug::flush();
#endif
    host_yield();
}

bool mcl_platform_headless_boot() {
    return host_headless_boot() != 0;
}

uint64_t mcl_platform_button_mask() {
    return host_input_button_mask() | mcl_desktop_button_mask;
}

int mcl_platform_encoder_delta(uint8_t encoder_id) {
    if (encoder_id >= GUI_NUM_ENCODERS)
        return 0;
    return (int)host_input_encoder_delta((int32_t)encoder_id);
}

uint32_t mcl_platform_encoder_button_mask() {
    uint32_t mask = host_input_encoder_button_mask();
    for (uint8_t i = 0; i < GUI_NUM_ENCODERS; ++i) {
        if (Encoders.encoders[i].button)
            mask |= (1u << i);
    }
    return mask;
}

#if defined(MCL_HAS_DESKTOP_MOUSE)
static_assert(sizeof(mcl_mouse_event_t) == 12,
              "mcl_mouse_event_t must match the SPS host ABI");

bool mcl_platform_mouse_pop(mcl_mouse_event_t* out) {
    if (!out)
        return false;
    return host_input_pointer_pop(out, (int32_t)sizeof(*out)) ==
           (int32_t)sizeof(*out);
}
#endif

// SW_IRQ storage — referenced via extern in platform.h. Values are
// irrelevant; the TRIGGER_SW_IRQ*/CLEAR_SW_IRQ* macros are no-ops.
uint8_t SW_IRQ1 = 0;
uint8_t SW_IRQ2 = 0;
uint8_t SW_IRQ3 = 0;
