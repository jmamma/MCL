/* Copyright 2026, Justin Mammarella jmamma@gmail.com */

#include "TrackLoadFadeRunner.h"
#include "MidiClock.h"
#include "Grid/TrackLoadFade.h"
#include <string.h>

using LoadFadeMask = uint32_t;

static_assert(NUM_SLOTS <= sizeof(LoadFadeMask) * 8,
              "TrackLoadFadeRunner active_mask is too narrow for NUM_SLOTS");

namespace {

constexpr uint8_t LOAD_FADE_RUNTIME_STARTED = 0x80;
#if MCL_FEATURE_HOST_LOAD_FADE_SEEK
constexpr uint8_t LOAD_FADE_RUNTIME_WAIT_START = 0x40;
#endif

class ATTR_PACKED() LoadFadeState {
public:
  TrackLoadFadeTarget target;
#if MCL_FEATURE_HOST_LOAD_FADE_SEEK
  uint16_t count_down;
  uint16_t elapsed_ticks;
  uint16_t duration_ticks;
#else
  uint16_t elapsed_q12;
  uint16_t duration_q12;
#endif
  uint8_t flags;
  uint8_t amount;
  uint8_t start_value;
  uint8_t end_value;
  uint8_t last_value;
#if MCL_FEATURE_HOST_LOAD_FADE_SEEK
  int8_t curve;
#endif

