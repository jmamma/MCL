// platform.cpp — wasm implementations for time + GPIO mock state.
//
// Same surface as src/platform/desktop/platform.cpp, but `millis`/`micros`
// route through host imports instead of std::chrono. The wasm runtime
// can't read host wall-clock without the host's help.
#include "platform.h"
#include "host_imports.h"

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

// SW_IRQ storage — referenced via extern in platform.h. Values are
// irrelevant; the TRIGGER_SW_IRQ*/CLEAR_SW_IRQ* macros are no-ops.
uint8_t SW_IRQ1 = 0;
uint8_t SW_IRQ2 = 0;
uint8_t SW_IRQ3 = 0;
