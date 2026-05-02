#ifndef SPSX_SEQ_TRACK_H__
#define SPSX_SEQ_TRACK_H__

#if !defined(__AVR__)

#include "SPSXSeqDefines.h"
#include "SPSXSeqTrackData.h"
#include "SeqTrack.h"
#include "MidiUart.h"

// Forward declarations
class MCLSeq;
class MDPattern;

// Track type for SPSX tracks
#ifndef SPSX_TRACK_TYPE
#define SPSX_TRACK_TYPE 17
#endif

// ============================================================================
// SlideData - Fixed-point slide interpolation (8.8 format)
// ============================================================================

class SPSXSlideData {
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
// SPSXSeqTrackBase - Base Track Class (timing, speed, step counting)
// ============================================================================

class SPSXSeqTrackBase : public SeqTrack {
public:
    // SPSX uses port/port2 aliases (set per-seq-call, not persistent)
    MidiUartClass *port = nullptr;
    MidiUartClass *port2 = nullptr;

    // SPSX tick counter (16-bit, counts within a step at 96 ticks/step)
    uint16_t tick_counter;

    SPSXSeqTrackBase() : SeqTrack() {
        active = 0;
        record_mutes = false;
        length = 16;
        speed = SPSX_SPEED_1X;
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
    }

    void reset() {
        tick_counter = 0;
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
        case SPSX_SPEED_1X:   tps = SPSX_TICKS_PER_STEP_1X; break;
        case SPSX_SPEED_2X:   tps = SPSX_TICKS_PER_STEP_1X / 2; break;
        case SPSX_SPEED_4X:   tps = SPSX_TICKS_PER_STEP_1X / 4; break;
        case SPSX_SPEED_3_4X: tps = SPSX_TICKS_PER_STEP_1X * 4 / 3; break;
        case SPSX_SPEED_3_2X: tps = SPSX_TICKS_PER_STEP_1X * 2 / 3; break;
        case SPSX_SPEED_1_2X: tps = SPSX_TICKS_PER_STEP_1X * 2; break;
        case SPSX_SPEED_1_4X: tps = SPSX_TICKS_PER_STEP_1X * 4; break;
        case SPSX_SPEED_1_8X: tps = SPSX_TICKS_PER_STEP_1X * 8; break;
        }
        return tps;
    }

    uint8_t get_quantized_step(int8_t &microtiming_out, uint8_t quant = 255);
};

// ============================================================================
// SPSXSeqTrackCond - Track with Conditional Logic
// ============================================================================

class SPSXSeqTrackCond : public SPSXSeqTrackBase {
public:
    uint8_t iterations[8];
    uint16_t cur_event_idx;
    uint8_t ignore_step;

    bool prev_trig_fired;
    bool first_run;

    // Pointer to owning MCLSeq (for shared state: neighbor_trig_mask, fill_mask)
    MCLSeq* seq_class = nullptr;

    SPSXSeqTrackCond() : SPSXSeqTrackBase() {
        reset();
    }

