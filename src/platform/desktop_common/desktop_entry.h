// Minimal entry points used only by the native compile/link oracle. Production
// hosts use the generated wasm ABI and must not link this interface.
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

#ifdef __cplusplus
}
#endif
