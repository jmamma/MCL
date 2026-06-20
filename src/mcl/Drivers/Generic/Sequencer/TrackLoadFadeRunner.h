/* Copyright 2026, Justin Mammarella jmamma@gmail.com */

#pragma once

#include "../../../MCL/MCLMemory.h"
#include "../../../MCL/MCLPlatformFeatures.h"
#include "TrackLoadFadeTarget.h"
#include "platform.h"
#include <inttypes.h>

class MidiUartClass;
class TrackLoadFadeData;

// Generic per-load fade engine. Owns timing/curve/runtime only — destination
// dispatch (read of the current parameter value, write of new values) is
// delegated to the TrackLoadFadeTarget bridge so the runner stays free of
// device coupling.
class TrackLoadFadeRunner {
public:
  // Clears every active fade. Call on transport reset/stop.
#if MCL_FEATURE_HOST_LOAD_FADE_SEEK
  static void clear(bool preserve_armed_prestart = false);
#else
  static void clear();
#endif

  // Always clears any state previously held for `slot`. If `fade` is null,
  // transport isn't STARTED, or the fade is disabled, returns after clearing
  // so no new fade is scheduled. Otherwise schedules a new fade using
  // `target` as the destination.
  static void start(GridSlot slot,
                    const TrackLoadFadeTarget &target,
                    const TrackLoadFadeData *fade,
                    uint32_t start_clock
#if MCL_FEATURE_HOST_LOAD_FADE_SEEK
                    ,
                    bool allow_prestart = false,
                    MidiUartClass *uart = nullptr,
                    MidiUartClass *uart2 = nullptr
#endif
                    );

  // Advances every active fade by one tick. Output is written through the
  // TrackLoadFadeTarget bridge.
  static void tick(MidiUartClass *uart, MidiUartClass *uart2);
};
