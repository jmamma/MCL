#if !defined(__AVR__)

#include "StepSeqTrack.h"
#include "MCLSeq.h"
#include "MCLSysConfig.h"
#include "MidiClock.h"

namespace {

uint64_t stepseq_mask_for_len(uint8_t len) {
    return len >= 64 ? ~0ULL : ((1ULL << len) - 1);
}

void stepseq_rotate_left_mask(uint64_t &mask, uint8_t len) {
    if (len == 0) return;
    if (len >= 64) {
        uint64_t msb = (mask >> 63) & 1ULL;
        mask = (mask << 1) | msb;
        return;
    }
    uint64_t msb = (mask >> (len - 1)) & 1ULL;
    mask = ((mask << 1) | msb) & stepseq_mask_for_len(len);
}

void stepseq_rotate_right_mask(uint64_t &mask, uint8_t len) {
    if (len == 0) return;
    if (len >= 64) {
        uint64_t lsb = mask & 1ULL;
        mask = (mask >> 1) | (lsb << 63);
        return;
    }
    uint64_t lsb = mask & 1ULL;
    mask = ((mask >> 1) | (lsb << (len - 1))) & stepseq_mask_for_len(len);
}

} // namespace

// ============================================================================
// StepSeqTrack - Timing Methods
// ============================================================================

uint16_t StepSeqTrack::get_ticks_per_step(uint8_t speed_) {
    uint16_t tps;
    switch (speed_) {
    default:
    case STEPSEQ_SPEED_1X:   tps = STEPSEQ_TICKS_PER_STEP_1X; break;
    case STEPSEQ_SPEED_2X:   tps = STEPSEQ_TICKS_PER_STEP_1X / 2; break;
    case STEPSEQ_SPEED_4X:   tps = STEPSEQ_TICKS_PER_STEP_1X / 4; break;
    case STEPSEQ_SPEED_3_4X: tps = STEPSEQ_TICKS_PER_STEP_1X * 4 / 3; break;
    case STEPSEQ_SPEED_3_2X: tps = STEPSEQ_TICKS_PER_STEP_1X * 2 / 3; break;
    case STEPSEQ_SPEED_1_2X: tps = STEPSEQ_TICKS_PER_STEP_1X * 2; break;
    case STEPSEQ_SPEED_1_4X: tps = STEPSEQ_TICKS_PER_STEP_1X * 4; break;
    case STEPSEQ_SPEED_1_8X: tps = STEPSEQ_TICKS_PER_STEP_1X * 8; break;
    }
    return tps;
}

uint8_t StepSeqTrack::get_quantized_step(int8_t &microtiming_out, uint8_t quant) {
    if (quant == 255) quant = mcl_cfg.rec_quant;

    uint16_t tps = get_ticks_per_step();
    int16_t tick_offset = (tick_counter == 0) ? 0 : (int16_t)(tick_counter - 1);
    uint8_t step = step_count;
    int16_t signed_offset = tick_offset;

    if (tick_offset > (int16_t)(tps / 2)) {
        step++;
        if (step == length) step = 0;
        signed_offset = tick_offset - (int16_t)tps;
    }

    if (quant) {
        microtiming_out = 0;
        return step;
    }

    int16_t timing_range = (int16_t)(tps / 4);
    if (timing_range < 1) timing_range = 1;

    int16_t mt = (int16_t)((int32_t)signed_offset * 128 / timing_range);
    if (mt < -127) mt = -127;
    if (mt > 127) mt = 127;
    microtiming_out = (int8_t)mt;
    return step;
}

// ============================================================================
// StepSeqTrackCond - Conditional Evaluation
// ============================================================================

void StepSeqTrackCond::record_trig_result(bool fired) {
    prev_trig_fired = fired;
    if (seq_class) {
        if (fired) {
            STEPSEQ_SET_BIT16(seq_class->neighbor_trig_mask, track_number);
        } else {
            STEPSEQ_CLEAR_BIT16(seq_class->neighbor_trig_mask, track_number);
        }
    }
}

bool StepSeqTrackCond::neighbor_fired() const {
    if (!seq_class || track_number == 0) return false;
    return STEPSEQ_IS_BIT_SET16(seq_class->neighbor_trig_mask, track_number - 1);
}

