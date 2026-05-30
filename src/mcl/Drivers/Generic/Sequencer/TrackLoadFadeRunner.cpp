/* Copyright 2026, Justin Mammarella jmamma@gmail.com */

#include "TrackLoadFadeRunner.h"
#include "MidiClock.h"
#include "../../../MCL/TrackLoadFade.h"
#include <string.h>

static_assert(GRID_WIDTH <= 32,
              "TrackLoadFadeRunner active_mask is uint32_t; widen it if "
              "GRID_WIDTH grows past 32");

namespace {

constexpr uint8_t LOAD_FADE_RUNTIME_STARTED = 0x80;

class ATTR_PACKED() LoadFadeState {
public:
  TrackLoadFadeTarget target;
  uint16_t elapsed_q12;
  uint16_t duration_q12;
  uint8_t flags;
  uint8_t amount;
  uint8_t start_value;
  uint8_t end_value;
  uint8_t last_value;

  void clear() {
    target.track_type = 0;
    target.device_idx = 0;
    target.track_number = 0;
    target.param = 0;
    elapsed_q12 = 0;
    duration_q12 = 0;
    flags = 0;
    amount = 0;
    start_value = 0;
    end_value = 0;
    last_value = 255;
  }
};

LoadFadeState fade_states[GRID_WIDTH];
uint32_t active_mask = 0;

uint8_t clamp_7bit(int16_t value) {
  if (value < 0) {
    return 0;
  }
  if (value > 127) {
    return 127;
  }
  return (uint8_t)value;
}

uint16_t load_fade_delay_q12(uint32_t start_clock) {
  uint32_t now = MidiClock.div192th_counter;
  uint32_t to_start = MidiClock.clock_diff_div192(now, start_clock);
  if (to_start == 0) {
    return 0;
  }

  uint32_t ticks_per_16th = MidiClock.div192th_ticks_per_16th();
  uint32_t cycle = 0x10000UL * ticks_per_16th;
  if (to_start > (cycle / 2u)) {
    return 0;
  }

  uint32_t delay_q12 = (to_start * 12u + ticks_per_16th - 1u) /
                       ticks_per_16th;
  return delay_q12 > 0xFFFFu ? 0xFFFFu : (uint16_t)delay_q12;
}

void clear_slot(GridSlot slot) {
  if (slot >= GRID_WIDTH) {
    return;
  }
  uint32_t bit = (uint32_t)1 << slot;
  active_mask &= ~bit;
  fade_states[slot].clear();
}

void tick_slot(GridSlot slot, MidiUartClass *uart, MidiUartClass *uart2) {
  if (slot >= GRID_WIDTH) {
    return;
  }

  uint32_t bit = (uint32_t)1 << slot;
  if ((active_mask & bit) == 0) {
    return;
  }

  LoadFadeState &state = fade_states[slot];
  if ((state.flags & TRACK_LOAD_FADE_FLAG_ENABLED) == 0 ||
      state.duration_q12 == 0 || state.amount == 0) {
    clear_slot(slot);
    return;
  }

  if ((state.flags & LOAD_FADE_RUNTIME_STARTED) == 0) {
    if (state.elapsed_q12 > 0) {
      state.elapsed_q12--;
      return;
    }

    uint8_t current = 0;
    if (!read_track_load_fade_value(state.target, &current)) {
      clear_slot(slot);
      return;
    }
    if (state.flags & TRACK_LOAD_FADE_FLAG_OUT) {
      state.start_value = current;
      state.end_value = current > state.amount ? current - state.amount : 0;
    } else {
      state.start_value = current > state.amount ? current - state.amount : 0;
      state.end_value = current;
    }
    state.elapsed_q12 = 0;
    state.last_value = 255;
    state.flags |= LOAD_FADE_RUNTIME_STARTED;
  }

  uint16_t elapsed = state.elapsed_q12;
  bool complete = elapsed >= state.duration_q12;
  uint8_t phase = complete
                      ? 127
                      : (uint8_t)(((uint32_t)elapsed * 127u) /
                                  state.duration_q12);
  int16_t value =
      (int16_t)state.start_value +
      ((int16_t)state.end_value - (int16_t)state.start_value) * phase / 127;
  uint8_t value7 = complete ? state.end_value : clamp_7bit(value);

  if (value7 != state.last_value) {
    write_track_load_fade_value(state.target, value7, uart, uart2);
    state.last_value = value7;
  }

  if (complete) {
    clear_slot(slot);
  } else if (state.elapsed_q12 < 0xFFFFu) {
    state.elapsed_q12++;
  }
}

} // namespace

void TrackLoadFadeRunner::clear() {
  active_mask = 0;
  for (uint8_t n = 0; n < GRID_WIDTH; n++) {
    fade_states[n].clear();
  }
}

void TrackLoadFadeRunner::start(GridSlot slot,
                                const TrackLoadFadeTarget &target,
                                const TrackLoadFadeData *fade,
                                uint32_t start_clock) {
  if (slot >= GRID_WIDTH) {
    return;
  }

  clear_slot(slot);

  if (MidiClock.state != MidiClockClass::STARTED || fade == nullptr ||
      !fade->enabled()) {
    return;
  }

  LoadFadeState &state = fade_states[slot];
  state.target = target;
  state.elapsed_q12 = load_fade_delay_q12(start_clock);
  state.duration_q12 = fade->duration_q12;
  state.flags = fade->flags & (TRACK_LOAD_FADE_FLAG_ENABLED |
                               TRACK_LOAD_FADE_FLAG_OUT);
  state.amount = fade->amount > 127 ? 127 : fade->amount;
  active_mask |= (uint32_t)1 << slot;
}

void TrackLoadFadeRunner::tick(MidiUartClass *uart, MidiUartClass *uart2) {
  if (active_mask == 0) {
    return;
  }
  if (MidiClock.state != MidiClockClass::STARTED) {
    clear();
    return;
  }
  for (uint8_t slot = 0; slot < GRID_WIDTH; slot++) {
    tick_slot(slot, uart, uart2);
  }
}
