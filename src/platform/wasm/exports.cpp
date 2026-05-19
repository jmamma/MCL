// exports.cpp — wasm-side implementations of the mcl_* exports the host
// (SPS plugin via WAMR) calls. Wraps desktop_entry.cpp's mcl_desktop_*
// trampolines so the host doesn't need to know about the "desktop_" name
// (kept for source-shared compatibility with the static-link path).
//
// Input state (encoder/button) is push-model: host invokes
// mcl_input_set_*; this file writes directly into MCL's Encoders/Buttons
// globals. MCL reads from those without a wasm→host crossing.
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
static constexpr uint16_t MCL_ABI_MINOR = 0;

// ---- Lifecycle -----------------------------------------------------------

extern "C" void mcl_setup(void) { mcl_desktop_setup(); }

// Audio-thread entry. Catches up MCL's timer-ISR equivalents for the
// elapsed period, then dispatches the softirq-equivalent work
// (sequencer step firing + incoming MIDI handling). Replicates the
// rp2040/irqs.cpp:timer1_handler + timer2_handler + softirq1 + softirq2
// behaviour without an actual ISR.
extern "C" void mcl_tick_audio(uint32_t elapsed_us) {
    if (elapsed_us == 0) return;

    // timer2 work (5 kHz on hardware → one tick per 200 µs).
    constexpr uint32_t kTimer2PeriodUs = 200;
    uint32_t fast_ticks = elapsed_us / kTimer2PeriodUs;
    bool fired_step = false;
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
            fired_step = true;
        }
    }

    // timer1 work (1 kHz on hardware → one tick per 1000 µs).
    uint32_t ms_ticks = elapsed_us / 1000;
    while (ms_ticks--) {
        g_clock_ms++;
        g_clock_ticks++;
        if (g_clock_ticks == 60000) {
            g_clock_ticks = 0;
            g_clock_minutes++;
        }
    }

    // softirq1 — sequencer step firing, dispatched once if any div192
    // tick interpolated during catch-up. (Hardware fires softirq1 per
    // tick; calling once at end coalesces multiple step boundaries
    // into one sweep.)
    if (fired_step) {
        mcl_seq.seq();
    }

    // softirq2 — drain incoming MIDI byte streams through the per-port
    // parsers. Cheap; runs unconditionally so byte arrivals from this
    // audio block are visible to MCL's sequencer next tick.
    handleIncomingMidi();
}

// GUI-rate entry. Host audio thread calls this every Nth block (rate
// limit to ~60 Hz). Mirrors what MCL::loop() does on hardware: UI
// polling + page display + framebuffer rasterisation.
extern "C" void mcl_tick_gui(void) {
    GUI_hardware.poll();
    mcl.loop();
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
// Push-model: host writes; MCL reads via its normal accessors. Encoder
// `normal` is the rotational delta and is consumed by MCL's poll/event
// loop — we *add* deltas so multiple host pushes within a tick coalesce.
//
// Button mask: low bit per button index. Maps directly into
// ButtonsClass::buttons[i].status's B_BIT_CURRENT (active-low: bit set
// means released). So we invert: host's "pressed" bit → status bit
// cleared.

extern "C" void mcl_input_set_button_mask(uint32_t mask) {
    for (uint8_t i = 0; i < GUI_NUM_BUTTONS; ++i) {
        bool pressed = (mask >> i) & 1u;
        if (pressed) {
            Buttons.buttons[i].status &= (uint8_t)~(1u << B_BIT_CURRENT);
        } else {
            Buttons.buttons[i].status |= (uint8_t)(1u << B_BIT_CURRENT);
        }
    }
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