    void reset() {
        cur_event_idx = 0;
        for (uint8_t i = 0; i < 8; i++) {
            iterations[i] = 1;
        }
        prev_trig_fired = false;
        first_run = true;
        SPSXSeqTrackBase::reset();
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

    // Virtual methods for track editing
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
// SPSXSeqSlideTrack - Track with Parameter Slide/Glide
// ============================================================================

class SPSXSeqSlideTrack : public SPSXSeqTrackCond {
public:
    SPSXSlideData locks_slide_data[SPSX_NUM_LOCKS];
    uint8_t locks_slide_next_lock_val[SPSX_NUM_LOCKS];
    uint8_t locks_slide_next_lock_step[SPSX_NUM_LOCKS];
    uint8_t locks_slides_recalc = 255;
    uint16_t locks_slides_idx = 0;

    SPSXSeqSlideTrack() : SPSXSeqTrackCond() {}

    void reset() {
        for (uint8_t n = 0; n < SPSX_NUM_LOCKS; n++) {
            locks_slide_data[n].init();
        }
        SPSXSeqTrackCond::reset();
    }

    void prepare_slide(uint8_t lock_idx, int16_t x0, int16_t x1, int8_t y0, int8_t y1);
    virtual void send_slides(volatile uint8_t *locks_params, uint8_t channel = 0);

protected:
    virtual void on_slide_dispatch_begin(uint8_t channel);
    virtual void dispatch_slide_value(uint8_t param, uint8_t value, uint8_t channel);
    virtual void on_slide_dispatch_end();
};

// ============================================================================
// TrigNotes - Note state for MID machines
// ============================================================================

constexpr uint8_t spsx_number_midi_cc = 6 * 2 + 4;  // 16 total
constexpr uint8_t spsx_midi_cc_array_size = 6 * 2 + 4;

class SPSXTrigNotes {
public:
    uint8_t note1;
    uint8_t note2;
    uint8_t note3;
    uint8_t len;
    uint8_t vel;
    uint8_t ccs[spsx_midi_cc_array_size];
    uint16_t count_down;
    bool first_trig = false;

    void init() {
        note1 = 255;
        note2 = 64;
        note3 = 64;
        len = 0;
        vel = 100;
        memset(ccs, 255, sizeof(ccs));
        count_down = 0;
        first_trig = true;
    }
};

// ============================================================================
// Retrig State
// ============================================================================

struct spsx_retrig_state_t {
    uint8_t remaining;
    uint16_t tick_interval;
    uint16_t ticks_countdown;
    bool env_reset;
    uint8_t velocity;

    void clear() {
        remaining = 0;
        tick_interval = 0;
        ticks_countdown = 0;
        env_reset = true;
        velocity = 0;
    }
};

// ============================================================================
// SPSXSeqTrack - Full SPSX Sequencer Track
// ============================================================================

class SPSXSeqTrack : public SPSXSeqTrackData, public SPSXSeqSlideTrack {
public:
    uint64_t oneshot_mask;
    uint64_t mute_mask;

    SPSXTrigNotes notes;
    spsx_retrig_state_t retrig;

    SPSXSeqTrack() : SPSXSeqSlideTrack() {
        active = SPSX_TRACK_TYPE;
        oneshot_mask = 0;
        mute_mask = 0;
        notes.init();
        retrig.clear();
    }

    void reset() {
        SPSXSeqSlideTrack::reset();
        oneshot_mask = 0;
        record_mutes = false;
        send_notes_off();
        retrig.clear();
    }

    // ========================================================================
    // Core Sequencer
    // ========================================================================

    void seq(MidiUartClass *uart_, MidiUartClass *uart2_);
    void pre_seq(MidiUartClass *port_);
    void post_seq(MidiUartClass *port_);
    void load_cache();

    // ========================================================================
    // Mute Control
    // ========================================================================

    void mute() { mute_state = SPSX_MUTE_ON; }
    void unmute() { mute_state = SPSX_MUTE_OFF; }

    // ========================================================================
    // Trigger Methods
    // ========================================================================

    void send_trig();
    void send_trig_inline(uint8_t velocity = 127);
    uint8_t trig_conditional(uint8_t condition);

    void schedule_retrig(uint8_t velocity, uint8_t rtrg, uint8_t rtim, uint8_t renv);
    void fire_subtrig();

    // ========================================================================
    // Parameter Lock Methods
    // ========================================================================

    void send_parameter_locks(uint8_t step, bool trig, uint16_t lock_idx = 0xFFFF);
    void send_parameter_locks_inline(uint8_t step, bool trig, uint16_t lock_idx);
    void reset_params();
    void get_step_locks(uint8_t step, uint8_t *params, bool ignore_locks_disabled = false);

    // ========================================================================
    // MID Machine Methods
    // ========================================================================

    bool is_midi_model() const;
    uint8_t get_midi_channel() const;

    void init_notes();
    void process_note_locks(uint8_t param, uint8_t val, uint8_t *ccs);
    void send_notes_ccs(uint8_t *ccs, bool send_ccs);
    void send_notes(uint8_t first_note = 255);
    void send_notes_on();
    void send_notes_off();

    // ========================================================================
    // Slide Methods
    // ========================================================================

    void recalc_slides();
    void find_next_locks(uint8_t curidx, uint8_t step, uint64_t &mask);
    void send_slides(volatile uint8_t *locks_params, uint8_t channel = 0) override;

    // ========================================================================
    // Step Accessors
    // ========================================================================

    void get_mask(uint64_t *_pmask, uint8_t mask_type) const;
    bool get_step(uint8_t step, uint8_t mask_type) const;
    void set_step(uint8_t step, uint8_t mask_type, bool val);

    // ========================================================================
    // Lock Management
    // ========================================================================

    void set_track_pitch(uint8_t step, uint8_t pitch);
    void set_track_step(uint8_t step, int8_t microtiming_val, uint8_t velocity = 127);
    bool set_track_locks_i(uint8_t step, uint8_t lockidx, uint8_t velocity);
    bool set_track_locks(uint8_t step, uint8_t track_param, uint8_t velocity);
    uint8_t get_track_lock(uint8_t step, uint8_t lockidx);
    uint8_t get_track_lock_implicit(uint8_t step, uint8_t param);

    // ========================================================================
    // Recording
    // ========================================================================

    void record_track(uint8_t velocity);
    void record_track_locks(uint8_t track_param, uint8_t value);
    void record_track_pitch(uint8_t pitch);

    // ========================================================================
    // Clear/Reset
    // ========================================================================

    void clear_mute();
    void clear_mutes();
    void clear_slide_data();
    void clear_step(uint8_t step);
    void disable_step_locks(uint8_t step);
    void enable_step_locks(uint8_t step);
    uint64_t get_step_locks_mask(uint8_t step);
    void clear_conditional();
    void clear_step_lock(uint8_t step, uint8_t param_id);
    void clear_locks();
    void clear_track(bool locks = true) override;
    void clear_param_locks(uint8_t param_id);
    bool is_param(uint8_t param_id);

    // ========================================================================
    // Track Editing
    // ========================================================================

    void set_length(uint8_t len, bool expand = false) override;
    void rotate_left() override { modify_track(SPSX_DIR_LEFT); }
    void rotate_right() override { modify_track(SPSX_DIR_RIGHT); }
    void reverse() override { modify_track(SPSX_DIR_REVERSE); }
    void modify_track(uint8_t dir);

    void set_speed(uint8_t new_speed, uint8_t old_speed = 255, bool timing_adjust = true);
    void store_mute_state();

    void copy_step(uint8_t n, SPSXSeqStep *step);
    void paste_step(uint8_t n, SPSXSeqStep *step, const uint8_t *source_locks_params = nullptr);
    void preview_step(uint8_t step);

    // ========================================================================
    // Transpose
    // ========================================================================

    void transpose(int8_t offset) override;

    // ========================================================================
    // Pattern Import
    // ========================================================================

    void merge_from_md(uint8_t track_number, MDPattern *pattern);

protected:
    void dispatch_slide_value(uint8_t param, uint8_t value, uint8_t channel) override;

private:
    struct SlideDispatchContext {
        bool is_midi_model;
        bool send_ccs;
        uint8_t *ccs;
    };
    SlideDispatchContext *slide_ctx = nullptr;
};

#endif // !defined(__AVR__)
#endif // SPSX_SEQ_TRACK_H__
