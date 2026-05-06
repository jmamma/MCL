#ifndef STEP_SEQ_TRACK_DATA_H__
#define STEP_SEQ_TRACK_DATA_H__

#if !defined(__AVR__)

#include "StepSeqDefines.h"
#include <cstddef>
#include <cstdint>
#include <cstring>

// ============================================================================
// StepSeqStepDescriptor - Step metadata
// ============================================================================

#pragma pack(push, 1)
class StepSeqStepDescriptor {
public:
    uint64_t locks;

    bool locks_enabled : 1;
    bool cond_plock : 1;
    uint8_t cond_id : 6;

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
// StepSeqStep - Full step for copy/paste operations
// ============================================================================

class StepSeqStep {
public:
    bool active;
    uint8_t locks[STEPSEQ_NUM_LOCKS];
    int8_t microtiming;
    StepSeqStepDescriptor data;
};

// ============================================================================
// StepSeqTrackData - Track-level step storage
// ============================================================================

#pragma pack(push, 1)
class StepSeqTrackData {
public:
    uint8_t locks[STEPSEQ_NUM_LOCK_SLOTS];
    uint8_t locks_params[STEPSEQ_NUM_LOCKS];
    int8_t microtiming[STEPSEQ_NUM_STEPS];
    StepSeqStepDescriptor steps[STEPSEQ_NUM_STEPS];

    uint64_t trig_mask;
    uint64_t slide_mask;
    uint64_t accent_mask;
    uint64_t swing_mask;

    uint8_t track_length;
    uint8_t track_speed;

    const uint8_t* data() const { return reinterpret_cast<const uint8_t*>(this); }
    uint8_t* data() { return reinterpret_cast<uint8_t*>(this); }
    static constexpr size_t dataSize() { return sizeof(StepSeqTrackData); }

    void init() {
        memset(this, 0, sizeof(StepSeqTrackData));
        swing_mask = STEPSEQ_DEFAULT_SWING_MASK;
        track_speed = 0xFF;
    }

    void clean_params() {
        uint64_t used_locks = 0;
        for (uint8_t x = 0; x < STEPSEQ_NUM_STEPS; x++) {
            used_locks |= steps[x].locks;
        }
        uint64_t mask = 1ULL;
        for (uint8_t a = 0; a < STEPSEQ_NUM_LOCKS; a++) {
            if (!(used_locks & mask)) {
                locks_params[a] = 0;
            }
            mask <<= 1;
        }
    }

    uint8_t find_param(uint8_t param_id) const {
        param_id += 1;
        if (!param_id) return 255;
        for (uint8_t c = 0; c < STEPSEQ_NUM_LOCKS; c++) {
            if (locks_params[c] == param_id) {
                return c;
            }
        }
        return 255;
    }

    uint16_t get_lockidx(uint8_t step) const {
        uint16_t idx = 0;
        for (uint8_t i = 0; i < step; ++i) {
            idx += stepseq_popcount(steps[i].locks);
        }
        return idx;
    }

    uint16_t get_lockidx(uint8_t step, uint8_t lock_idx) const {
        uint64_t mask = 1ULL << lock_idx;
        uint64_t rmask = mask - 1;
        if (steps[step].is_lock_bit(lock_idx)) {
            auto idx = (uint16_t)(get_lockidx(step) +
                                  stepseq_popcount(steps[step].locks & rmask));
            return idx;
        } else {
            return STEPSEQ_NUM_LOCK_SLOTS;
        }
    }

    bool hasData() const {
        return trig_mask != 0;
    }
};
#pragma pack(pop)

#endif // !defined(__AVR__)
#endif // STEP_SEQ_TRACK_DATA_H__
