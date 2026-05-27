#ifndef STEP_SEQ_TRACK_H__
#define STEP_SEQ_TRACK_H__

#if !defined(__AVR__)

#include "StepSeqDefines.h"
#include "StepSeqTrackData.h"
#include "MidiUart.h"
#include "SeqTrack.h"

class MCLSeq;

// ============================================================================
// StepSeqSlideData - Fixed-point slide interpolation (8.8 format)
// ============================================================================

class StepSeqSlideData {
public:
    int16_t x0;
    int16_t x1;
    uint16_t accum;
    uint16_t delta;
    int8_t y0;
    int8_t y1;

    void init() {
        x0 = 0;
        x1 = 0;
        accum = 0;
        delta = 0;
    }
};

// ============================================================================
// StepSeqTrack - Generic SPS-style step sequencer timing core
// ============================================================================

class StepSeqTrack : public SeqTrack {
public:
    // Set per seq call by the concrete backend.
    MidiUartClass *port = nullptr;
    MidiUartClass *port2 = nullptr;

    uint16_t tick_counter;

    StepSeqTrack() : SeqTrack() {
        active = 0;
        record_mutes = false;
        length = 16;
        speed = STEPSEQ_SPEED_1X;
        step_count = 0;
        tick_counter = 0;
        count_down = 0;
        cache_loaded = true;
        load_sound = false;
    }

    void step_count_inc() {
        if (step_count == length - 1) {
            step_count = 0;
        } else {
            step_count++;
        }
    }

    void update_legacy_progress_counter(uint16_t tps) {
        uint8_t legacy_tps = SeqTrack::get_speed_multiplier_int(speed);
        mod12_counter = (tps == 0)
                            ? 0
                            : (uint8_t)(((uint32_t)tick_counter * legacy_tps) / tps);
    }

    void seq() {
        uint16_t tps = get_ticks_per_step();
        tick_counter++;
        if (count_down) {
            count_down--;
            if (count_down == 0) {
                reset();
            }
        }
        if (tick_counter >= tps) {
            count_down = 0;
            tick_counter = 0;
            step_count_inc();
        }
        update_legacy_progress_counter(tps);
    }

    void reset() {
        tick_counter = 0;
        mod12_counter = 0;
        step_count = 0;
        count_down = 0;
        cache_loaded = true;
        load_sound = false;
    }

    void toggle_mute() { mute_state = !mute_state; }

    uint16_t get_ticks_per_step(uint8_t speed_);
    uint16_t get_ticks_per_step() { return get_ticks_per_step(speed); }

