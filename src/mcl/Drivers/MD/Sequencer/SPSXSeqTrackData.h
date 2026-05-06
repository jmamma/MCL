#ifndef SPSX_SEQ_TRACK_DATA_H__
#define SPSX_SEQ_TRACK_DATA_H__

#if !defined(__AVR__)

#include "SPSXSeqDefines.h"
#include <cstdint>
#include <cstring>

// ============================================================================
// SPSXSeqStepDescriptor - Step metadata
// 64-bit lock bitfield (supports up to 34 lock params) + flags
// ============================================================================

#pragma pack(push, 1)
class SPSXSeqStepDescriptor {
public:
    uint64_t locks;         // Bitfield of locks in current step (LSB = lock 0)

    bool locks_enabled : 1; // true: locks active, false: disabled but preserved
    bool cond_plock : 1;    // Conditional applies to plocks only
    uint8_t cond_id : 6;    // Conditional type (0-63)

    uint64_t get_locks() const { return locks; }
    void set_locks(uint64_t v) { locks = v; }

    bool is_lock_bit(uint8_t idx) const {
        return (locks & (1ULL << idx)) != 0;
    }

    bool is_lock(uint8_t idx) const {
        return is_lock_bit(idx) && locks_enabled;
    }
};
#pragma pack(pop)

// ============================================================================
// SPSXSeqStep - Full step for copy/paste operations
// ============================================================================

class SPSXSeqStep {
public:
    bool active;
    uint8_t locks[SPSX_NUM_LOCKS];
    int8_t microtiming;  // Microtiming offset (-127 to +127, 0 = on grid)
    SPSXSeqStepDescriptor data;
};

// ============================================================================
// SPSXSeqTrackData - Track-level step storage
// ============================================================================

#pragma pack(push, 1)
class SPSXSeqTrackData {
public:
    uint8_t locks[SPSX_NUM_MD_LOCK_SLOTS];     // 256 lock value slots
    uint8_t locks_params[SPSX_NUM_LOCKS];       // Which param each lock maps to (1-based, 0=unused)
    int8_t microtiming[SPSX_NUM_MD_STEPS];      // Microtiming per step (-127 to +127, 0 = on grid)
    SPSXSeqStepDescriptor steps[SPSX_NUM_MD_STEPS]; // 64 step descriptors

    // Step masks (64-bit, one bit per step)
    uint64_t trig_mask;
    uint64_t slide_mask;
    uint64_t accent_mask;
    uint64_t swing_mask;

    // Per-track timing
    uint8_t track_length;   // 0 = use pattern length
    uint8_t track_speed;    // 0xFF = use pattern speed

    const uint8_t* data() const { return reinterpret_cast<const uint8_t*>(this); }
    uint8_t* data() { return reinterpret_cast<uint8_t*>(this); }
    static constexpr size_t dataSize() { return sizeof(SPSXSeqTrackData); }

    void init() {
        memset(this, 0, sizeof(SPSXSeqTrackData));
        swing_mask = SPSX_DEFAULT_SWING_MASK;
        track_speed = 0xFF;
    }

    void clean_params() {
        uint64_t _locks = 0;
        for (uint8_t x = 0; x < SPSX_NUM_MD_STEPS; x++) {
            _locks |= steps[x].locks;
        }
        uint64_t mask = 1ULL;
        for (uint8_t a = 0; a < SPSX_NUM_LOCKS; a++) {
            if (!(_locks & mask)) {
                locks_params[a] = 0;
            }
            mask <<= 1;
        }
    }

    uint8_t find_param(uint8_t param_id) const {
        param_id += 1;
        if (!param_id) return 255;
        for (uint8_t c = 0; c < SPSX_NUM_LOCKS; c++) {
            if (locks_params[c] == param_id) {
                return c;
            }
        }
        return 255;
    }

    uint16_t get_lockidx(uint8_t step) const {
        uint16_t idx = 0;
        for (uint8_t i = 0; i < step; ++i) {
            idx += spsx_popcount(steps[i].locks);
        }
        return idx;
    }

    uint16_t get_lockidx(uint8_t step, uint8_t lock_idx) const {
        uint64_t mask = 1ULL << lock_idx;
        uint64_t rmask = mask - 1;
        if (steps[step].is_lock_bit(lock_idx)) {
            auto idx = (uint16_t)(get_lockidx(step) + spsx_popcount(steps[step].locks & rmask));
            return idx;
        } else {
            return SPSX_NUM_MD_LOCK_SLOTS;
        }
    }

    bool hasData() const {
        return trig_mask != 0;
    }
};
#pragma pack(pop)

#endif // !defined(__AVR__)
#endif // SPSX_SEQ_TRACK_DATA_H__
