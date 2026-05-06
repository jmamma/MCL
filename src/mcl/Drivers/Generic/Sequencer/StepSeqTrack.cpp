#if !defined(__AVR__)

#include "StepSeqTrack.h"
#include "MCLSeq.h"
#include "MCLSysConfig.h"

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

#endif // !defined(__AVR__)
