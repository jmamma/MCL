#ifndef STEP_SEQ_DEFINES_H__
#define STEP_SEQ_DEFINES_H__

#if !defined(__AVR__)

#include <cstdint>
#include "platform.h"

extern uint8_t get_random_byte();

// ============================================================================
// Step Sequencer Interpolation
// ============================================================================
// 16x MIDI clock interpolation: 96 ticks per 16th note.
#define STEPSEQ_SEQ_INTERPOLATION 16
#define STEPSEQ_LEGACY_SEQ_INTERPOLATION 2

// Ticks per step at 1X speed (6 * interpolation).
#define STEPSEQ_TICKS_PER_STEP_1X (6 * STEPSEQ_SEQ_INTERPOLATION)

// ============================================================================
// Lock Params
// ============================================================================
// 34 lock params per track (24 legacy + 4 envelope + 3 LFO-B + 3 retrig).
#define STEPSEQ_NUM_LOCKS 34

// ============================================================================
// Speed Multipliers (same enum values as MCL, different tick counts)
// ============================================================================
#define STEPSEQ_SPEED_1X   0
#define STEPSEQ_SPEED_2X   1
#define STEPSEQ_SPEED_3_4X 2
#define STEPSEQ_SPEED_3_2X 3
#define STEPSEQ_SPEED_1_2X 4
#define STEPSEQ_SPEED_1_4X 5
#define STEPSEQ_SPEED_1_8X 6
#define STEPSEQ_SPEED_4X   7

// ============================================================================
// Mask types
// ============================================================================
#define STEPSEQ_MASK_PATTERN 0
#define STEPSEQ_MASK_LOCK 1
#define STEPSEQ_MASK_SLIDE 2
#define STEPSEQ_MASK_MUTE 3
#define STEPSEQ_MASK_LOCKS_ON_STEP 4

// ============================================================================
// Mute state
// ============================================================================
#define STEPSEQ_MUTE_ON 1
#define STEPSEQ_MUTE_OFF 0

// ============================================================================
// Trig conditional results
// ============================================================================
#define STEPSEQ_TRIG_FALSE 0
#define STEPSEQ_TRIG_TRUE 1
#define STEPSEQ_TRIG_ONESHOT 3

// ============================================================================
// Conditional Types (6-bit field = 0-63)
// ============================================================================
#define STEPSEQ_COND_100PCT     0
#define STEPSEQ_COND_90PCT      1
#define STEPSEQ_COND_75PCT      2
#define STEPSEQ_COND_66PCT      3
#define STEPSEQ_COND_50PCT      4
#define STEPSEQ_COND_33PCT      5
#define STEPSEQ_COND_25PCT      6
#define STEPSEQ_COND_10PCT      7

#define STEPSEQ_COND_ONESHOT    8
#define STEPSEQ_COND_FIRST      9
#define STEPSEQ_COND_NOT_FIRST  10
#define STEPSEQ_COND_FILL       11
#define STEPSEQ_COND_NOT_FILL   12
#define STEPSEQ_COND_PRE        13
#define STEPSEQ_COND_NOT_PRE    14
#define STEPSEQ_COND_NEI        15
#define STEPSEQ_COND_NOT_NEI    16

#define STEPSEQ_COND_ITER_BASE  17
#define STEPSEQ_COND_ITER_MAX   51

#define STEPSEQ_NUM_TRIG_CONDITIONS 52

// Retrig infinite sentinel.
#define STEPSEQ_RETRIG_INFINITE 255

// ============================================================================
// Steps and lock slots
// ============================================================================
#define STEPSEQ_NUM_STEPS 64
#define STEPSEQ_NUM_LOCK_SLOTS 256

// Default swing mask: every second step (off-beats).
#define STEPSEQ_DEFAULT_SWING_MASK 0xAAAAAAAAAAAAAAAAULL

// ============================================================================
// Editing directions
// ============================================================================
#define STEPSEQ_DIR_LEFT 0
#define STEPSEQ_DIR_RIGHT 1
#define STEPSEQ_DIR_REVERSE 2

// ============================================================================
// RTIM lookup table: ticks per retrig at 96 PPQN
// ============================================================================
#define STEPSEQ_RTIM_COUNT 15
static const uint16_t STEPSEQ_RTIM_TICKS_96PPQN[STEPSEQ_RTIM_COUNT] = {
    1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192
};

// ============================================================================
// Bit manipulation helpers
// ============================================================================
#define STEPSEQ_SET_BIT64(mask, bit) ((mask) |= (1ULL << (bit)))
#define STEPSEQ_CLEAR_BIT64(mask, bit) ((mask) &= ~(1ULL << (bit)))
#define STEPSEQ_IS_BIT_SET64(mask, bit) (((mask) >> (bit)) & 1ULL)

#define STEPSEQ_SET_BIT16(mask, bit) ((mask) |= (1U << (bit)))
#define STEPSEQ_CLEAR_BIT16(mask, bit) ((mask) &= ~(1U << (bit)))
#define STEPSEQ_IS_BIT_SET16(mask, bit) (((mask) >> (bit)) & 1U)

#define STEPSEQ_ROTATE_LEFT(mask, len) do { \
    uint64_t msb = ((mask) >> ((len) - 1)) & 1ULL; \
    (mask) = (((mask) << 1) | msb) & ((1ULL << (len)) - 1); \
} while (0)

#define STEPSEQ_ROTATE_RIGHT(mask, len) do { \
    uint64_t lsb = (mask) & 1ULL; \
    (mask) = (((mask) >> 1) | (lsb << ((len) - 1))) & ((1ULL << (len)) - 1); \
} while (0)

// ============================================================================
// Utility functions
// ============================================================================
inline uint64_t stepseq_reverse_mask64(uint64_t mask, uint8_t len) {
    uint64_t result = 0;
    for (uint8_t i = 0; i < len; i++) {
        if (mask & (1ULL << i)) {
            result |= (1ULL << (len - 1 - i));
        }
    }
    return result;
}

inline uint8_t stepseq_popcount(uint64_t x) {
    x = x - ((x >> 1) & 0x5555555555555555ULL);
    x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
    x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
    return (uint8_t)((x * 0x0101010101010101ULL) >> 56);
}

inline int16_t stepseq_microtiming_to_ticks(int8_t mt, uint16_t tps) {
    return (int16_t)((int16_t)mt * (tps / 4) / 128);
}

inline uint8_t stepseq_cond_iter_encode(uint8_t x, uint8_t y) {
    if (y < 2 || y > 8 || x < 1 || x > y) return STEPSEQ_COND_100PCT;
    uint8_t offset = (uint8_t)((y - 2) * (y - 1) / 2 + (y - 2));
    return (uint8_t)(STEPSEQ_COND_ITER_BASE + offset + (x - 1));
}

inline bool stepseq_cond_iter_decode(uint8_t cond, uint8_t &x, uint8_t &y) {
    if (cond < STEPSEQ_COND_ITER_BASE || cond > STEPSEQ_COND_ITER_MAX) return false;
    uint8_t offset = cond - STEPSEQ_COND_ITER_BASE;
    static const uint8_t y_starts[] = {0, 2, 5, 9, 14, 20, 27, 35};
    for (uint8_t i = 0; i < 7; i++) {
        if (offset < y_starts[i + 1]) {
            y = i + 2;
            x = (uint8_t)(offset - y_starts[i] + 1);
            return true;
        }
    }
    return false;
}

inline uint8_t stepseq_get_random_byte() {
    return get_random_byte();
}

#endif // !defined(__AVR__)
#endif // STEP_SEQ_DEFINES_H__
