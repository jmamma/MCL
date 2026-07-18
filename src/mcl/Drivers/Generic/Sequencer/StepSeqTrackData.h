#ifndef STEP_SEQ_TRACK_DATA_H__
#define STEP_SEQ_TRACK_DATA_H__

#if !defined(__AVR__)

#include "StepSeqDefines.h"
#include "Sequencer/SeqTrackModData.h"
#include <cstddef>
#include <cstdint>
#include <cstring>

// ============================================================================
// Step metadata
// ============================================================================

#pragma pack(push, 1)
class StepSeqStepDescriptor {
public:
    uint64_t locks;

    uint8_t reserved : 1;
    bool cond_plock : 1;
    uint8_t cond_id : 6;

    uint64_t get_locks() const { return locks; }
    void set_locks(uint64_t value) { locks = value; }
    bool is_lock_bit(uint8_t index) const {
        return (locks & (uint64_t{1} << index)) != 0;
    }
    bool is_lock(uint8_t index) const { return is_lock_bit(index); }
};
#pragma pack(pop)

// Transient copy/paste data is deliberately large enough for the hosted
// SPS-X specialization. It is not part of generic/TBD grid storage and is not
// compiled into AVR builds.
static constexpr std::size_t STEPSEQ_TRANSFER_LOCK_CAPACITY = 37;
class StepSeqStep {
public:
    bool active;
    uint8_t locks[STEPSEQ_TRANSFER_LOCK_CAPACITY];
    int8_t microtiming;
    bool trig;
    bool slide;
    bool accent;
    bool swing;
    bool mute;
    StepSeqStepDescriptor data;
};

// ============================================================================
// Lock-count-specialized track storage
// ============================================================================

#pragma pack(push, 1)
template <std::size_t LockCount>
class BasicStepSeqTrackFixedData {
public:
    static_assert(LockCount <= 64, "step lock masks are 64-bit");

    StepSeqStepDescriptor steps[STEPSEQ_NUM_STEPS];
    int8_t microtiming[STEPSEQ_NUM_STEPS];

    uint64_t trig_mask;
    uint64_t slide_mask;
    uint64_t accent_mask;
    uint64_t swing_mask;
    uint64_t mute_mask;

    uint8_t track_length;
    uint8_t track_speed;
    uint8_t locks_params[LockCount];

    void clean_params() {
        uint64_t used_locks = 0;
        for (uint8_t step = 0; step < STEPSEQ_NUM_STEPS; ++step)
            used_locks |= steps[step].locks;
        for (std::size_t lock = 0; lock < LockCount; ++lock) {
            if ((used_locks & (uint64_t{1} << lock)) == 0)
                locks_params[lock] = 0;
        }
    }

    uint8_t find_param(uint8_t param_id) const {
        ++param_id;
        if (param_id == 0)
            return 255;
        for (std::size_t lock = 0; lock < LockCount; ++lock) {
            if (locks_params[lock] == param_id)
                return static_cast<uint8_t>(lock);
        }
        return 255;
    }

    uint16_t get_lockidx(uint8_t step) const {
        uint16_t index = 0;
        for (uint8_t current = 0; current < step; ++current)
            index += stepseq_popcount(steps[current].locks);
        return index;
    }

    uint16_t get_lockidx(uint8_t step, uint8_t lock_idx) const {
        const uint64_t mask = uint64_t{1} << lock_idx;
        if (!steps[step].is_lock_bit(lock_idx))
            return STEPSEQ_NUM_LOCK_SLOTS;
        return static_cast<uint16_t>(
            get_lockidx(step) +
            stepseq_popcount(steps[step].locks & (mask - 1)));
    }

    bool hasData() const { return trig_mask != 0; }
};