  void clear() {
    target.track_type = 0;
    target.device_idx = 0;
    target.track_number = 0;
    target.param = 0;
#if MCL_FEATURE_HOST_LOAD_FADE_SEEK
    count_down = 0;
    elapsed_ticks = 0;
    duration_ticks = 0;
#else
    elapsed_q12 = 0;
    duration_q12 = 0;
#endif
    flags = 0;
    amount = 0;
    start_value = 0;
    end_value = 0;
    last_value = 255;
#if MCL_FEATURE_HOST_LOAD_FADE_SEEK
    curve = 0;
#endif
  }
};

LoadFadeState fade_states[NUM_SLOTS];
LoadFadeMask active_mask = 0;

#if defined(PLATFORM_WASM) && defined(DEBUGMODE)
#define FADE_RUN_TRACE(fmt, ...) DEBUG_PRINT_FN("[fade-run] " fmt, ##__VA_ARGS__)
#else
#define FADE_RUN_TRACE(fmt, ...)
#endif

uint8_t clamp_7bit(int16_t value) {
  if (value < 0) {
    return 0;
  }
  if (value > 127) {
    return 127;
  }
  return (uint8_t)value;
}

#if MCL_FEATURE_HOST_LOAD_FADE_SEEK
uint8_t fade_curve_phase(uint8_t phase, int8_t curve) {
  uint16_t lin = phase > 127 ? 127 : phase;
  uint16_t expo = (uint16_t)((lin * lin + 63u) / 127u);
  uint16_t inv_base = (uint16_t)(127u - lin);
  uint16_t inv =
      (uint16_t)(127u - (inv_base * inv_base + 63u) / 127u);
  uint16_t mix = curve < 0 ? (uint16_t)(-curve) : (uint16_t)curve;
  if (mix > 127) {
    mix = 127;
  }
  uint16_t shaped = curve > 0 ? expo : (curve < 0 ? inv : lin);
  uint16_t out =
      (uint16_t)((lin * (127u - mix) + shaped * mix + 63u) / 127u);
  return out > 127 ? 127 : (uint8_t)out;
}

uint16_t clamp_u16(uint32_t value) {
  return value > 0xFFFFu ? 0xFFFFu : (uint16_t)value;
}

uint16_t load_fade_count_down(uint32_t start_clock) {
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

  return clamp_u16(to_start);
}

uint16_t load_fade_duration_ticks(uint16_t duration_q12) {
  uint32_t ticks_per_16th = MidiClock.div192th_ticks_per_16th();
  uint32_t ticks =
      ((uint32_t)duration_q12 * ticks_per_16th + 11u) / 12u;
  return clamp_u16(ticks == 0 ? 1u : ticks);
}

uint16_t load_fade_elapsed_ticks(uint16_t elapsed_q12) {
  if (elapsed_q12 == 0) {
    return 0;
  }
  uint32_t ticks_per_16th = MidiClock.div192th_ticks_per_16th();
  uint32_t ticks =
      ((uint32_t)elapsed_q12 * ticks_per_16th + 11u) / 12u;
  return clamp_u16(ticks);
}
#else
uint16_t load_fade_delay_q12(uint32_t start_clock) {
  uint32_t now = MidiClock.div192th_counter;
  uint32_t to_start = MidiClock.clock_diff_div192(now, start_clock);
  if (to_start == 0) {
    return 0;
  }

#if defined(__AVR__)
  if (to_start > (0x10000UL * 12u / 2u)) {
    return 0;
  }
  return to_start > 0xFFFFu ? 0xFFFFu : (uint16_t)to_start;
#else
  uint32_t ticks_per_16th = MidiClock.div192th_ticks_per_16th();
  uint32_t cycle = 0x10000UL * ticks_per_16th;
  if (to_start > (cycle / 2u)) {
    return 0;
  }

  uint32_t delay_q12 =
      (to_start * 12u + ticks_per_16th - 1u) / ticks_per_16th;
  return delay_q12 > 0xFFFFu ? 0xFFFFu : (uint16_t)delay_q12;
#endif
}
#endif

LoadFadeMask clear_slot(GridSlot slot) {
  if (slot >= NUM_SLOTS) {
    return 0;
  }
  LoadFadeMask bit = (LoadFadeMask)1 << slot;
  active_mask &= ~bit;
  memset(&fade_states[slot], 0, sizeof(fade_states[slot]));
  return bit;
}

bool begin_slot(GridSlot slot, LoadFadeState &state) {
  uint8_t current = 0;
  if (!read_track_load_fade_value(state.target, &current)) {
    FADE_RUN_TRACE("read fail slot=%u type=%u dev=%u track=%u param=%u",
                   slot, state.target.track_type, state.target.device_idx,
                   state.target.track_number, state.target.param);
    clear_slot(slot);
    return false;
  }

  uint8_t floor = current > state.amount ? current - state.amount : 0;
  if (state.flags & TRACK_LOAD_FADE_FLAG_OUT) {
    state.start_value = current;
    state.end_value = floor;
  } else {
    state.start_value = floor;
    state.end_value = current;
  }
  state.last_value = 255;
  state.flags |= LOAD_FADE_RUNTIME_STARTED;
#if MCL_FEATURE_HOST_LOAD_FADE_SEEK
  FADE_RUN_TRACE("runtime slot=%u type=%u dev=%u track=%u start=%u end=%u elapsed=%u durTicks=%u curve=%d",
                 slot, state.target.track_type, state.target.device_idx,
                 state.target.track_number, state.start_value,
                 state.end_value, state.elapsed_ticks, state.duration_ticks,
                 (int)state.curve);
#endif
  return true;
}

bool write_slot_value(GridSlot slot,
                      LoadFadeState &state,
                      MidiUartClass *uart,
                      MidiUartClass *uart2,
                      bool advance) {
#if MCL_FEATURE_HOST_LOAD_FADE_SEEK
  uint16_t elapsed = state.elapsed_ticks;
  bool complete = elapsed >= state.duration_ticks;
  uint8_t phase = complete
                      ? 127
                      : (uint8_t)(((uint32_t)elapsed * 127u) /
                                  state.duration_ticks);
  phase = fade_curve_phase(phase, state.curve);
  int16_t value =
      (int16_t)state.start_value +
      ((int16_t)state.end_value - (int16_t)state.start_value) * phase / 127;
  uint8_t value7 = complete ? state.end_value : clamp_7bit(value);
#else
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
#endif

  if (value7 != state.last_value) {
#if defined(PLATFORM_WASM) && defined(DEBUGMODE)
    const bool first_write = state.last_value == 255;
#endif
    write_track_load_fade_value(state.target, value7, uart, uart2);
#if defined(PLATFORM_WASM) && defined(DEBUGMODE)
    if (first_write || complete || (elapsed % 24u) == 0) {
      FADE_RUN_TRACE("write slot=%u track=%u value=%u elapsed=%u/%u complete=%u",
                     slot, state.target.track_number, value7, elapsed,
                     state.duration_ticks, complete ? 1 : 0);
    }
#endif
    state.last_value = value7;
  }

  if (complete) {
    clear_slot(slot);
    return false;
  }

  if (advance
#if MCL_FEATURE_HOST_LOAD_FADE_SEEK
      && state.elapsed_ticks < 0xFFFFu
#else
      && state.elapsed_q12 < 0xFFFFu
#endif
      ) {
#if MCL_FEATURE_HOST_LOAD_FADE_SEEK
    state.elapsed_ticks++;
#else
    state.elapsed_q12++;
#endif
  }
  return true;
}

void tick_slot(GridSlot slot, MidiUartClass *uart, MidiUartClass *uart2) {
  LoadFadeState &state = fade_states[slot];
  if ((state.flags & TRACK_LOAD_FADE_FLAG_ENABLED) == 0 ||
#if MCL_FEATURE_HOST_LOAD_FADE_SEEK
      state.duration_ticks == 0 || state.amount == 0) {
    FADE_RUN_TRACE("clear invalid slot=%u flags=%u durTicks=%u amount=%u",
                   slot, state.flags, state.duration_ticks, state.amount);
#else
      state.duration_q12 == 0 || state.amount == 0) {
#endif
    clear_slot(slot);
    return;
  }

#if MCL_FEATURE_HOST_LOAD_FADE_SEEK
  if ((state.flags & LOAD_FADE_RUNTIME_WAIT_START) != 0) {
    if (MidiClock.state != MidiClockClass::STARTED) {
      return;
    }
    state.flags &= (uint8_t)~LOAD_FADE_RUNTIME_WAIT_START;
    state.count_down = 0;
    FADE_RUN_TRACE("armed start slot=%u track=%u state=%u elapsed=%u",
                   slot, state.target.track_number, (unsigned)MidiClock.state,
                   state.elapsed_ticks);
  }
#endif

  if ((state.flags & LOAD_FADE_RUNTIME_STARTED) == 0) {
#if MCL_FEATURE_HOST_LOAD_FADE_SEEK
    if (state.count_down > 0) {
      state.count_down--;
      return;
    }
#else
    if (state.elapsed_q12 > 0) {
      state.elapsed_q12--;
      return;
    }
#endif

    if (!begin_slot(slot, state)) {
      return;
    }
  }

  write_slot_value(slot, state, uart, uart2, true);
}

} // namespace