bool StepSeqTrackCond::conditional(uint8_t condition) {
    if (condition <= STEPSEQ_COND_10PCT) {
        static const uint8_t pct_thresholds[] = {
            255, 230, 192, 169, 128, 84, 64, 26
        };
        if (condition == STEPSEQ_COND_100PCT) return true;
        return stepseq_get_random_byte() <= pct_thresholds[condition];
    }

    switch (condition) {
    case STEPSEQ_COND_ONESHOT:   return true;
    case STEPSEQ_COND_FIRST:     return first_run;
    case STEPSEQ_COND_NOT_FIRST: return !first_run;
    case STEPSEQ_COND_FILL:
        return seq_class && STEPSEQ_IS_BIT_SET16(seq_class->fill_mask, track_number);
    case STEPSEQ_COND_NOT_FILL:
        return !seq_class || !STEPSEQ_IS_BIT_SET16(seq_class->fill_mask, track_number);
    case STEPSEQ_COND_PRE:       return prev_trig_fired;
    case STEPSEQ_COND_NOT_PRE:   return !prev_trig_fired;
    case STEPSEQ_COND_NEI:       return neighbor_fired();
    case STEPSEQ_COND_NOT_NEI:   return !neighbor_fired();
    default: break;
    }

    if (condition >= STEPSEQ_COND_ITER_BASE && condition <= STEPSEQ_COND_ITER_MAX) {
        uint8_t x, y;
        if (stepseq_cond_iter_decode(condition, x, y)) {
            return get_iteration(y) == x;
        }
    }

    return true;
}

// ============================================================================
// StepSeqSlideTrack - Slide/Glide Implementation
// ============================================================================

void StepSeqSlideTrack::on_slide_dispatch_begin(uint8_t) {}
void StepSeqSlideTrack::on_slide_dispatch_end() {}

void StepSeqSlideTrack::dispatch_slide_value(uint8_t param, uint8_t value, uint8_t channel) {
    if (port) {
        port->sendCC(channel, param, value);
    }
}

void StepSeqSlideTrack::prepare_slide(uint8_t lock_idx, int16_t x0, int16_t x1, int8_t y0, int8_t y1) {
    uint8_t c = lock_idx;
    locks_slide_data[c].x0 = x0;
    locks_slide_data[c].x1 = x1;
    locks_slide_data[c].y0 = y0;
    locks_slide_data[c].y1 = y1;
    locks_slide_data[c].accum = 0;

    int16_t steps = x1 - x0;
    if (steps > 0) {
        int16_t y_diff = y1 - y0;
        int32_t delta_calc = ((int32_t)y_diff << 8) / steps;
        locks_slide_data[c].delta = (uint16_t)delta_calc;
    } else {
        locks_slide_data[c].delta = 0;
    }
}

void StepSeqSlideTrack::send_slides(volatile uint8_t *locks_params, uint8_t channel) {
    on_slide_dispatch_begin(channel);
    for (uint8_t c = 0; c < STEPSEQ_NUM_LOCKS; c++) {
        if (locks_params[c] == 0) continue;
        if (locks_slide_data[c].x0 >= locks_slide_data[c].x1) continue;

        int16_t val_calc = locks_slide_data[c].y0 +
                           ((int16_t)(locks_slide_data[c].accum + 128) >> 8);
        uint8_t val = (uint8_t)val_calc;

        locks_slide_data[c].accum += locks_slide_data[c].delta;
        locks_slide_data[c].x0++;

        if (locks_slide_data[c].x0 >= locks_slide_data[c].x1) {
            val = (uint8_t)locks_slide_data[c].y1;
            locks_slide_data[c].init();
        }

        uint8_t param = locks_params[c] - 1;
        dispatch_slide_value(param, val, channel);
    }
    on_slide_dispatch_end();
}

// ============================================================================
// StepSeqDataTrack - Driver Hooks
// ============================================================================

bool StepSeqDataTrack::get_default_lock_value(uint8_t, uint8_t &) const {
    return false;
}

uint8_t StepSeqDataTrack::velocity_lock_param() const {
    return 255;
}

uint8_t StepSeqDataTrack::pitch_lock_param() const {
    return 0;
}

void StepSeqDataTrack::clear_step_oneshot(uint8_t) {}

void StepSeqDataTrack::on_modify_track_begin() {}

// ============================================================================
// StepSeqDataTrack - Slide Scan
// ============================================================================

int16_t StepSeqDataTrack::effective_timing_offset(uint8_t step,
                                                  uint16_t tps) const {
    int8_t mt = microtiming[step];
    if (mt == 0 && swing_amount &&
        STEPSEQ_IS_BIT_SET64(swing_mask, step)) {
        return (int16_t)(((uint16_t)swing_amount * tps + 25) / 50);
    }
    return stepseq_microtiming_to_ticks(mt, tps);
}