    uint16_t get_ticks_per_step_inline() {
        uint16_t tps;
        switch (speed) {
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

    uint8_t get_quantized_step(int8_t &microtiming_out, uint8_t quant = 255);
};

// ============================================================================
// StepSeqTrackCond - Generic conditional trigger state
// ============================================================================

class StepSeqTrackCond : public StepSeqTrack {
public:
    uint8_t iterations[8];
    uint16_t cur_event_idx;
    uint8_t ignore_step;

    bool prev_trig_fired;
    bool first_run;

    // Shared sequencer state for fill and neighbor trigger conditions.
    MCLSeq* seq_class = nullptr;

    StepSeqTrackCond() : StepSeqTrack() {
        reset();
    }

    void reset() {
        cur_event_idx = 0;
        for (uint8_t i = 0; i < 8; i++) {
            iterations[i] = 1;
        }
        prev_trig_fired = false;
        first_run = true;
        StepSeqTrack::reset();
        ignore_step = 255;
    }

    void seq() {
        uint16_t tps = get_ticks_per_step();
        tick_counter++;
        if (count_down) {
            count_down--;
            if (count_down == 0) {
                reset();
            }
        }
        if (tick_counter >= tps) {
            count_down = 0;
            tick_counter = 0;
            step_count_inc();
        }
        update_legacy_progress_counter(tps);
    }

    void step_count_inc() {
        if (step_count == length - 1) {
            step_count = 0;
            cur_event_idx = 0;
            first_run = false;
            for (uint8_t i = 1; i < 8; i++) {
                uint8_t cycle_len = i + 1;
                iterations[i] = (iterations[i] >= cycle_len) ? 1 : iterations[i] + 1;
            }
        } else {
            step_count++;
        }
    }

    uint8_t get_iteration(uint8_t y) const {
        if (y < 2 || y > 8) return 1;
        return iterations[y - 1];
    }

    virtual void set_length(uint8_t len, bool expand = false) = 0;
    virtual void clear_track(bool locks = true) = 0;
    virtual void rotate_left() = 0;
    virtual void rotate_right() = 0;
    virtual void reverse() = 0;
    virtual void transpose(int8_t offset) = 0;

    bool conditional(uint8_t condition);
    void record_trig_result(bool fired);
    bool neighbor_fired() const;
};

// ============================================================================
// StepSeqSlideTrack - Generic parameter slide/glide state
// ============================================================================

class StepSeqSlideTrack : public StepSeqTrackCond {
public:
    StepSeqSlideData locks_slide_data[STEPSEQ_NUM_LOCKS];
    uint8_t locks_slide_next_lock_val[STEPSEQ_NUM_LOCKS];
    uint8_t locks_slide_next_lock_step[STEPSEQ_NUM_LOCKS];
    uint8_t locks_slides_recalc = 255;
    uint16_t locks_slides_idx = 0;

    StepSeqSlideTrack() : StepSeqTrackCond() {}

    void reset() {
        for (uint8_t n = 0; n < STEPSEQ_NUM_LOCKS; n++) {
            locks_slide_data[n].init();
        }
        StepSeqTrackCond::reset();
    }

    void prepare_slide(uint8_t lock_idx, int16_t x0, int16_t x1, int8_t y0, int8_t y1);
    virtual void send_slides(volatile uint8_t *locks_params, uint8_t channel = 0);

protected:
    virtual void on_slide_dispatch_begin(uint8_t channel);
    virtual void dispatch_slide_value(uint8_t param, uint8_t value, uint8_t channel);
    virtual void on_slide_dispatch_end();
};

// ============================================================================
// StepSeqDataTrack - Generic SPS-style step data operations
// ============================================================================

class StepSeqDataTrack : public StepSeqTrackData, public StepSeqSlideTrack {
public:
    static constexpr uint8_t NO_PENDING_SWING_AMOUNT = 0xFF;

    volatile uint8_t pending_swing_amount;

    StepSeqDataTrack() : StepSeqSlideTrack() {
        StepSeqTrackData::init();
        mute_mask = 0;
        pending_swing_amount = NO_PENDING_SWING_AMOUNT;
    }

    void reset() {
        StepSeqSlideTrack::reset();
        record_mutes = false;
        pending_swing_amount = NO_PENDING_SWING_AMOUNT;
    }

    void get_mask(uint64_t *_pmask, uint8_t mask_type) const;
    bool get_step(uint8_t step, uint8_t mask_type) const;
    void set_step(uint8_t step, uint8_t mask_type, bool val);

    void get_step_locks(uint8_t step, uint8_t *params,
                        bool include_all_locks = false);
    int16_t effective_timing_offset(uint8_t step, uint16_t tps) const;
    void recalc_slides();
    void find_next_locks(uint8_t curidx, uint8_t step, uint64_t &mask);

    void set_track_pitch(uint8_t step, uint8_t pitch);
    void set_track_step(uint8_t step, int8_t microtiming_val,
                        uint8_t velocity = 127);
    bool set_track_locks_i(uint8_t step, uint8_t lockidx, uint8_t value);
    bool set_track_locks(uint8_t step, uint8_t track_param, uint8_t value);
    uint8_t get_track_lock(uint8_t step, uint8_t lockidx);
    uint8_t get_track_lock_implicit(uint8_t step, uint8_t param);

    void record_track(uint8_t velocity);
    void record_track_locks(uint8_t track_param, uint8_t value);
    void record_track_pitch(uint8_t pitch);

    void clear_mute();
    virtual void clear_mutes();
    void clear_slide_data();
    void clear_step(uint8_t step);
    void clear_step_locks(uint8_t step);
    void disable_step_locks(uint8_t step);
    void enable_step_locks(uint8_t step);
    uint64_t get_step_locks_mask(uint8_t step);
    void clear_conditional();
    void clear_step_lock(uint8_t step, uint8_t param_id);
    void clear_locks();
    void clear_track(bool locks = true) override;
    void clear_param_locks(uint8_t param_id);
    bool is_param(uint8_t param_id);
    virtual bool owns_sound_data() const { return false; }
    virtual bool preview_step(uint8_t step) {
        (void)step;
        return false;
    }

    void set_length(uint8_t len, bool expand = false) override;
    void rotate_left() override { modify_track(STEPSEQ_DIR_LEFT); }
    void rotate_right() override { modify_track(STEPSEQ_DIR_RIGHT); }
    void reverse() override { modify_track(STEPSEQ_DIR_REVERSE); }
    void modify_track(uint8_t dir);

    void set_speed(uint8_t new_speed, uint8_t old_speed = 255,
                   bool timing_adjust = true);
    void request_swing_amount_change(uint8_t amount);
    void apply_pending_swing_amount();
    void store_mute_state();

    void copy_step(uint8_t n, StepSeqStep *step);
    void paste_step(uint8_t n, StepSeqStep *step,
                    const uint8_t *source_locks_params = nullptr);

    void transpose(int8_t offset) override;

protected:
    virtual bool get_default_lock_value(uint8_t param_id, uint8_t &value) const;
    virtual uint8_t velocity_lock_param() const;
    virtual uint8_t pitch_lock_param() const;
    virtual void clear_step_oneshot(uint8_t step);
    virtual void on_modify_track_begin();
};

#endif // !defined(__AVR__)
#endif // STEP_SEQ_TRACK_H__
