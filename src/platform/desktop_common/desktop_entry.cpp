// desktop_entry.cpp — C-linkage entry points exposed to the SPS plugin.
//
// Step 1 of the integration deliberately keeps these as stubs: MCL's setup()
// and loop() haven't been compiled in yet. Once src/mcl is glob-compiled into
// the static lib (step 2) we'll route through to the Arduino-style functions
// that MCL/src/mcl/main.cpp defines.
#include "desktop_entry.h"

#include "SdFat.h"
#include "Arduino.h"

#include <string.h>
// std::atomic requires libc++. wasm32 runs single-threaded — a plain bool
// is enough. Desktop is also fine: setup is called once on the message
// thread, tick from a single timer.
#if !defined(PLATFORM_WASM)
#include <atomic>
#endif

// MCL's logical clocks. On hardware these are bumped from timer ISRs;
// on desktop nothing increments them unless we do it ourselves. The
// `read_clock_ms()` macro in helpers.h:222 reads g_clock_ms directly,
// so MidiClock, sysex timeouts, scheduler timing etc. all stall if it
// never advances. Update them lazily at every tick boundary from
// millis().
extern volatile uint16_t g_clock_ms;
extern volatile uint16_t g_clock_fast;

// Forward-declared so this TU doesn't depend on Arduino.h. MCL itself defines
// setup()/loop() in src/mcl/main.cpp.
extern void setup();
extern void loop();

namespace {
#if defined(PLATFORM_WASM)
bool g_setup_done = false;
#else
std::atomic<bool> g_setup_done{false};
#endif

void desktop_advance_clocks() {
    const unsigned long now_ms = millis();
    g_clock_ms   = (uint16_t)(now_ms);
    g_clock_fast = (uint16_t)(now_ms);
}
} // namespace

void mcl_desktop_setup(void) {
#if defined(PLATFORM_WASM)
    if (g_setup_done) return;
    g_setup_done = true;
#else
    bool expected = false;
    if (!g_setup_done.compare_exchange_strong(expected, true)) {
        return;
    }
#endif
    desktop_advance_clocks();
#ifdef MCL_DESKTOP_LINK_MCL_CORE
    setup();
#endif
}

void mcl_desktop_tick(void) {
#if defined(PLATFORM_WASM)
    if (!g_setup_done) return;
#else
    if (!g_setup_done.load(std::memory_order_acquire)) return;
#endif
    desktop_advance_clocks();
#ifdef MCL_DESKTOP_LINK_MCL_CORE
    loop();
#endif
}

void mcl_set_sd_root(const char* path) {
    if (path && *path) {
        mcl_desktop_set_sd_root(path);
    }
}

void mcl_inject_midi(const uint8_t* /*data*/, size_t /*len*/) {
    // TODO(step 5): push into MidiUart ingress ring.
}

size_t mcl_drain_midi_out(uint8_t* /*dst*/, size_t /*cap*/) {
    // TODO(step 5): drain MidiUart egress ring.
    return 0;
}

const uint8_t* mcl_framebuffer(void) {
    // TODO(step 5+): return oled_display.getBuffer() once oled is wired up.
    return nullptr;
}

uint16_t mcl_framebuffer_width(void)  { return 128; }
uint16_t mcl_framebuffer_height(void) { return 64;  }
