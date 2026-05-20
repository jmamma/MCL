// exports.cpp — wasm-side implementations of the mcl_* exports the host
// calls through WAMR. Wraps desktop_entry.cpp's mcl_desktop_* trampolines
// so the host doesn't need to know about the "desktop_" name (kept for
// source-shared compatibility with the static-link path).
//
// Input state (encoder/button) is normally pull-model: GUI_hardware.poll()
// asks the host via host_input_* so modal loops can receive events while
// wasm is already executing. mcl_input_set_* remains as a compatibility
// export for non-modal hosts.
//
// Single-threaded — see ABI.md.
#include "wasm_exports.h"
#include "host_imports.h"   // mcl_midi_port_t
#include "desktop_entry.h"

#include "oled.h"
#include "MidiUart.h"
#include "GUI_hardware.h"
#include "MidiClock.h"
#include "MCL.h"
#include "MCLSeq.h"

#include <stdint.h>
#include <string.h>

// MCL globals provided by global.cpp / GUI_hardware.cpp.
extern Oled             oled_display;
extern MidiUartClass    MidiUart;
extern MidiUartClass    MidiUart2;
extern MidiUartUSBClass MidiUartUSB;
extern EncodersClass    Encoders;
extern ButtonsClass     Buttons;
extern uint64_t         mcl_desktop_button_mask;
extern MidiClockClass   MidiClock;
extern MCLSeq           mcl_seq;
extern MCL              mcl;
extern void handleIncomingMidi();

extern volatile uint16_t g_clock_ms;
extern volatile uint16_t g_clock_fast;
extern volatile uint16_t g_clock_ticks;
extern volatile uint16_t g_clock_minutes;

// ABI version. Bump major when removing/renaming/changing signatures.
static constexpr uint16_t MCL_ABI_MAJOR = 1;
static constexpr uint16_t MCL_ABI_MINOR = 3;

static uint32_t s_timer1_remainder_us = 0;
static uint32_t s_timer2_remainder_us = 0;
static constexpr uint32_t kMaxHostMidiPumpBytesPerPort = 4096;

static void pump_host_midi_input_for_audio() {
    for (int32_t port = MCL_MIDI_UART; port <= MCL_MIDI_USB; ++port) {
        uint32_t count = 0;
        while (count < kMaxHostMidiPumpBytesPerPort) {
            int32_t byte_val = host_midi_in_pop(port);
            if (byte_val < 0)
                break;
            mcl_midi_in_push(port, (uint8_t)byte_val);
            ++count;
        }
    }
}

static void pump_host_midi_output_for_audio() {
    for (int32_t port = MCL_MIDI_UART; port <= MCL_MIDI_USB; ++port) {
        uint32_t count = 0;
        while (count < kMaxHostMidiPumpBytesPerPort) {
            int32_t byte_val = mcl_midi_out_pop(port);
            if (byte_val < 0)
                break;
            host_midi_out_push(port, (uint8_t)byte_val);
            ++count;
        }
    }
}

// ---- Lifecycle -----------------------------------------------------------

extern "C" void mcl_setup(void) {
    s_timer1_remainder_us = 0;
    s_timer2_remainder_us = 0;
    mcl_desktop_setup();
}

// Audio-thread entry. Catches up MCL's timer-ISR equivalents for the
// elapsed period, then dispatches the softirq-equivalent work
// (sequencer step firing + incoming MIDI handling). Replicates the
// rp2040/irqs.cpp:timer1_handler + timer2_handler + softirq1 + softirq2
// behaviour without an actual ISR.
extern "C" void mcl_tick_audio(uint32_t elapsed_us) {
    if (!mcl_desktop_is_setup_done()) return;
    if (elapsed_us == 0) return;

    // Pull host-queued MIDI bytes while already inside this wasm export. The
    // host audio thread only queues bytes; it does not make one WAMR export
    // call per byte.
    pump_host_midi_input_for_audio();

    handleIncomingMidi();

    // timer2 work (5 kHz on hardware → one tick per 200 µs).
    constexpr uint32_t kTimer2PeriodUs = 200;
    uint64_t fast_total_us = (uint64_t)elapsed_us + s_timer2_remainder_us;
    uint32_t fast_ticks = (uint32_t)(fast_total_us / kTimer2PeriodUs);
    s_timer2_remainder_us = (uint32_t)(fast_total_us % kTimer2PeriodUs);
    while (fast_ticks--) {
        g_clock_fast++;

        // handleInternalTimerTick is PLATFORM_TBD-only on hardware (master
        // clock generation). Skipped here — desktop/wasm follow external
        // clock or run unsynchronised.

        MidiClock.div192th_countdown++;
        if (MidiClock.state == MidiClockClass::STARTED &&
            MidiClock.div192_time > 0 &&
            MidiClock.div192th_countdown >= MidiClock.div192_time &&
            MidiClock.interp_budget > 0) {
            MidiClock.increment192Counter();
            MidiClock.div192th_countdown = 0;
            MidiClock.interp_budget--;
            mcl_seq.seq();
        }
    }

    // timer1 work (1 kHz on hardware → one tick per 1000 µs).
    constexpr uint32_t kTimer1PeriodUs = 1000;
    uint64_t ms_total_us = (uint64_t)elapsed_us + s_timer1_remainder_us;
    uint32_t ms_ticks = (uint32_t)(ms_total_us / kTimer1PeriodUs);
    s_timer1_remainder_us = (uint32_t)(ms_total_us % kTimer1PeriodUs);
    while (ms_ticks--) {
        g_clock_ms++;
        g_clock_ticks++;
        if (g_clock_ticks == 60000) {
            g_clock_ticks = 0;
            g_clock_minutes++;
        }
        MidiUart.tickActiveSense();
        MidiUart2.tickActiveSense();
        MidiUartUSB.tickActiveSense();
    }

    // softirq2 — catch any bytes produced while the virtual timers ran.
    handleIncomingMidi();
    pump_host_midi_output_for_audio();
}

