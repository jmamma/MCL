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

// Canonical conditional ids. These match the StepSeq/SPSX encoding and fit in
// the 6-bit condition fields used by the newer sequencers.
#define SEQ_COND_100PCT     0
#define SEQ_COND_90PCT      1
#define SEQ_COND_75PCT      2
#define SEQ_COND_66PCT      3
#define SEQ_COND_50PCT      4
#define SEQ_COND_33PCT      5
#define SEQ_COND_25PCT      6
#define SEQ_COND_10PCT      7

#define SEQ_COND_ONESHOT    8
#define SEQ_COND_FIRST      9
#define SEQ_COND_NOT_FIRST  10
#define SEQ_COND_FILL       11
#define SEQ_COND_NOT_FILL   12

#define SEQ_COND_ITER_BASE  13
#define SEQ_COND_ITER_MAX   47
#define SEQ_COND_MAX        SEQ_COND_ITER_MAX
#define SEQ_NUM_TRIG_CONDITIONS 48

#define SEQ_LEGACY_COND_MAX 14

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
    return 14; // 2:2
  case 3:
    return 17; // 3:3
  case 4:
    return 21; // 4:4
  case 5:
    return 26; // 5:5
  case 6:
    return 32; // 6:6
  case 7:
    return 39; // 7:7
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
