#pragma once

#include <stdint.h>

#define MASK_PATTERN 0
#define MASK_MUTE 1
#define MASK_SWING 2
#define MASK_SLIDE 3
#define MASK_LOCK 4
#define MASK_LOCKS_ON_STEP 5

#define SEQ_SPEED_1X 0
#define SEQ_SPEED_2X 1
#define SEQ_SPEED_3_4X 2
#define SEQ_SPEED_3_2X 3
#define SEQ_SPEED_1_2X 4
#define SEQ_SPEED_1_4X 5
#define SEQ_SPEED_1_8X 6
#define SEQ_SPEED_4X 7

// Canonical conditional ids. These match the step-edit UI order and fit in the
// 6-bit condition fields used by the newer sequencers.
#define SEQ_COND_100PCT     0
#define SEQ_COND_10PCT      1
#define SEQ_COND_25PCT      2
#define SEQ_COND_33PCT      3
#define SEQ_COND_50PCT      4
#define SEQ_COND_66PCT      5
#define SEQ_COND_75PCT      6
#define SEQ_COND_90PCT      7

#define SEQ_COND_ONESHOT    8
#define SEQ_COND_FIRST      9
#define SEQ_COND_NOT_FIRST  10
#define SEQ_COND_FILL       11
#define SEQ_COND_NOT_FILL   12
#define SEQ_COND_PRE        13
#define SEQ_COND_NOT_PRE    14
#define SEQ_COND_NEI        15
#define SEQ_COND_NOT_NEI    16

#define SEQ_COND_ITER_BASE  17
#define SEQ_COND_ITER_MAX   51
#define SEQ_COND_MAX        SEQ_COND_ITER_MAX
#define SEQ_NUM_TRIG_CONDITIONS 52

#define SEQ_LEGACY_COND_MAX 14

inline uint8_t seq_cond_knob_to_step(uint8_t knob, uint8_t max_condition,
                                     bool *plock) {
  if (knob == max_condition) {
    *plock = false;
    return SEQ_COND_100PCT;
  }
  if (knob > max_condition) {
    *plock = false;
    return (uint8_t)(knob - max_condition);
  }
  *plock = true;
  return (uint8_t)(max_condition - knob);
}

inline uint8_t seq_cond_step_to_knob(uint8_t condition, bool plock,
                                     uint8_t max_condition) {
  if (condition == SEQ_COND_100PCT) {
    return max_condition;
  }
  return plock ? (uint8_t)(max_condition - condition)
               : (uint8_t)(max_condition + condition);
}

inline uint8_t seq_cond_iter_encode(uint8_t x, uint8_t y) {
  if (y < 2 || y > 8 || x < 1 || x > y) return SEQ_COND_100PCT;
  uint8_t offset = 0;
  for (uint8_t i = 2; i < y; i++) offset += i;
  return (uint8_t)(SEQ_COND_ITER_BASE + offset + (x - 1));
}

bool seq_cond_iter_decode(uint8_t cond, uint8_t &x, uint8_t &y);

inline uint8_t seq_legacy_cond_to_stepseq(uint8_t condition) {
  switch (condition) {
  case 0:
  case 1:
    return SEQ_COND_100PCT;
  case 2:
    return 18; // 2:2
  case 3:
    return 21; // 3:3
  case 4:
    return 25; // 4:4
  case 5:
    return 30; // 5:5
  case 6:
    return 36; // 6:6
  case 7:
    return 43; // 7:7
  case 8:
    return SEQ_COND_ITER_MAX; // 8:8
  case 9:
    return SEQ_COND_10PCT;
  case 10:
    return SEQ_COND_25PCT;
  case 11:
    return SEQ_COND_50PCT;
  case 12:
    return SEQ_COND_75PCT;
  case 13:
    return SEQ_COND_90PCT;
  case 14:
    return SEQ_COND_ONESHOT;
  default:
    return SEQ_COND_100PCT;
  }
}
