// desktop_entry.h — C-linkage interface a host uses to drive MCL. These are
// the only symbols the host should call directly; everything else stays inside
// the mcl_desktop static lib.
#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Prepare MCL's Arduino-style runtime. On wasm this is intentionally fast:
// the actual setup() body is entered from mcl_desktop_tick(), in the same
// cooperative driver path as loop().
void mcl_desktop_setup(void);

// Run one iteration of MCL's Arduino-style loop(). Drive from a juce::Timer
// at the desired refresh rate (e.g. 60 Hz) or from PluginProcessor::processBlock.
void mcl_desktop_tick(void);

// True after the Arduino setup() body has completed.
bool mcl_desktop_is_setup_done(void);

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