void StepSeqDataTrack::recalc_slides() {
    if (locks_slides_recalc == 255) return;

    int16_t x0, x1;
    int8_t y0, y1;
    uint8_t step = locks_slides_recalc;
    uint16_t tps = get_ticks_per_step();

    uint64_t find_mask = 0;
    uint64_t cur_mask = 1ULL;
    for (uint8_t i = 0; i < STEPSEQ_NUM_LOCKS; i++) {
        if (locks_params[i] && (steps[step].locks & cur_mask)) {
            find_mask |= cur_mask;
        }
        cur_mask <<= 1;
    }

    auto lockidx = locks_slides_idx;
    if (find_mask == 0) goto end;

    find_next_locks((uint8_t)lockidx, step, find_mask);

    for (uint8_t c = 0; c < STEPSEQ_NUM_LOCKS; c++) {
        if (!locks_params[c] || !steps[step].is_lock_bit(c)) continue;

        auto cur_lockidx = lockidx++;

        if (find_mask & (1ULL << c)) {
            locks_slide_data[c].init();
            continue;
        }

        auto next_lockstep = locks_slide_next_lock_step[c];
        if (step == next_lockstep) {
            locks_slide_data[c].init();
            continue;
        }

        int16_t mt_offset = effective_timing_offset(step, tps);
        int16_t mt_next_offset = effective_timing_offset(next_lockstep, tps);

        x0 = (int16_t)(step * tps + mt_offset + 1);
        if (next_lockstep < step) {
            x1 = (int16_t)((length + next_lockstep) * tps +
                           mt_next_offset - 1);
        } else {
            x1 = (int16_t)(next_lockstep * tps + mt_next_offset - 1);
        }

        y0 = (int8_t)locks[cur_lockidx];
        y1 = (int8_t)locks_slide_next_lock_val[c];
        prepare_slide(c, x0, x1, y0, y1);
    }

end:
    locks_slides_recalc = 255;
}

void StepSeqDataTrack::find_next_locks(uint8_t curidx, uint8_t step,
                                       uint64_t &mask) {
    uint8_t next_step = step + 1;
    uint8_t max_len = length;
    curidx += stepseq_popcount(steps[step].locks);

again:
    for (; next_step < max_len; next_step++) {
        uint64_t cur_mask = 1ULL;
        auto lcks = steps[next_step].locks;
        bool next_step_has_trig = STEPSEQ_IS_BIT_SET64(trig_mask, next_step);

        if (!lcks && !next_step_has_trig) continue;

        for (uint8_t i = 0; i < STEPSEQ_NUM_LOCKS; ++i) {
            if (mask & cur_mask) {
                if (lcks & cur_mask) {
                    locks_slide_next_lock_val[i] = locks[curidx];
                    locks_slide_next_lock_step[i] = next_step;
                    mask &= ~cur_mask;
                } else if (next_step_has_trig && locks_params[i]) {
                    uint8_t default_value = 0;
                    if (get_default_lock_value(locks_params[i] - 1,
                                               default_value)) {
                        locks_slide_next_lock_val[i] = default_value;
                        locks_slide_next_lock_step[i] = next_step;
                        mask &= ~cur_mask;
                    }
                }
                if (!mask) return;
            }
            if (lcks & cur_mask) curidx++;
            cur_mask <<= 1;
        }
    }

    if (next_step >= length) {
        next_step = 0;
        curidx = 0;
        max_len = step;
        goto again;
    }
}

// ============================================================================
// StepSeqDataTrack - Step Accessors
// ============================================================================

void StepSeqDataTrack::get_mask(uint64_t *_pmask, uint8_t mask_type) const {
    switch (mask_type) {
    case STEPSEQ_MASK_PATTERN: *_pmask = trig_mask; return;
    case STEPSEQ_MASK_SLIDE:   *_pmask = slide_mask; return;
    case STEPSEQ_MASK_MUTE:    *_pmask = mute_mask; return;
    case STEPSEQ_MASK_SWING:   *_pmask = swing_mask; return;
    default: break;
    }
    *_pmask = 0;
    for (uint8_t i = 0; i < STEPSEQ_NUM_STEPS; i++) {
        bool set_bit = false;
        switch (mask_type) {
        case STEPSEQ_MASK_LOCKS_ON_STEP: if (steps[i].locks) set_bit = true; break;
        case STEPSEQ_MASK_LOCK: if (steps[i].locks) set_bit = true; break;
        default: break;
        }
        if (set_bit) *_pmask |= (1ULL << i);
    }
}

