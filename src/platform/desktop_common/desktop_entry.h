// desktop_entry.h — C-linkage interface the SPS JUCE plugin uses to drive
// MCL. These are the only symbols the plugin should call directly; everything
// else stays inside the mcl_desktop static lib.
#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Run MCL's Arduino-style setup() once. Idempotent; calling twice is a no-op.
void mcl_desktop_setup(void);

// Run one iteration of MCL's Arduino-style loop(). Drive from a juce::Timer
// at the desired refresh rate (e.g. 60 Hz) or from PluginProcessor::processBlock.
void mcl_desktop_tick(void);

// Configure the directory MCL's SdFat shim resolves "/foo" paths against.
// Defaults to "./mcl_sd" relative to the host process working directory.
// Pass an absolute path from JUCE (e.g. userApplicationDataDirectory/mcl_sd).
void mcl_set_sd_root(const char* path);

// Inject raw MIDI bytes into MCL's primary UART ingress ring. Called from
// PluginProcessor::processBlock against juce::MidiBuffer events.
void mcl_inject_midi(const uint8_t* data, size_t len);

// Drain queued MIDI bytes from MCL's primary UART egress ring. Returns the
// number of bytes copied into `dst` (up to `cap`). Empty buffer returns 0.
size_t mcl_drain_midi_out(uint8_t* dst, size_t cap);

// Pointer to MCL's monochrome framebuffer. Layout is Adafruit_GFX's internal
// format (row-major, vertically-packed bytes). 128 × oled-height pixels.
// May return null before mcl_desktop_setup() runs. Returns uint32 sizes
// to share a return-type signature with the wasm-ABI mcl_framebuffer_*
// exports (see ../wasm/wasm_exports.h).
const uint8_t* mcl_framebuffer(void);
uint32_t       mcl_framebuffer_width(void);
uint32_t       mcl_framebuffer_height(void);

#ifdef __cplusplus
}
#endif
