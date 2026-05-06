#ifndef SPSX_SEQ_DEFINES_H__
#define SPSX_SEQ_DEFINES_H__

#if !defined(__AVR__)

#include "StepSeqDefines.h"

// SPSX compatibility names. The implementation now lives in the generic
// StepSeq layer so TBD can share the same storage/timing constants.
#define SPSX_SEQ_INTERPOLATION STEPSEQ_SEQ_INTERPOLATION
#define LEGACY_SEQ_INTERPOLATION STEPSEQ_LEGACY_SEQ_INTERPOLATION
#define SPSX_TICKS_PER_STEP_1X STEPSEQ_TICKS_PER_STEP_1X

#define SPSX_NUM_LOCKS STEPSEQ_NUM_LOCKS

#define SPSX_SPEED_1X STEPSEQ_SPEED_1X
#define SPSX_SPEED_2X STEPSEQ_SPEED_2X
#define SPSX_SPEED_3_4X STEPSEQ_SPEED_3_4X
#define SPSX_SPEED_3_2X STEPSEQ_SPEED_3_2X
#define SPSX_SPEED_1_2X STEPSEQ_SPEED_1_2X
#define SPSX_SPEED_1_4X STEPSEQ_SPEED_1_4X
#define SPSX_SPEED_1_8X STEPSEQ_SPEED_1_8X
#define SPSX_SPEED_4X STEPSEQ_SPEED_4X

#define SPSX_MASK_PATTERN STEPSEQ_MASK_PATTERN
#define SPSX_MASK_LOCK STEPSEQ_MASK_LOCK
#define SPSX_MASK_SLIDE STEPSEQ_MASK_SLIDE
#define SPSX_MASK_MUTE STEPSEQ_MASK_MUTE
#define SPSX_MASK_LOCKS_ON_STEP STEPSEQ_MASK_LOCKS_ON_STEP

#define SPSX_MUTE_ON STEPSEQ_MUTE_ON
#define SPSX_MUTE_OFF STEPSEQ_MUTE_OFF

#define SPSX_TRIG_FALSE STEPSEQ_TRIG_FALSE
#define SPSX_TRIG_TRUE STEPSEQ_TRIG_TRUE
#define SPSX_TRIG_ONESHOT STEPSEQ_TRIG_ONESHOT

#define SPSX_COND_100PCT STEPSEQ_COND_100PCT
#define SPSX_COND_90PCT STEPSEQ_COND_90PCT
#define SPSX_COND_75PCT STEPSEQ_COND_75PCT
#define SPSX_COND_66PCT STEPSEQ_COND_66PCT
#define SPSX_COND_50PCT STEPSEQ_COND_50PCT
#define SPSX_COND_33PCT STEPSEQ_COND_33PCT
#define SPSX_COND_25PCT STEPSEQ_COND_25PCT
#define SPSX_COND_10PCT STEPSEQ_COND_10PCT

#define SPSX_COND_ONESHOT STEPSEQ_COND_ONESHOT
#define SPSX_COND_FIRST STEPSEQ_COND_FIRST
#define SPSX_COND_NOT_FIRST STEPSEQ_COND_NOT_FIRST
#define SPSX_COND_FILL STEPSEQ_COND_FILL
#define SPSX_COND_NOT_FILL STEPSEQ_COND_NOT_FILL
#define SPSX_COND_PRE STEPSEQ_COND_PRE
#define SPSX_COND_NOT_PRE STEPSEQ_COND_NOT_PRE
#define SPSX_COND_NEI STEPSEQ_COND_NEI
#define SPSX_COND_NOT_NEI STEPSEQ_COND_NOT_NEI

#define SPSX_COND_ITER_BASE STEPSEQ_COND_ITER_BASE
#define SPSX_COND_ITER_MAX STEPSEQ_COND_ITER_MAX
#define SPSX_NUM_TRIG_CONDITIONS STEPSEQ_NUM_TRIG_CONDITIONS

#define SPSX_RETRIG_INFINITE STEPSEQ_RETRIG_INFINITE

#define SPSX_NUM_MD_STEPS STEPSEQ_NUM_STEPS
#define SPSX_NUM_MD_LOCK_SLOTS STEPSEQ_NUM_LOCK_SLOTS

#define SPSX_DEFAULT_SWING_MASK STEPSEQ_DEFAULT_SWING_MASK

#define SPSX_DIR_LEFT STEPSEQ_DIR_LEFT
#define SPSX_DIR_RIGHT STEPSEQ_DIR_RIGHT
#define SPSX_DIR_REVERSE STEPSEQ_DIR_REVERSE

#define SPSX_RTIM_COUNT STEPSEQ_RTIM_COUNT
#define SPSX_RTIM_TICKS_96PPQN STEPSEQ_RTIM_TICKS_96PPQN

#define SPSX_SET_BIT64(mask, bit) STEPSEQ_SET_BIT64(mask, bit)
#define SPSX_CLEAR_BIT64(mask, bit) STEPSEQ_CLEAR_BIT64(mask, bit)
#define SPSX_IS_BIT_SET64(mask, bit) STEPSEQ_IS_BIT_SET64(mask, bit)

#define SPSX_SET_BIT16(mask, bit) STEPSEQ_SET_BIT16(mask, bit)
#define SPSX_CLEAR_BIT16(mask, bit) STEPSEQ_CLEAR_BIT16(mask, bit)
#define SPSX_IS_BIT_SET16(mask, bit) STEPSEQ_IS_BIT_SET16(mask, bit)

#define SPSX_ROTATE_LEFT(mask, len) STEPSEQ_ROTATE_LEFT(mask, len)
#define SPSX_ROTATE_RIGHT(mask, len) STEPSEQ_ROTATE_RIGHT(mask, len)

inline uint64_t spsx_reverse_mask64(uint64_t mask, uint8_t len) {
    return stepseq_reverse_mask64(mask, len);
}

inline uint8_t spsx_popcount(uint64_t x) {
    return stepseq_popcount(x);
}

inline int16_t spsx_microtiming_to_ticks(int8_t mt, uint16_t tps) {
    return stepseq_microtiming_to_ticks(mt, tps);
}

inline uint8_t spsx_cond_iter_encode(uint8_t x, uint8_t y) {
    return stepseq_cond_iter_encode(x, y);
}

inline bool spsx_cond_iter_decode(uint8_t cond, uint8_t &x, uint8_t &y) {
    return stepseq_cond_iter_decode(cond, x, y);
}

inline uint8_t spsx_get_random_byte() {
    return stepseq_get_random_byte();
}

#endif // !defined(__AVR__)
#endif // SPSX_SEQ_DEFINES_H__
