#pragma once

#include "SeqDefines.h"
#include "platform.h"

#include <stdint.h>

extern uint8_t get_random_byte();

#define SEQ_CONDITION_INLINE static inline __attribute__((always_inline))

enum {
  SEQ_CONDITIONAL_FIRST_RUN = 1 << 0,
  SEQ_CONDITIONAL_PREV_TRIG = 1 << 1,
};

SEQ_CONDITION_INLINE void seq_condition_reset(uint8_t *iterations,
                                              uint8_t &flags) {
  for (uint8_t i = 0; i < 7; i++) {
    iterations[i] = 1;
  }
  flags = SEQ_CONDITIONAL_FIRST_RUN;
}

SEQ_CONDITION_INLINE void seq_condition_advance_cycle(uint8_t *iterations,
                                                      uint8_t &flags) {
  flags &= (uint8_t)~SEQ_CONDITIONAL_FIRST_RUN;

  for (uint8_t i = 0; i < 7; i++) {
    uint8_t max = i + 2;
    uint8_t value = iterations[i] + 1;
    if (value > max) {
      value = 1;
    }
    iterations[i] = value;
  }
}

SEQ_CONDITION_INLINE uint8_t seq_condition_get_iteration(
    const uint8_t *iterations, uint8_t y) {
  if (y < 2 || y > 8) {
    return 1;
  }
  return iterations[y - 2];
}

SEQ_CONDITION_INLINE bool seq_condition_first_run(uint8_t flags) {
  return (flags & SEQ_CONDITIONAL_FIRST_RUN) != 0;
}

SEQ_CONDITION_INLINE void seq_condition_set_first_run(uint8_t &flags,
                                                      bool first_run) {
  if (first_run) {
    flags |= SEQ_CONDITIONAL_FIRST_RUN;
  } else {
    flags &= (uint8_t)~SEQ_CONDITIONAL_FIRST_RUN;
  }
}

SEQ_CONDITION_INLINE bool seq_condition_prev_trig(uint8_t flags) {
  return (flags & SEQ_CONDITIONAL_PREV_TRIG) != 0;
}

SEQ_CONDITION_INLINE void seq_condition_set_prev_trig(uint8_t &flags,
                                                      bool fired) {
  if (fired) {
    flags |= SEQ_CONDITIONAL_PREV_TRIG;
  } else {
    flags &= (uint8_t)~SEQ_CONDITIONAL_PREV_TRIG;
  }
}

SEQ_CONDITION_INLINE void seq_condition_record_neighbor(uint16_t &mask,
                                                        uint8_t track_number,
                                                        bool fired) {
  if (track_number >= 16) {
    return;
  }
  if (fired) {
    mask |= (uint16_t)(1u << track_number);
  } else {
    mask &= (uint16_t)~(1u << track_number);
  }
}

SEQ_CONDITION_INLINE bool seq_condition_neighbor_fired(uint16_t mask,
                                                       uint8_t track_number) {
  return track_number != 0 &&
         (mask & (uint16_t)(1u << (track_number - 1))) != 0;
}

SEQ_CONDITION_INLINE bool seq_condition_match(uint8_t condition,
                                              const uint8_t *iterations,
                                              uint8_t flags,
                                              uint8_t track_number,
                                              uint16_t fill_mask,
                                              uint16_t neighbor_mask) {
  if (condition <= SEQ_COND_90PCT) {
    if (condition == SEQ_COND_100PCT) {
      return true;
    }
    static const uint8_t thresholds[] PROGMEM = {
        255, 26, 64, 84, 128, 169, 192, 230,
    };
    return get_random_byte() <= pgm_read_byte_near(thresholds + condition);
  }

  if (condition >= SEQ_COND_FIRST && condition <= SEQ_COND_NOT_NEI) {
    bool value = false;
    if (condition < SEQ_COND_FILL) {
      value = seq_condition_first_run(flags);
    } else if (condition < SEQ_COND_PRE) {
      value = track_number < 16 &&
              (fill_mask & ((uint16_t)1 << track_number)) != 0;
    } else if (condition < SEQ_COND_NEI) {
      value = seq_condition_prev_trig(flags);
    } else {
      value = seq_condition_neighbor_fired(neighbor_mask, track_number);
    }
    return (condition & 1) ? value : !value;
  }

  if (condition >= SEQ_COND_ITER_BASE && condition <= SEQ_COND_ITER_MAX) {
    uint8_t x, y;
    if (seq_cond_iter_decode(condition, x, y)) {
      return iterations[y - 2] == x;
    }
  }

  return true;
}

#undef SEQ_CONDITION_INLINE