#if MCL_FEATURE_HOST_LOAD_FADE_SEEK
void TrackLoadFadeRunner::clear(bool preserve_armed_prestart) {
#else
void TrackLoadFadeRunner::clear() {
#endif
#if MCL_FEATURE_HOST_LOAD_FADE_SEEK
  if (!preserve_armed_prestart) {
    active_mask = 0;
    for (uint8_t n = 0; n < NUM_SLOTS; n++) {
      fade_states[n].clear();
    }
    return;
  }

  for (uint8_t n = 0; n < NUM_SLOTS; n++) {
    LoadFadeMask bit = (LoadFadeMask)1 << n;
    if ((active_mask & bit) != 0 &&
        (fade_states[n].flags & LOAD_FADE_RUNTIME_WAIT_START) != 0) {
      continue;
    }
    active_mask &= ~bit;
    fade_states[n].clear();
  }
#else
  active_mask = 0;
  memset(fade_states, 0, sizeof(fade_states));
#endif
}

void TrackLoadFadeRunner::start(GridSlot slot,
                                const TrackLoadFadeTarget &target,
                                const TrackLoadFadeData *fade,
                                uint32_t start_clock
#if MCL_FEATURE_HOST_LOAD_FADE_SEEK
                                ,
                                bool allow_prestart,
                                MidiUartClass *uart,
                                MidiUartClass *uart2
#endif
                                ) {
  if (slot >= NUM_SLOTS) {
    return;
  }

  LoadFadeMask bit = clear_slot(slot);

#if MCL_FEATURE_HOST_LOAD_FADE_SEEK
  const bool transport_started = MidiClock.state == MidiClockClass::STARTED;
  if ((!transport_started && !allow_prestart) || fade == nullptr ||
      !fade->enabled()) {
    FADE_RUN_TRACE("start reject slot=%u state=%u prestart=%u fade=%p enabled=%u",
                   slot, (unsigned)MidiClock.state, allow_prestart ? 1 : 0, fade,
                   (fade != nullptr && fade->enabled()) ? 1 : 0);
    return;
  }
#else
  if (MidiClock.state != MidiClockClass::STARTED || fade == nullptr ||
      !fade->enabled()) {
    return;
  }
#endif

  LoadFadeState &state = fade_states[slot];
  state.target = target;
#if MCL_FEATURE_HOST_LOAD_FADE_SEEK
  state.count_down = transport_started ? load_fade_count_down(start_clock) : 0;
  state.duration_ticks = load_fade_duration_ticks(fade->duration_q12);
  state.elapsed_ticks = load_fade_elapsed_ticks(fade->elapsed_q12());
  if (state.elapsed_ticks > state.duration_ticks) {
    state.elapsed_ticks = state.duration_ticks;
  }
  state.flags = fade->flags & (TRACK_LOAD_FADE_FLAG_ENABLED |
                               TRACK_LOAD_FADE_FLAG_OUT);
  if (!transport_started) {
    state.flags |= LOAD_FADE_RUNTIME_WAIT_START;
  }
#else
  state.elapsed_q12 = load_fade_delay_q12(start_clock);
  state.duration_q12 = fade->duration_q12;
  state.flags = fade->flags & (TRACK_LOAD_FADE_FLAG_ENABLED |
                               TRACK_LOAD_FADE_FLAG_OUT);
#endif
  state.amount = fade->amount > 127 ? 127 : fade->amount;
#if MCL_FEATURE_HOST_LOAD_FADE_SEEK
  state.curve = fade->curve;
#endif
  active_mask |= bit;
#if MCL_FEATURE_HOST_LOAD_FADE_SEEK
  FADE_RUN_TRACE("start slot=%u type=%u dev=%u track=%u flags=%u countDown=%u elapsed=%u durTicks=%u amount=%u curve=%d prestart=%u",
                 slot, target.track_type, target.device_idx,
                 target.track_number, state.flags, state.count_down,
                 state.elapsed_ticks, state.duration_ticks, state.amount,
                 (int)state.curve,
                 transport_started ? 0 : 1);
  if (!transport_started && allow_prestart && state.count_down == 0) {
    if (begin_slot(slot, state)) {
      write_slot_value(slot, state, uart, uart2, false);
    }
  }
#endif
}

void TrackLoadFadeRunner::tick(MidiUartClass *uart, MidiUartClass *uart2) {
  if (active_mask == 0) {
    return;
  }
  if (MidiClock.state != MidiClockClass::STARTED) {
    FADE_RUN_TRACE("pause transport state=%u mask=%lu",
                   (unsigned)MidiClock.state, (unsigned long)active_mask);
#if MCL_FEATURE_HOST_LOAD_FADE_SEEK
    clear(true);
#else
    clear();
#endif
    return;
  }
  LoadFadeMask bit = 1;
  for (uint8_t slot = 0; slot < NUM_SLOTS; slot++, bit <<= 1) {
    if ((active_mask & bit) != 0) {
      tick_slot(slot, uart, uart2);
    }
  }
}
