// wasm_exports.h — wasm-side exports the JUCE host calls into.
//
// These are the symbols a host looks up via wasm_runtime_lookup_function()
// on the mcl.aot module. The wasm side must export them with C linkage and
// matching signatures, otherwise the host can't drive MCL.
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

// Fast prepare step. Host calls once before any mcl_tick_*. The Arduino
// setup() body runs from the first mcl_tick_gui() call.
void mcl_setup(void);

// Audio-side entry. `elapsed_us` is derived from the host audio sample
// clock, not wall clock.
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

// GUI-rate entry. The first call runs Arduino setup(); later calls run
// encoder/key polling, page display(), and framebuffer rasterisation. Must
// not run concurrently with mcl_tick_audio — single-threaded wasm, no locks.
//
// May allocate / touch the SD shim / be slow. Do not call from the audio
// callback.
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

// Set the module transport position from the SPS host clock domain. tick96 uses
// 24 MIDI clocks * 16 interpolation ticks per quarter note.
void mcl_set_transport_position(uint32_t tick96);

// Compatibility encoder/button state-injection for simple harnesses. The
// full host uses host_input_* imports so blocking modal loops can receive
// input while mcl_tick_gui() is already executing.
void mcl_input_set_button_mask(uint32_t mask);
void mcl_input_set_button_mask64(uint32_t mask_lo, uint32_t mask_hi);
void mcl_input_add_encoder_delta(int32_t idx, int8_t delta);
void mcl_input_set_encoder_button(int32_t idx, uint8_t pressed);
void mcl_input_set_key_state(int32_t key, uint8_t pressed);

// Identifier the host can read to confirm the .aot it loaded matches
// this header's expectations. Bump on incompatible ABI changes.
//
// Layout: low 16 = minor (additions), high 16 = major (breaks).
uint32_t mcl_abi_version(void);

// Optional diagnostic export. Returns MCL's currently measured MIDI clock
// tempo as BPM * 100 for host-side SPS-vs-MCL clock comparisons.
uint32_t mcl_debug_tempo_x100(void);

// Optional diagnostic export for SPS integration tests. Returns 0 until MCL
// setup has completed, then exposes read-only state packed so the host can
// wait for readiness without changing MCL behavior:
//   bits  0..7   current PageIndex, or 255 if invalid
//   bits  8..15  current grid row
//   bit   16     current grid half
//   bit   17     GridPage slot menu visible
//   bit   18     key interface active
//   bit   19     note interface accepting notes
//   bit   20     MD.connected
//   bit   21     primary device connected
//   bit   22     secondary device connected
//   bit   23     setup complete
//   bit   24     note_interface has notes_on bits
//   bit   25     note_interface has notes_off bits
//   bit   26     GridIOPage::track_select nonzero
//   bit   27     GridIOPage::show_offset
//   bit   28     GridIOPage::show_track_type
uint32_t mcl_debug_state(void);

// Optional diagnostic values for SPS integration tests:
//   1 GridIOPage::track_select
//   2 note_interface.notes_on
//   3 note_interface.notes_off
//   4 mcl_debug_state()
//   5 mcl_cfg.track_type_select
//   200 MCL config menu display text snapshot byte length
//   201..264 menu display text snapshot bytes, packed little-endian 4 chars/value
//   265 top-level system menu display text snapshot byte length
//   266 top-level system menu display text snapshot after detaching devices
//   300 MidiClock.div16th_counter
//   301 MidiClock.div96th_counter
//   302 MidiClock.div192th_counter
//   303 packed clock state/mod6/mod12/interpolation
uint32_t mcl_debug_value(int32_t id);

#ifdef __cplusplus
}
#endif