// GUI-rate entry. The first GUI tick enters the Arduino setup() body; later
// ticks run loop(). MCL::loop() itself polls input on wasm so nested modal
// loops receive button/encoder events too.
extern "C" void mcl_tick_gui(void) {
    if (!mcl_desktop_is_setup_done()) {
        mcl_desktop_tick();
        return;
    }
    mcl_desktop_tick();
}

// ---- Framebuffer ---------------------------------------------------------
//
// The Oled placeholder in oled.h holds an internal 128×64×1bpp buffer.
// On wasm we expose its offset; the host's wasm_runtime_addr_app_to_native()
// turns that into a real pointer it can read pixels from.

extern "C" uint32_t mcl_framebuffer_offset(void) {
    return (uint32_t)(uintptr_t)oled_display.getBuffer();
}
extern "C" uint32_t mcl_framebuffer_stride(void) { return OLED_WIDTH / 8; }
extern "C" uint32_t mcl_framebuffer_width (void) { return OLED_WIDTH;     }
extern "C" uint32_t mcl_framebuffer_height(void) { return OLED_HEIGHT;    }

// ---- MIDI bridge ---------------------------------------------------------

static MidiUartClass* uart_for_port(int32_t port) {
    switch (port) {
    case MCL_MIDI_UART:  return &MidiUart;
    case MCL_MIDI_UART2: return &MidiUart2;
    case MCL_MIDI_USB:   return (MidiUartClass*)&MidiUartUSB;
    default:             return nullptr;
    }
}

extern "C" int32_t mcl_midi_in_push(int32_t port, uint8_t byte_val) {
    auto* u = uart_for_port(port);
    if (!u) return 0;
    u->desktop_ingress(&byte_val, 1);
    return 1;
}

extern "C" int32_t mcl_midi_out_pop(int32_t port) {
    auto* u = uart_for_port(port);
    if (!u) return -1;
    uint8_t b;
    size_t n = u->desktop_egress(&b, 1);
    return n ? (int32_t)b : -1;
}

// ---- Input -------------------------------------------------------------
//
// Compatibility push-model exports. The SPS host uses the host_input_*
// imports instead; these remain useful for small harnesses that only call
// mcl_tick_gui() after setting input.

extern "C" void mcl_input_set_button_mask(uint32_t mask) {
    mcl_desktop_button_mask = (uint64_t)mask;
}

extern "C" void mcl_input_set_button_mask64(uint32_t mask_lo, uint32_t mask_hi) {
    mcl_desktop_button_mask = ((uint64_t)mask_hi << 32) | (uint64_t)mask_lo;
}

extern "C" void mcl_input_add_encoder_delta(int32_t idx, int8_t delta) {
    if (idx < 0 || idx >= GUI_NUM_ENCODERS) return;
    int new_val = (int)Encoders.encoders[idx].normal + (int)delta;
    if (new_val > 127)  new_val = 127;
    if (new_val < -128) new_val = -128;
    Encoders.encoders[idx].normal = (int8_t)new_val;
}

extern "C" void mcl_input_set_encoder_button(int32_t idx, uint8_t pressed) {
    if (idx < 0 || idx >= GUI_NUM_ENCODERS) return;
    Encoders.encoders[idx].button = pressed ? 1 : 0;
}

// ---- Version stamp -------------------------------------------------------

extern "C" uint32_t mcl_abi_version(void) {
    return ((uint32_t)MCL_ABI_MAJOR << 16) | MCL_ABI_MINOR;
}
