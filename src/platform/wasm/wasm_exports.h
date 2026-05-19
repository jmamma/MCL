// wasm_exports.h — wasm-side exports the JUCE host calls into.
//
// These are the symbols a host (the SPS plugin) looks up via
// wasm_runtime_lookup_function() on the mcl.aot module. The wasm side
// must export them with C linkage and matching signatures, otherwise the
// host can't drive MCL.
//
// Kept in sync with:
//   - host_imports.h          (what wasm calls back into the host)
//   - tools/build_mcl_wasm.sh (which exports get marked dynamic)
#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Run MCL's Arduino-style setup() once. Idempotent. Host must call this
// before mcl_tick(). Runs on the host's UI/timer thread; never on the
// audio thread.
void mcl_setup(void);

// Run one iteration of MCL's Arduino-style loop(). Drive at ~60 Hz from
// a juce::Timer on the host's UI thread. Single-threaded — never call
// concurrently with mcl_setup() or another mcl_tick().
void mcl_tick(void);

// Linear-memory offset of MCL's framebuffer (128 × oled_height × 1bpp,
// Adafruit_GFX layout). Host translates the offset into a wasm-linear-
// memory pointer with wasm_runtime_addr_app_to_native() and reads pixels
// directly. Pointer is stable for the lifetime of the wasm instance.
uint32_t mcl_framebuffer_offset(void);
uint32_t mcl_framebuffer_stride(void);   // bytes per row
uint32_t mcl_framebuffer_width(void);
uint32_t mcl_framebuffer_height(void);

// Push one MIDI byte into MCL's named UART ingress ring. port is the
// same tag the host uses for host_midi_*. Returns 1 if accepted, 0 on
// overflow.
int32_t mcl_midi_in_push(int32_t port, uint8_t byte_val);

// Pop one MIDI byte from MCL's egress ring for `port`. Returns -1 if
// the ring is empty, 0..255 otherwise.
int32_t mcl_midi_out_pop(int32_t port);

// Encoder/button state-injection. Host sets values; MCL's next tick
// reads them through host_encoder_delta()/host_encoder_button()/
// host_button_mask() (which forward to these accumulators).
void mcl_input_set_button_mask(uint32_t mask);
void mcl_input_add_encoder_delta(int32_t idx, int8_t delta);
void mcl_input_set_encoder_button(int32_t idx, uint8_t pressed);

// Identifier the host can read to confirm the .aot it loaded matches
// this header's expectations. Bump on incompatible ABI changes.
//
// Layout: low 16 = minor (additions), high 16 = major (breaks).
uint32_t mcl_abi_version(void);

#ifdef __cplusplus
}
#endif
