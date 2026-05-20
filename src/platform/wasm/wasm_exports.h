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

// Run MCL's Arduino-style setup() once. Idempotent. Host calls before
// any mcl_tick_*. May run on either the audio or the message thread.
void mcl_setup(void);

// Audio-thread entry. Called once per audio block from
// PluginProcessor::processBlock. `elapsed_us` is derived from the host
// audio sample clock, not wall clock.
//
// Replaces what timer1 (1 kHz) and timer2 (5 kHz) ISRs do on hardware:
//   - Advances g_clock_ms/g_clock_fast by the elapsed time.
//   - Fires MidiClock.handleInternalTimerTick / increment192Counter
//     for each 200us slice that elapsed (gated by interp_budget).
//   - Calls mcl_seq.seq() whenever a div192 tick interpolates (softirq1).
//   - Drains incoming MIDI via handleIncomingMidi() (softirq2).
//
// Real-time-safe. No file I/O, no allocations beyond what MCL's
// sequencer already does.
void mcl_tick_audio(uint32_t elapsed_us);

// GUI-rate entry. Runs encoder/key polling, page display(), framebuffer
// rasterisation. Must not run concurrently with mcl_tick_audio —
// single-threaded wasm, no locks.
//
// May allocate / touch the SD shim / be slow. The audio thread budget
// must accommodate the worst frame, otherwise occasional dropouts.
// Typical MCL display update is well under 1 ms on a desktop CPU.
void mcl_tick_gui(void);

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
void mcl_input_set_button_mask64(uint32_t mask_lo, uint32_t mask_hi);
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