bool StepSeqDataTrack::get_step(uint8_t step, uint8_t mask_type) const {
    switch (mask_type) {
    case STEPSEQ_MASK_PATTERN: return STEPSEQ_IS_BIT_SET64(trig_mask, step);
    case STEPSEQ_MASK_LOCK:    return steps[step].locks != 0;
    case STEPSEQ_MASK_MUTE:    return STEPSEQ_IS_BIT_SET64(mute_mask, step);
    case STEPSEQ_MASK_SLIDE:   return STEPSEQ_IS_BIT_SET64(slide_mask, step);
    case STEPSEQ_MASK_SWING:   return STEPSEQ_IS_BIT_SET64(swing_mask, step);
    default: return false;
    }
}

void StepSeqDataTrack::set_step(uint8_t step, uint8_t mask_type, bool val) {
    switch (mask_type) {
    case STEPSEQ_MASK_PATTERN:
        if (val) STEPSEQ_SET_BIT64(trig_mask, step);
        else STEPSEQ_CLEAR_BIT64(trig_mask, step);
        break;
    case STEPSEQ_MASK_LOCK:
        break;
    case STEPSEQ_MASK_MUTE:
        if (val) STEPSEQ_SET_BIT64(mute_mask, step);
        else STEPSEQ_CLEAR_BIT64(mute_mask, step);
        break;
    case STEPSEQ_MASK_SLIDE:
        if (val) STEPSEQ_SET_BIT64(slide_mask, step);
        else STEPSEQ_CLEAR_BIT64(slide_mask, step);
        break;
    case STEPSEQ_MASK_SWING:
        if (val) STEPSEQ_SET_BIT64(swing_mask, step);
        else STEPSEQ_CLEAR_BIT64(swing_mask, step);
        break;
    default:
        break;
    }
}

// ============================================================================
// StepSeqDataTrack - Lock Management
// ============================================================================

void StepSeqDataTrack::get_step_locks(uint8_t step, uint8_t *params,
                                      bool include_all_locks) {
    (void)include_all_locks;
    uint16_t lock_idx = get_lockidx(step);
    for (uint8_t c = 0; c < STEPSEQ_NUM_LOCKS; c++) {
        bool lock_bit = steps[step].is_lock_bit(c);
        if (locks_params[c]) {
            uint8_t param = locks_params[c] - 1;
            if (lock_bit) {
                params[param] = locks[lock_idx];
            }
        }
        lock_idx += lock_bit;
    }
}

uint8_t StepSeqDataTrack::get_track_lock_implicit(uint8_t step,
                                                  uint8_t param) {
    uint8_t lock_idx = find_param(param);
    if (lock_idx < STEPSEQ_NUM_LOCKS) return get_track_lock(step, lock_idx);
    return 255;
}

uint8_t StepSeqDataTrack::get_track_lock(uint8_t step, uint8_t lock_idx) {
    auto idx = get_lockidx(step, lock_idx);
    if (idx < STEPSEQ_NUM_LOCK_SLOTS) {
        return locks[idx];
    }
    return 255;
}

bool StepSeqDataTrack::set_track_locks(uint8_t step, uint8_t track_param,
                                       uint8_t value) {
    uint8_t match = find_param(track_param);
    for (uint8_t c = 0; c < STEPSEQ_NUM_LOCKS && match == 255; c++) {
        if (locks_params[c] == 0) {
            locks_params[c] = track_param + 1;
            match = c;
        }
    }
    if (match != 255) return set_track_locks_i(step, match, value);
    return false;
}

bool StepSeqDataTrack::set_track_locks_i(uint8_t step, uint8_t lockidx,
                                         uint8_t value) {
    auto lock_slot = get_lockidx(step, lockidx);
    if (lock_slot == STEPSEQ_NUM_LOCK_SLOTS) {
        auto idx = get_lockidx(step);
        auto nlock = stepseq_popcount(steps[step].locks &
                                      ((1ULL << lockidx) - 1));
        lock_slot = idx + nlock;
        if (lock_slot >= STEPSEQ_NUM_LOCK_SLOTS) return false;
        memmove(locks + lock_slot + 1, locks + lock_slot,
                STEPSEQ_NUM_LOCK_SLOTS - lock_slot - 1);
        if (step < step_count) cur_event_idx++;
        steps[step].locks |= (1ULL << lockidx);
    }
    locks[lock_slot] = (value > 127) ? 127 : value;
    return true;
}