template <std::size_t LockCount>
class BasicStepSeqTrackDataV1
    : public BasicStepSeqTrackFixedData<LockCount> {
public:
    uint8_t locks[STEPSEQ_NUM_LOCK_SLOTS];

    const uint8_t* data() const {
        return reinterpret_cast<const uint8_t*>(this);
    }
    uint8_t* data() { return reinterpret_cast<uint8_t*>(this); }
    static constexpr std::size_t dataSize() {
        return sizeof(BasicStepSeqTrackDataV1);
    }

    void init() {
        std::memset(this, 0, sizeof(BasicStepSeqTrackDataV1));
        this->swing_mask = STEPSEQ_DEFAULT_SWING_MASK;
        this->track_speed = 0xFF;
    }

    uint16_t used_lock_slots() const {
        uint16_t used = 0;
        for (uint8_t step = 0; step < STEPSEQ_NUM_STEPS; ++step)
            used += stepseq_popcount(this->steps[step].locks);
        return used > STEPSEQ_NUM_LOCK_SLOTS ? STEPSEQ_NUM_LOCK_SLOTS : used;
    }

    uint16_t store_size() const {
        return static_cast<uint16_t>(
            reinterpret_cast<const uint8_t*>(locks) -
            reinterpret_cast<const uint8_t*>(this) + used_lock_slots());
    }
};

template <std::size_t LockCount>
class BasicStepSeqTrackData
    : public BasicStepSeqTrackFixedData<LockCount> {
public:
    uint8_t swing_amount;
    uint8_t locks[STEPSEQ_NUM_LOCK_SLOTS];

    const uint8_t* data() const {
        return reinterpret_cast<const uint8_t*>(this);
    }
    uint8_t* data() { return reinterpret_cast<uint8_t*>(this); }
    static constexpr std::size_t dataSize() {
        return sizeof(BasicStepSeqTrackData);
    }

    void init() {
        std::memset(this, 0, sizeof(BasicStepSeqTrackData));
        this->swing_mask = STEPSEQ_DEFAULT_SWING_MASK;
        this->track_speed = 0xFF;
    }

    uint16_t used_lock_slots() const {
        uint16_t used = 0;
        for (uint8_t step = 0; step < STEPSEQ_NUM_STEPS; ++step)
            used += stepseq_popcount(this->steps[step].locks);
        return used > STEPSEQ_NUM_LOCK_SLOTS ? STEPSEQ_NUM_LOCK_SLOTS : used;
    }

    uint16_t store_size() const {
        return static_cast<uint16_t>(
            reinterpret_cast<const uint8_t*>(locks) -
            reinterpret_cast<const uint8_t*>(this) + used_lock_slots());
    }
};
#pragma pack(pop)

using StepSeqTrackFixedData = BasicStepSeqTrackFixedData<STEPSEQ_NUM_LOCKS>;
using StepSeqTrackDataV1 = BasicStepSeqTrackDataV1<STEPSEQ_NUM_LOCKS>;
using StepSeqTrackData = BasicStepSeqTrackData<STEPSEQ_NUM_LOCKS>;

// These are the established generic/TBD row dimensions. Keep them exact so
// widening the hosted SPS-X specialization cannot silently change other grid
// formats.
static_assert(sizeof(StepSeqTrackFixedData) == 716,
              "generic/TBD fixed step storage wire size changed");
static_assert(sizeof(StepSeqTrackDataV1) == 972,
              "generic/TBD v1 step storage wire size changed");
static_assert(sizeof(StepSeqTrackData) == 973,
              "generic/TBD step storage wire size changed");
static_assert(sizeof(StepSeqTrackDataV1) ==
                  sizeof(StepSeqTrackFixedData) + STEPSEQ_NUM_LOCK_SLOTS,
              "generic step-sequence v1 storage size changed");
static_assert(sizeof(StepSeqTrackData) ==
                  sizeof(StepSeqTrackFixedData) + sizeof(uint8_t) +
                      STEPSEQ_NUM_LOCK_SLOTS,
              "generic step-sequence storage size changed");

template <std::size_t LockCount>
class ATTR_PACKED() BasicStepSeqTrackStorage
    : public SeqTrackModStorage,
      public BasicStepSeqTrackData<LockCount> {
public:
    void init_storage() {
        SeqTrackModStorage::init_mod();
        BasicStepSeqTrackData<LockCount>::init();
    }

    uint16_t store_size() const {
        return static_cast<uint16_t>(
            sizeof(SeqTrackModStorage) +
            BasicStepSeqTrackData<LockCount>::store_size());
    }
};

using StepSeqTrackStorage = BasicStepSeqTrackStorage<STEPSEQ_NUM_LOCKS>;
static_assert(sizeof(StepSeqTrackStorage) ==
                  sizeof(SeqTrackModStorage) + sizeof(StepSeqTrackData),
              "generic step-sequence track storage size changed");

#endif // !defined(__AVR__)
#endif // STEP_SEQ_TRACK_DATA_H__
