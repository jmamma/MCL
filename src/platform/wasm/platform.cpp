// platform.cpp — wasm implementations for time + GPIO mock state.
//
// Same surface as src/platform/desktop/platform.cpp, but `millis`/`micros`
// route through host imports instead of std::chrono. The wasm runtime
// can't read host wall-clock without the host's help.
#include "platform.h"
#include "host_imports.h"
#include "GUI_hardware.h"

extern uint64_t mcl_desktop_button_mask;

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

void mcl_platform_yield() {
    host_yield();
}

uint64_t mcl_platform_button_mask() {
    return host_input_button_mask() | mcl_desktop_button_mask;
}

int mcl_platform_encoder_delta(uint8_t encoder_id) {
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

// SW_IRQ storage — referenced via extern in platform.h. Values are
// irrelevant; the TRIGGER_SW_IRQ*/CLEAR_SW_IRQ* macros are no-ops.
uint8_t SW_IRQ1 = 0;
uint8_t SW_IRQ2 = 0;
uint8_t SW_IRQ3 = 0;