void StepSeqDataTrack::set_track_pitch(uint8_t step, uint8_t pitch) {
    set_track_locks(step, pitch_lock_param(), pitch);
}

void StepSeqDataTrack::set_track_step(uint8_t step, int8_t microtiming_val,
                                      uint8_t velocity) {
    clear_step_oneshot(step);
    STEPSEQ_SET_BIT64(trig_mask, step);
    if (velocity >= 127) {
        STEPSEQ_SET_BIT64(accent_mask, step);
    } else {
        STEPSEQ_CLEAR_BIT64(accent_mask, step);
        uint8_t velocity_param = velocity_lock_param();
        if (velocity_param != 255) {
            set_track_locks(step, velocity_param, velocity);
        }
    }
    steps[step].cond_id = 0;
    steps[step].cond_plock = false;
    microtiming[step] = microtiming_val;
}

// ============================================================================
// StepSeqDataTrack - Recording
// ============================================================================

void StepSeqDataTrack::record_track(uint8_t velocity) {
    if (step_count >= length) return;
    int8_t mt = 0;
    uint8_t step = get_quantized_step(mt);
    ignore_step = step;
    set_track_step(step, mt, velocity);
}

void StepSeqDataTrack::record_track_locks(uint8_t track_param, uint8_t value) {
    if (step_count >= length) return;
    int8_t mt = 0;
    set_track_locks(get_quantized_step(mt), track_param, value);
}

void StepSeqDataTrack::record_track_pitch(uint8_t pitch) {
    if (step_count >= length) return;
    int8_t mt = 0;
    set_track_pitch(get_quantized_step(mt), pitch);
}

// ============================================================================
// StepSeqDataTrack - Clear/Reset
// ============================================================================

void StepSeqDataTrack::clear_mute() {
    mute_mask = 0;
}

void StepSeqDataTrack::clear_mutes() {
    mute_mask = 0;
}

void StepSeqDataTrack::clear_slide_data() {
    slide_mask = 0;
}

void StepSeqDataTrack::clear_step(uint8_t step) {
    uint16_t idx16 = get_lockidx(step);
    uint8_t cnt = stepseq_popcount(steps[step].locks);
    if (cnt != 0 && idx16 + cnt <= STEPSEQ_NUM_LOCK_SLOTS) {
        uint8_t idx = (uint8_t)idx16;
        memmove(locks + idx, locks + idx + cnt,
                STEPSEQ_NUM_LOCK_SLOTS - idx - cnt);
        if (step < step_count) cur_event_idx -= cnt;
    }
    steps[step].locks = 0;
    steps[step].cond_id = 0;
    steps[step].cond_plock = 0;
    microtiming[step] = 0;
    STEPSEQ_CLEAR_BIT64(trig_mask, step);
    STEPSEQ_CLEAR_BIT64(slide_mask, step);
    STEPSEQ_CLEAR_BIT64(accent_mask, step);
    STEPSEQ_CLEAR_BIT64(swing_mask, step);
    STEPSEQ_CLEAR_BIT64(mute_mask, step);
    clear_step_oneshot(step);
}

void StepSeqDataTrack::clear_step_locks(uint8_t step) {
    uint16_t idx16 = get_lockidx(step);
    uint8_t cnt = stepseq_popcount(steps[step].locks);
    if (cnt == 0 || idx16 + cnt > STEPSEQ_NUM_LOCK_SLOTS) return;
    uint8_t idx = (uint8_t)idx16;
    memmove(locks + idx, locks + idx + cnt,
            STEPSEQ_NUM_LOCK_SLOTS - idx - cnt);
    if (step < step_count) cur_event_idx -= cnt;
    steps[step].locks = 0;
    clean_params();
}

void StepSeqDataTrack::disable_step_locks(uint8_t step) {
    (void)step;
}

void StepSeqDataTrack::enable_step_locks(uint8_t step) {
    (void)step;
}

uint64_t StepSeqDataTrack::get_step_locks_mask(uint8_t step) {
    return steps[step].locks;
}

void StepSeqDataTrack::clear_conditional() {
    for (uint8_t c = 0; c < STEPSEQ_NUM_STEPS; c++) {
        steps[c].cond_id = 0;
        steps[c].cond_plock = 0;
        microtiming[c] = 0;
    }
    clear_mutes();
    ignore_step = 255;
}

