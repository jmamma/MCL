#ifndef SPSX_SEQ_TRACK_H__
#define SPSX_SEQ_TRACK_H__

#if !defined(__AVR__)

#include "SPSXSeqDefines.h"
#include "SPSXSeqTrackData.h"
#include "StepSeqTrack.h"
#include <cstring>

// Forward declarations
class MCLSeq;
class MDPattern;

// Track type for SPSX tracks is defined as MDSPSX_TRACK_TYPE in GridTrack.h
#include "GridTrack.h"

using SPSXSlideData = StepSeqSlideData;
using SPSXSeqTrackBase = StepSeqTrack;
using SPSXSeqTrackCond = StepSeqTrackCond;
using SPSXSeqSlideTrack = BasicStepSeqSlideTrack<SPSX_NUM_LOCKS>;
using SPSXSeqDataTrack = BasicStepSeqDataTrack<SPSX_NUM_LOCKS>;

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

class SPSXSeqTrack : public SPSXSeqDataTrack {
public:
    uint64_t oneshot_mask;

    SPSXTrigNotes notes;
    spsx_retrig_state_t retrig;

    SPSXSeqTrack() : SPSXSeqDataTrack() {
        active = MDSPSX_TRACK_TYPE;
        oneshot_mask = 0;
        notes.init();
        retrig.clear();
    }

    void reset() {
        SPSXSeqDataTrack::reset();
        oneshot_mask = 0;
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
    uint8_t trig_conditional(uint8_t step, uint8_t condition);

    void schedule_retrig(uint8_t velocity, uint8_t rtrg, uint8_t rtim, uint8_t renv);
    void fire_subtrig();

    // ========================================================================
    // Parameter Lock Methods
    // ========================================================================

    void send_parameter_locks(uint8_t step, bool trig, uint16_t lock_idx = 0xFFFF);
    void send_parameter_locks_inline(uint8_t step, bool trig, uint16_t lock_idx);
    void reset_params();

    // ========================================================================
    // MID Machine Methods
    // ========================================================================

    bool is_midi_model() const;
    uint8_t get_midi_channel() const;

    void init_notes();
    void process_note_locks(uint8_t param, uint8_t val, uint8_t *ccs);
    void send_notes_ccs(uint8_t *ccs, bool send_ccs);
    void send_notes(uint8_t first_note = 255,
                    MidiUartClass *uart2_ = nullptr);
    void send_notes_on(MidiUartClass *uart2_ = nullptr);
    void send_notes_off(MidiUartClass *uart2_ = nullptr);
    void onControlChangeCallback_Midi(uint8_t track_param, uint8_t value);

    // ========================================================================
    // Slide Methods
    // ========================================================================

    void send_slides(volatile uint8_t *locks_params, uint8_t channel = 0) override;

    // ========================================================================
    // Clear/Reset
    // ========================================================================

    void clear_mutes() override;
    void clear_oneshot();

    bool preview_step(uint8_t step) override;

    // ========================================================================
    // Pattern Import
    // ========================================================================

    void merge_from_md(uint8_t track_number, MDPattern *pattern);

protected:
    bool get_default_lock_value(uint8_t param_id, uint8_t &value) const override;
    uint8_t velocity_lock_param() const override;
    void clear_step_oneshot(uint8_t step) override;
    void on_modify_track_begin() override;
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
