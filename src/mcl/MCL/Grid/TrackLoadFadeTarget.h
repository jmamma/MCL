/* Copyright 2026, Justin Mammarella jmamma@gmail.com */

#pragma once

#include "MCLMemory.h"
#include <inttypes.h>

class GridDeviceTrack;
class MidiUartClass;
class TrackLoadFadeData;

// Per-slot fade destination resolved at fade start. Stored inside the runner
// so the generic tick path can dispatch back through the bridge without
// re-resolving the slot's GridDeviceTrack each tick.
struct TrackLoadFadeTarget {
  uint8_t track_type;
  uint8_t device_idx;
  uint8_t track_number;
  uint8_t param;
};

// Returns true and populates *target only if the slot/gdt/fade triple
// designates a fade the bridge can drive. Currently supports MD_TRACK_TYPE
// and MDSPSX_TRACK_TYPE with TARGET_DEFAULT or MODEL_LEVEL.
bool resolve_track_load_fade_target(GridSlot slot,
                                    GridDeviceTrack *gdt,
                                    const TrackLoadFadeData *fade,
                                    TrackLoadFadeTarget *target);

// Returns true and writes the current parameter value to *value. The runner
// uses this to sample the fade's start/end anchor when the fade actually
// begins (i.e. after the start-clock delay elapses).
bool read_track_load_fade_value(const TrackLoadFadeTarget &target,
                                uint8_t *value);

// Sends `value` to the resolved device parameter. Picks uart vs. uart2 based
// on the resolved device_idx.
void write_track_load_fade_value(const TrackLoadFadeTarget &target,
                                 uint8_t value,
                                 MidiUartClass *uart,
                                 MidiUartClass *uart2);