void StepSeqDataTrack::clear_step_lock(uint8_t step, uint8_t param_id) {
    uint8_t match = find_param(param_id);
    if (match == 255) return;
    uint64_t mask = (1ULL << match);
    uint16_t idx = get_lockidx(step);
    if (!(steps[step].locks & mask)) return;
    uint8_t offset = stepseq_popcount(steps[step].locks & (mask - 1));
    memmove(locks + idx + offset, locks + idx + offset + 1,
            STEPSEQ_NUM_LOCK_SLOTS - idx - offset - 1);
    steps[step].locks &= ~mask;
    locks_slide_data[match].init();
    cur_event_idx = get_lockidx(step_count);
    clean_params();
}

void StepSeqDataTrack::clear_locks() {
    for (uint8_t c = 0; c < STEPSEQ_NUM_LOCKS; c++) locks_params[c] = 0;
    memset(locks, 0, sizeof(locks));
    cur_event_idx = 0;
}

void StepSeqDataTrack::clear_track(bool clear_locks_too) {
    clear_conditional();
    if (clear_locks_too) clear_locks();
    memset(steps, 0, sizeof(steps));
    trig_mask = 0;
    slide_mask = 0;
    accent_mask = 0;
    swing_mask = 0;
    swing_amount = 0;
    memset(microtiming, 0, sizeof(microtiming));
}

void StepSeqDataTrack::clear_param_locks(uint8_t param_id) {
    uint8_t match = find_param(param_id);
    if (match == 255) return;

    uint64_t mask = 1ULL << match;
    uint64_t nmask = ~mask;
    uint64_t rmask = mask - 1;
    bool remove[STEPSEQ_NUM_STEPS];

    for (uint8_t x = 0; x < STEPSEQ_NUM_STEPS; x++) {
        remove[x] = (steps[x].locks & mask) != 0;
        steps[x].locks &= nmask;
    }

    uint16_t rd = 0, wr = 0;
    for (uint8_t i = 0; i < STEPSEQ_NUM_STEPS; ++i) {
        uint64_t _locks = steps[i].locks;
        uint8_t nlocks = stepseq_popcount(_locks);
        uint8_t total = remove[i] ? (uint8_t)(nlocks + 1) : nlocks;
        uint8_t skip = remove[i] ? stepseq_popcount(_locks & rmask) : total;
        for (uint8_t j = 0; j < total; ++j) {
            if (skip == j) { ++rd; }
            else { locks[wr++] = locks[rd++]; }
        }
    }

    locks_slide_data[match].init();
    cur_event_idx = get_lockidx(step_count);
    clean_params();
}

bool StepSeqDataTrack::is_param(uint8_t param_id) {
    for (uint8_t c = 0; c < STEPSEQ_NUM_LOCKS; c++) {
        if (locks_params[c] > 0 && locks_params[c] - 1 == param_id) return true;
    }
    return false;
}

// ============================================================================
// StepSeqDataTrack - Track Editing
// ============================================================================

void StepSeqDataTrack::set_length(uint8_t len, bool expand) {
    uint8_t old_length = length;
    if (len == 0) len = STEPSEQ_NUM_STEPS;
    if (len > STEPSEQ_NUM_STEPS) len = STEPSEQ_NUM_STEPS;
    length = len;
    track_length = len;

    uint8_t step = step_count;
    if (step >= length) step = step % length;
    uint8_t idx = (uint8_t)get_lockidx(step);
    cur_event_idx = idx;
    step_count = step;

    if (expand && old_length > 0 && length > old_length) {
        bool has_existing_data = false;
        for (uint8_t n = old_length; n < length && n < STEPSEQ_NUM_STEPS; n++) {
            if (get_step(n, STEPSEQ_MASK_PATTERN) ||
                get_step(n, STEPSEQ_MASK_LOCK) ||
                steps[n].locks != 0) {
                has_existing_data = true;
                break;
            }
        }
        if (!has_existing_data) {
            StepSeqStep step_copy;
            uint8_t src_idx = 0;
            for (uint8_t n = old_length; n < length && n < STEPSEQ_NUM_STEPS; n++) {
                copy_step(src_idx, &step_copy);
                paste_step(n, &step_copy);
                microtiming[n] = microtiming[src_idx];
                src_idx++;
                if (src_idx >= old_length) src_idx = 0;
            }
            uint64_t old_mask = stepseq_mask_for_len(old_length);
            uint64_t old_trig = trig_mask & old_mask;
            uint64_t old_accent = accent_mask & old_mask;
            uint64_t old_swing = swing_mask & old_mask;
            uint64_t old_slide = slide_mask & old_mask;
            for (uint8_t page = 1; page * old_length < length && page < 4; page++) {
                trig_mask |= (old_trig << (page * old_length));
                accent_mask |= (old_accent << (page * old_length));
                swing_mask |= (old_swing << (page * old_length));
                slide_mask |= (old_slide << (page * old_length));
            }
        }
    }
}

