// platform.cpp — desktop implementations for time + GPIO mock state.
#include "platform.h"
#include "hardware.h"
#include <chrono>

namespace {
// Capture epoch at first call so millis() returns sensible small values
// from the host process's perspective.
auto& epoch() {
    static auto t0 = std::chrono::steady_clock::now();
    return t0;
}
} // namespace

unsigned long millis() {
    auto now = std::chrono::steady_clock::now();
    return static_cast<unsigned long>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now - epoch()).count());
}

unsigned long micros() {
    auto now = std::chrono::steady_clock::now();
    return static_cast<unsigned long>(
        std::chrono::duration_cast<std::chrono::microseconds>(now - epoch()).count());
}

// delay() / delayMicroseconds() are no-ops. MCL's setup() has a delay(2000)
// at the top — on desktop we just continue. Audio-thread blocking is the
// reason these stay empty.
void delay(unsigned long /*ms*/) {}
void delayMicroseconds(unsigned int /*us*/) {}

void platform_poll() {
#ifdef DEBUGMODE
    mcl_debug::flush();
#endif
}

void platform_wait_poll() {
#ifdef DEBUGMODE
    mcl_debug::flush();
#endif
}

extern uint64_t mcl_desktop_button_mask;

uint64_t mcl_platform_button_mask() {
    return mcl_desktop_button_mask;
}

int mcl_platform_encoder_delta(uint8_t /*encoder_id*/) {
    return 0;
}

uint32_t mcl_platform_encoder_button_mask() {
    return 0;
}

// SW_IRQ definitions to satisfy the externs in platform.h. Their values
// are irrelevant — TRIGGER_SW_IRQ*/CLEAR_SW_IRQ* macros are no-ops.
uint8_t SW_IRQ1 = 0;
uint8_t SW_IRQ2 = 0;
uint8_t SW_IRQ3 = 0;
