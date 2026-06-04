/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MDSEQTRACKDATA_H__
#define MDSEQTRACKDATA_H__

#include "helpers.h"

#define NUM_LOCKS_270 4
#define NUM_MD_STEPS_270 64

#define NUM_MD_LOCK_SLOTS 256
#define NUM_MD_STEPS 64
#define MDSEQ_DEFAULT_SWING_MASK 0xAAAAAAAAAAAAAAAAULL


class ATTR_PACKED() MDSeqStepDescriptor {
public:
  uint8_t
      locks; // <-- bitfield of 8 locks in the current step, first lock is lsb
  bool trig : 1;
  bool cond_plock : 1;
  uint8_t cond_id : 6;
  bool is_lock_bit(const uint8_t idx) const { return locks & (1 << idx); }
  bool is_lock(const uint8_t idx) const { return is_lock_bit(idx); }
};

class MDSeqStep {
public:
  uint8_t locks[NUM_LOCKS];
  int8_t microtiming;
  bool swing;
  bool slide;
  bool mute;
  MDSeqStepDescriptor data;
};

class ATTR_PACKED() MDSeqTrackDataV1 {
public:
  MDSeqStepDescriptor steps[NUM_MD_STEPS];
  int8_t microtiming[NUM_MD_STEPS];
  uint8_t locks_params[NUM_LOCKS];
  uint8_t locks[NUM_MD_LOCK_SLOTS];

  // !! Note lockidx is lock index, not param id
  bool set_track_locks_i(uint8_t step, uint8_t lockidx, uint8_t velocity);
  // !! Note track_param is param_id, not lock index
  bool set_track_locks(uint8_t step, uint8_t track_param, uint8_t velocity);
  // !! Note lockidx is lock index, not param_id
  uint8_t get_track_lock(uint8_t step, uint8_t lockidx);

  // get the pointer to the data chunk.
  // useful to skip the vtable

  uint8_t *data() const { return (uint8_t *)&steps; }

  void clean_params() {
    uint8_t _locks = 0;
    for (uint8_t x = 0; x < NUM_MD_STEPS; x++) {
      _locks |= steps[x].locks;
    }

    uint8_t mask = 1;

    for (uint8_t a = 0; a < NUM_LOCKS; a++) {
      if (!(_locks & mask)) {
        locks_params[a] = 0;
      }
      mask <<= 1;
    }
  }

  uint8_t find_param(uint8_t param_id) const {
    param_id += 1;
    if (!param_id)
      return 255;
    for (uint8_t c = 0; c < NUM_LOCKS; c++) {
      if (locks_params[c] == param_id) {
        return c;
      }
    }
    return 255;
  }

  uint16_t get_lockidx(uint8_t step) const {
    uint16_t idx = 0;
    for (uint8_t i = 0; i < step; ++i) {
      idx += popcount(steps[i].locks);
    }
    return idx;
  }

  uint16_t get_lockidx(uint8_t step, uint8_t lock_idx) const {
    uint8_t mask = 1 << lock_idx;
    uint8_t rmask = (mask - 1);
    if (steps[step].is_lock_bit(lock_idx)) {
      auto idx = get_lockidx(step) + popcount(steps[step].locks & rmask);
      return idx;
    } else {
      return NUM_MD_LOCK_SLOTS;
    }
  }

  uint8_t remove_step_locks(uint8_t step);

  void init() { memset(this, 0, sizeof(MDSeqTrackDataV1)); }
};

class ATTR_PACKED() MDSeqTrackData : public MDSeqTrackDataV1 {
public:
  uint64_t mute_mask;
  uint64_t slide_mask;
  uint64_t swing_mask;
  uint8_t swing_amount;

  void set_swing_from_mask(const uint8_t *mask_bytes) {
    memcpy(&swing_mask, mask_bytes, sizeof(swing_mask));
  }

  void set_default_swing() {
    swing_mask = MDSEQ_DEFAULT_SWING_MASK;
  }

  void init() {
    memset(this, 0, sizeof(MDSeqTrackData));
    set_default_swing();
  }
};

static_assert(sizeof(MDSeqTrackData) ==
                  sizeof(MDSeqTrackDataV1) + sizeof(uint64_t) * 3 +
                      sizeof(uint8_t),
              "MDSeqTrackData storage size changed unexpectedly");

#endif /* MDSEQTRACKDATA_H__ */