void StepSeqDataTrack::set_speed(uint8_t new_speed, uint8_t old_speed,
                                 bool timing_adjust) {
    if (old_speed == 255) old_speed = speed;
    (void)old_speed;
    (void)timing_adjust;
    speed = new_speed;
    track_speed = new_speed;
    uint16_t tps = get_ticks_per_step();
    if (tick_counter > tps) tick_counter = tick_counter % tps;
}

void StepSeqDataTrack::request_swing_amount_change(uint8_t amount) {
    if (amount > 30) amount = 30;
    if (MidiClock.state != MidiClockClass::STARTED) {
        swing_amount = amount;
        pending_swing_amount = NO_PENDING_SWING_AMOUNT;
        return;
    }

    USE_LOCK();
    SET_LOCK();
    pending_swing_amount = amount;
    CLEAR_LOCK();
}

void StepSeqDataTrack::apply_pending_swing_amount() {
    if (pending_swing_amount == NO_PENDING_SWING_AMOUNT) return;

    USE_LOCK();
    SET_LOCK();
    uint8_t amount = pending_swing_amount;
    pending_swing_amount = NO_PENDING_SWING_AMOUNT;
    CLEAR_LOCK();
    swing_amount = amount;
}

void StepSeqDataTrack::store_mute_state() {
    for (uint8_t n = 0; n < STEPSEQ_NUM_STEPS; n++) {
        if (STEPSEQ_IS_BIT_SET64(mute_mask, n)) {
            set_step(n, STEPSEQ_MASK_PATTERN, false);
            clear_step_locks(n);
        }
    }
    clear_mutes();
}

void StepSeqDataTrack::modify_track(uint8_t dir) {
    uint8_t old_mute_state = mute_state;
    on_modify_track_begin();

    constexpr size_t ncopy = sizeof(steps) - sizeof(StepSeqStepDescriptor);
    uint8_t lock_buf[STEPSEQ_NUM_LOCKS];
    StepSeqStepDescriptor step_buf;
    int8_t mt_buf;
    uint16_t total_nlock = get_lockidx(length);

    mute_state = STEPSEQ_MUTE_ON;

    switch (dir) {
    case STEPSEQ_DIR_LEFT: {
        uint8_t nlock = stepseq_popcount(steps[0].locks);
        memcpy(lock_buf, locks, nlock);
        memmove(locks, locks + nlock, total_nlock - nlock);
        memcpy(locks + total_nlock - nlock, lock_buf, nlock);

        step_buf = steps[0];
        mt_buf = microtiming[0];
        memmove(steps, steps + 1, ncopy);
        memmove(microtiming, microtiming + 1, length - 1);
        steps[length - 1] = step_buf;
        microtiming[length - 1] = mt_buf;
        stepseq_rotate_left_mask(mute_mask, length);
        stepseq_rotate_left_mask(trig_mask, length);
        stepseq_rotate_left_mask(slide_mask, length);
        stepseq_rotate_left_mask(accent_mask, length);
        stepseq_rotate_left_mask(swing_mask, length);
        break;
    }
    case STEPSEQ_DIR_RIGHT: {
        uint8_t nlock = stepseq_popcount(steps[length - 1].locks);
        memcpy(lock_buf, locks + total_nlock - nlock, nlock);
        memmove(locks + nlock, locks, total_nlock - nlock);
        memcpy(locks, lock_buf, nlock);

        step_buf = steps[length - 1];
        mt_buf = microtiming[length - 1];
        memmove(steps + 1, steps, ncopy);
        memmove(microtiming + 1, microtiming, length - 1);
        steps[0] = step_buf;
        microtiming[0] = mt_buf;
        stepseq_rotate_right_mask(mute_mask, length);
        stepseq_rotate_right_mask(trig_mask, length);
        stepseq_rotate_right_mask(slide_mask, length);
        stepseq_rotate_right_mask(accent_mask, length);
        stepseq_rotate_right_mask(swing_mask, length);
        break;
    }
    case STEPSEQ_DIR_REVERSE: {
        uint8_t rev_locks[STEPSEQ_NUM_LOCK_SLOTS];
        memcpy(rev_locks, locks, sizeof(locks));
        uint16_t l = 0, r = 0;

        for (uint8_t i = 0; i <= length / 2; ++i) {
            int j = length - i - 1;
            if (j < (int)i) break;

            uint8_t ni = stepseq_popcount(steps[i].locks);
            uint8_t nj = stepseq_popcount(steps[j].locks);
            memcpy(locks + l, rev_locks + total_nlock - l - nj, nj);
            memcpy(locks + total_nlock - r - ni, rev_locks + r, ni);
            l += nj;
            r += ni;

            step_buf = steps[i];
            steps[i] = steps[j];
            steps[j] = step_buf;

            mt_buf = microtiming[i];
            microtiming[i] = microtiming[j];
            microtiming[j] = mt_buf;

            bool a = STEPSEQ_IS_BIT_SET64(mute_mask, i);
            bool b = STEPSEQ_IS_BIT_SET64(mute_mask, j);
            if (a) STEPSEQ_SET_BIT64(mute_mask, j);
            else STEPSEQ_CLEAR_BIT64(mute_mask, j);
            if (b) STEPSEQ_SET_BIT64(mute_mask, i);
            else STEPSEQ_CLEAR_BIT64(mute_mask, i);
        }
        trig_mask = stepseq_reverse_mask64(trig_mask, length);
        slide_mask = stepseq_reverse_mask64(slide_mask, length);
        accent_mask = stepseq_reverse_mask64(accent_mask, length);
        swing_mask = stepseq_reverse_mask64(swing_mask, length);
        break;
    }
    default:
        break;
    }

    cur_event_idx = get_lockidx(step_count);
    mute_state = old_mute_state;
}

