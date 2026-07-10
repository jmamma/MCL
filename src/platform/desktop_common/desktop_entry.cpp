// desktop_entry.cpp — minimal entry points exercised by the native oracle.
#include "desktop_entry.h"

#include "Arduino.h"
// std::atomic requires libc++. wasm32 runs single-threaded — plain bools
// are enough there.
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
bool g_setup_requested = false;
bool g_setup_done = false;
#else
std::atomic<bool> g_setup_done{false};
#endif

void desktop_advance_clocks() {
    const unsigned long now_ms = millis();
    g_clock_ms   = (uint16_t)(now_ms);
#if !defined(PLATFORM_WASM)
    g_clock_fast = (uint16_t)(now_ms);
#endif
}
} // namespace

void mcl_desktop_setup(void) {
#if defined(PLATFORM_WASM)
    g_setup_requested = true;
    desktop_advance_clocks();
    return;
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
    if (!g_setup_requested) return;
    desktop_advance_clocks();
#ifdef MCL_DESKTOP_LINK_MCL_CORE
    if (!g_setup_done) {
        setup();
        g_setup_done = true;
        return;
    }
    loop();
#endif
    return;
#else
    if (!g_setup_done.load(std::memory_order_acquire)) return;
    desktop_advance_clocks();
#ifdef MCL_DESKTOP_LINK_MCL_CORE
    loop();
#endif
#endif
}

bool mcl_desktop_is_setup_done(void) {
#if defined(PLATFORM_WASM)
    return g_setup_done;
#else
    return g_setup_done.load(std::memory_order_acquire);
#endif
}
