#ifndef SPSX_SEQ_DEFINES_H__
#define SPSX_SEQ_DEFINES_H__

#if !defined(__AVR__)

#include <cstdint>

// ============================================================================
// SPSX Sequencer Interpolation
// ============================================================================
// 16x MIDI clock interpolation — matches host resolution
#define SPSX_SEQ_INTERPOLATION 16

// Ticks per step at 1X speed (6 * interpolation)
#define SPSX_TICKS_PER_STEP_1X (6 * SPSX_SEQ_INTERPOLATION)

// ============================================================================
// SPSX Lock Params
// ============================================================================
// 34 lock params per track (24 legacy + 4 envelope + 3 LFO-B + 3 retrig)
#define SPSX_NUM_LOCKS 34

// ============================================================================
// Speed Multipliers (same enum values as MCL, different tick counts)
// ============================================================================
#define SPSX_SPEED_1X   0   // SPSX_TICKS_PER_STEP_1X ticks/step (normal)
#define SPSX_SPEED_2X   1   // /2 (double speed)
#define SPSX_SPEED_3_4X 2   // *4/3 (3/4 speed)
#define SPSX_SPEED_3_2X 3   // *2/3 (3/2 speed)
#define SPSX_SPEED_1_2X 4   // *2 (half speed)
#define SPSX_SPEED_1_4X 5   // *4 (quarter speed)
#define SPSX_SPEED_1_8X 6   // *8 (eighth speed)
#define SPSX_SPEED_4X   7   // /4 (quadruple speed)

// ============================================================================
// Mask types
// ============================================================================
#define SPSX_MASK_PATTERN 0
#define SPSX_MASK_LOCK 1
#define SPSX_MASK_SLIDE 2
#define SPSX_MASK_MUTE 3
#define SPSX_MASK_LOCKS_ON_STEP 4

// ============================================================================
// Mute state
// ============================================================================
#define SPSX_MUTE_ON 1
#define SPSX_MUTE_OFF 0

// ============================================================================
// Trig conditional results
// ============================================================================
#define SPSX_TRIG_FALSE 0
#define SPSX_TRIG_TRUE 1
#define SPSX_TRIG_ONESHOT 3

// ============================================================================
// Conditional Types (6-bit field = 0-63)
// ============================================================================

// Percentage-based (0-7)
#define SPSX_COND_100PCT     0
#define SPSX_COND_90PCT      1
#define SPSX_COND_75PCT      2
#define SPSX_COND_66PCT      3
#define SPSX_COND_50PCT      4
#define SPSX_COND_33PCT      5
#define SPSX_COND_25PCT      6
#define SPSX_COND_10PCT      7

// Special conditions (8-16)
#define SPSX_COND_ONESHOT    8
#define SPSX_COND_FIRST      9
#define SPSX_COND_NOT_FIRST  10
#define SPSX_COND_FILL       11
#define SPSX_COND_NOT_FILL   12
#define SPSX_COND_PRE        13
#define SPSX_COND_NOT_PRE    14
#define SPSX_COND_NEI        15
#define SPSX_COND_NOT_NEI    16

// Iteration ratios X:Y (17-51)
#define SPSX_COND_ITER_BASE  17
#define SPSX_COND_ITER_MAX   51

// Number of conditional types
#define SPSX_NUM_TRIG_CONDITIONS 52

// Retrig infinite sentinel
#define SPSX_RETRIG_INFINITE 255

// ============================================================================
// Steps and lock slots
// ============================================================================
#define SPSX_NUM_MD_STEPS 64
#define SPSX_NUM_MD_LOCK_SLOTS 256

// Default swing mask: every second step (off-beats)
#define SPSX_DEFAULT_SWING_MASK 0xAAAAAAAAAAAAAAAAULL

// ============================================================================
// Editing directions
// ============================================================================
#define SPSX_DIR_LEFT 0
#define SPSX_DIR_RIGHT 1
#define SPSX_DIR_REVERSE 2

// ============================================================================
// RTIM lookup table: ticks per retrig at 96 PPQN
// ============================================================================
#define SPSX_RTIM_COUNT 15
static const uint16_t SPSX_RTIM_TICKS_96PPQN[SPSX_RTIM_COUNT] = {
    1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192
};

// ============================================================================
// Bit manipulation helpers (64-bit)
// ============================================================================
#define SPSX_SET_BIT64(mask, bit) ((mask) |= (1ULL << (bit)))
#define SPSX_CLEAR_BIT64(mask, bit) ((mask) &= ~(1ULL << (bit)))
#define SPSX_IS_BIT_SET64(mask, bit) (((mask) >> (bit)) & 1ULL)

#define SPSX_SET_BIT16(mask, bit) ((mask) |= (1U << (bit)))
#define SPSX_CLEAR_BIT16(mask, bit) ((mask) &= ~(1U << (bit)))
#define SPSX_IS_BIT_SET16(mask, bit) (((mask) >> (bit)) & 1U)

// Rotate macros
#define SPSX_ROTATE_LEFT(mask, len) do { \
    uint64_t msb = ((mask) >> ((len) - 1)) & 1ULL; \
    (mask) = (((mask) << 1) | msb) & ((1ULL << (len)) - 1); \
} while(0)

#define SPSX_ROTATE_RIGHT(mask, len) do { \
    uint64_t lsb = (mask) & 1ULL; \
    (mask) = (((mask) >> 1) | (lsb << ((len) - 1))) & ((1ULL << (len)) - 1); \
} while(0)

// ============================================================================
// Utility functions
// ============================================================================

// Reverse bits 0..len-1 of a 64-bit mask
inline uint64_t spsx_reverse_mask64(uint64_t mask, uint8_t len) {
    uint64_t result = 0;
    for (uint8_t i = 0; i < len; i++) {
        if (mask & (1ULL << i)) {
            result |= (1ULL << (len - 1 - i));
        }
    }
    return result;
}

// Popcount 64-bit
inline uint8_t spsx_popcount(uint64_t x) {
    x = x - ((x >> 1) & 0x5555555555555555ULL);
    x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
    x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
    return (uint8_t)((x * 0x0101010101010101ULL) >> 56);
}

// Convert microtiming (-128 to +127) to tick offset (quarter step range)
inline int16_t spsx_microtiming_to_ticks(int8_t mt, uint16_t tps) {
    return (int16_t)((int16_t)mt * (tps / 4) / 128);
}

// Encode X:Y ratio to condition ID
inline uint8_t spsx_cond_iter_encode(uint8_t x, uint8_t y) {
    if (y < 2 || y > 8 || x < 1 || x > y) return SPSX_COND_100PCT;
    uint8_t offset = (uint8_t)((y - 2) * (y - 1) / 2 + (y - 2));
    return (uint8_t)(SPSX_COND_ITER_BASE + offset + (x - 1));
}

// Decode condition ID to X:Y ratio
inline bool spsx_cond_iter_decode(uint8_t cond, uint8_t &x, uint8_t &y) {
    if (cond < SPSX_COND_ITER_BASE || cond > SPSX_COND_ITER_MAX) return false;
    uint8_t offset = cond - SPSX_COND_ITER_BASE;
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

// Use the platform's entropy-mixed random byte
inline uint8_t spsx_get_random_byte() {
    return get_random_byte();
}

#endif // !defined(__AVR__)
#endif // SPSX_SEQ_DEFINES_H__