void StepSeqDataTrack::copy_step(uint8_t n, StepSeqStep *step) {
    step->active = true;
    step->microtiming = microtiming[n];
    step->trig = STEPSEQ_IS_BIT_SET64(trig_mask, n);
    step->slide = STEPSEQ_IS_BIT_SET64(slide_mask, n);
    step->accent = STEPSEQ_IS_BIT_SET64(accent_mask, n);
    step->swing = STEPSEQ_IS_BIT_SET64(swing_mask, n);
    step->mute = STEPSEQ_IS_BIT_SET64(mute_mask, n);
    uint16_t idx = get_lockidx(n);
    uint64_t lcks = steps[n].locks;
    uint64_t mask = 1ULL;
    for (uint8_t a = 0; a < STEPSEQ_NUM_LOCKS; a++) {
        if (lcks & mask) {
            step->locks[a] = locks[idx++] + 1;
        } else {
            step->locks[a] = 0;
        }
        mask <<= 1;
    }
    memcpy(&step->data, &steps[n], sizeof(StepSeqStepDescriptor));
}

void StepSeqDataTrack::paste_step(uint8_t n, StepSeqStep *step,
                                  const uint8_t *source_locks_params) {
    clear_step(n);
    microtiming[n] = step->microtiming;
    const uint8_t *lp = source_locks_params ? source_locks_params : locks_params;
    for (uint8_t a = 0; a < STEPSEQ_NUM_LOCKS; a++) {
        if (step->locks[a] != 0 && lp[a] != 0) {
            set_track_locks(n, lp[a] - 1, step->locks[a] - 1);
        }
    }
    steps[n].cond_plock = step->data.cond_plock;
    steps[n].cond_id = step->data.cond_id;
    set_step(n, STEPSEQ_MASK_PATTERN, step->trig);
    set_step(n, STEPSEQ_MASK_SLIDE, step->slide);
    set_step(n, STEPSEQ_MASK_MUTE, step->mute);
    if (step->accent) STEPSEQ_SET_BIT64(accent_mask, n);
    else STEPSEQ_CLEAR_BIT64(accent_mask, n);
    if (step->swing) STEPSEQ_SET_BIT64(swing_mask, n);
    else STEPSEQ_CLEAR_BIT64(swing_mask, n);
}

void StepSeqDataTrack::transpose(int8_t offset) {
    for (uint8_t n = 0; n < STEPSEQ_NUM_STEPS; n++) {
        uint8_t pitch = get_track_lock_implicit(n, pitch_lock_param());
        if (pitch == 255) continue;
        int16_t new_pitch = pitch + offset;
        if (new_pitch < 0) new_pitch = 0;
        if (new_pitch > 127) new_pitch = 127;
        set_track_pitch(n, (uint8_t)new_pitch);
    }
}

#endif // !defined(__AVR__)
