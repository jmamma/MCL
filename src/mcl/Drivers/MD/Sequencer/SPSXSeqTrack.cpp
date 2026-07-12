#if !defined(__AVR__)

#include "SPSXSeqTrack.h"
#include "SPSXTrack.h"
#include "MD.h"
#include "MDPattern.h"
#include "MDParams.h"
#include "Sequencer/MCLSeq.h"
#include "MDSeqTrack.h"
#include "MidiClock.h"
#include "MidiUart.h"
#include "GUI/Pages/CommonPages.h"
#include "MCLSysConfig.h"
#include "GUI/Pages/Performance/MixerPage.h"
#include "Sequencer/SeqTrackTransition.h"

// SPSX tracks share MDSeqTrack::md_trig_mask and MDSeqTrack::gui_update statics

namespace {

uint8_t spsx_swing_q14_to_amount(uint32_t swing_q14) {
    uint32_t amount = (swing_q14 * 50UL + 8192UL) >> 14;
    return amount > 30 ? 30 : (uint8_t)amount;
}

} // namespace

// ============================================================================
// SPSXSeqTrack — MID Machine Helpers
// ============================================================================

bool SPSXSeqTrack::is_midi_model() const {
    return (MD.kit.models[track_number] & 0xF0) == MID_01_MODEL;
}

uint8_t SPSXSeqTrack::get_midi_channel() const {
    return MD.kit.models[track_number] - MID_01_MODEL;
}

void SPSXSeqTrack::init_notes() {
    memcpy(&notes.note1, MD.kit.params[track_number], 5);
    notes.count_down = 0;
}

// ============================================================================
// SPSXSeqTrack — Core Sequencer
// ============================================================================

void SPSXSeqTrack::seq(MidiUartClass *uart_, MidiUartClass *uart2_) {
    MidiUartClass *port_old = port;
    MidiUartClass *port2_old = port2;

    port = uart_;
    port2 = uart2_;

    uint16_t tps = get_ticks_per_step();
    bool retrig_scheduled_this_call = false;

    tick_counter++;

    if (tick_counter >= tps) {
        tick_counter = 0;
        cur_event_idx += spsx_popcount(steps[step_count].locks);
        if (ignore_step == step_count) {
            ignore_step = 255;
        }
        step_count_inc();
        apply_pending_swing_amount();
    }
    update_legacy_progress_counter(tps);

    // MID machine note-off countdown
    if (notes.count_down) {
        notes.count_down--;
        if (notes.count_down == 0) {
            send_notes_off();
        }
    }

    if (count_down) {
        count_down--;
        if (count_down == 0) {
            reset();
            tick_counter = 0;
            mod12_counter = 0;
            SPSX_SET_BIT16(MDSeqTrack::gui_update, track_number);
        } else if (SeqTrackTransition::in_cache_window(
                       SEQ_TRANSITION_CACHE_MD_MACHINE, count_down,
                       track_number)) {
            if (!cache_loaded) {
                load_cache();
                cache_loaded = true;
            }
            goto end;
        }
    }

    if (record_mutes) {
        int8_t m = 0;
        uint8_t q = 0;
        uint8_t s = get_quantized_step(m, q);
        SPSX_SET_BIT64(mute_mask, s);
    }

    if ((mute_state == SPSX_MUTE_OFF) && (ignore_step != step_count)) {
        uint8_t next_step = (step_count == (length - 1)) ? 0 : step_count + 1;
        uint8_t current_step = 255;

        // Send active slides
        send_slides(locks_params);

        // Microtiming trigger logic (signed, center=0)
        int8_t mt_current = microtiming[step_count];

        // Current step: fires at tick_offset + 1 (positive or zero microtiming)
        if (mt_current >= 0) {
            int16_t tick_offset = effective_timing_offset(step_count, tps);
            if (tick_counter == (uint16_t)(tick_offset + 1)) {
                current_step = step_count;
            }
        }

        // Next step triggers early (negative microtiming).
        // At very high speeds (e.g. tps=12 at 8x) and large negative microtiming,
        // tps + microtiming_to_ticks(mt) can underflow to 0 or below, which would
        // fire the trigger immediately on every tick. Clamp to >= 1.
        int8_t mt_next = microtiming[next_step];
        if (mt_next < 0 && current_step == 255) {
            int16_t trigger_tick = (int16_t)(tps + spsx_microtiming_to_ticks(mt_next, tps));
            if (trigger_tick < 1) trigger_tick = 1;
            if (tick_counter == (uint16_t)trigger_tick) {
                current_step = next_step;
            }
        }

        if (current_step != 255) {
            uint16_t lock_idx = cur_event_idx;
            if (current_step == next_step) {
                if (current_step == 0) {
                    lock_idx = 0;
                } else {
                    lock_idx += spsx_popcount(steps[step_count].locks);
                }
            }

            auto &step = steps[current_step];
            uint8_t send_trig_result =
                trig_conditional(current_step, step.cond_id);

            bool midi_model = is_midi_model();
            bool step_has_trig = SPSX_IS_BIT_SET64(trig_mask, current_step);
            bool step_has_slide = SPSX_IS_BIT_SET64(slide_mask, current_step);

            if (send_trig_result == SPSX_TRIG_TRUE ||
                (!step.cond_plock && send_trig_result != SPSX_TRIG_ONESHOT)) {

                if (midi_model && send_trig_result == SPSX_TRIG_TRUE && step_has_trig) {
                    send_notes_off();
                    init_notes();
                }

                send_parameter_locks_inline(current_step, step_has_trig, lock_idx);

                if (step_has_slide) {
                    locks_slides_recalc = current_step;
                    locks_slides_idx = lock_idx;
                }

                if (send_trig_result == SPSX_TRIG_TRUE && step_has_trig) {
                    uint8_t velocity = SPSX_IS_BIT_SET64(accent_mask, current_step) ? 0x7F : 0x60;

                    if (midi_model) {
                        notes.count_down = notes.len == 0 ? tps / 4 : (notes.len * tps / 2);
                        send_notes_on();
                    }
                    send_trig_inline(velocity);

                    // Schedule retrig from kit params + lock overlay
                    uint8_t rtrg = MD.kit.params[track_number][MODEL_RTRG];
                    uint8_t rtim = MD.kit.params[track_number][MODEL_RTIM];
                    uint8_t renv = MD.kit.params[track_number][MODEL_RENV];

                    uint8_t v;
                    v = get_track_lock_implicit(current_step, MODEL_RTRG);
                    if (v != 255) rtrg = v;
                    v = get_track_lock_implicit(current_step, MODEL_RTIM);
                    if (v != 255) rtim = v;
                    v = get_track_lock_implicit(current_step, MODEL_RENV);
                    if (v != 255) renv = v;

                    if (rtrg > 0) {
                        schedule_retrig(velocity, rtrg, rtim, renv);
                        retrig_scheduled_this_call = true;
                    } else {
                        retrig.clear();
                    }
                }
            }
            record_trig_result(send_trig_result == SPSX_TRIG_TRUE &&
                               step_has_trig);
        }
    }

    // Process retrig countdown
    if (!retrig_scheduled_this_call && retrig.remaining > 0) {
        if (retrig.ticks_countdown > 0) {
            retrig.ticks_countdown--;
        }
        if (retrig.ticks_countdown == 0) {
            fire_subtrig();
            if (retrig.remaining != SPSX_RETRIG_INFINITE)
                retrig.remaining--;
            if (retrig.remaining > 0) {
                // Re-read RTIM from kit (live value)
                uint8_t rtim = MD.kit.params[track_number][MODEL_RTIM];
                if (rtim >= SPSX_RTIM_COUNT) rtim = SPSX_RTIM_COUNT - 1;
                uint16_t base_ticks = SPSX_RTIM_TICKS_96PPQN[rtim];
                uint16_t interval = (uint16_t)(((uint32_t)base_ticks * SPSX_TICKS_PER_STEP_1X + 12) / 24);
                if (interval < 1) interval = 1;
                retrig.tick_interval = interval;
                retrig.ticks_countdown = retrig.tick_interval;
            }
        }
    }

end:
    port = port_old;
    port2 = port2_old;
}

void SPSXSeqTrack::pre_seq(MidiUartClass *port_) {
    MDSeqTrack::pre_seq(port_);
}

void SPSXSeqTrack::post_seq(MidiUartClass *port_) {
    MDSeqTrack::post_seq(port_);
}

void SPSXSeqTrack::load_cache() {
    SPSXTrack t;
    t.load_from_mem(track_number, MDSPSX_TRACK_TYPE);
    t.load_seq_data(static_cast<SeqTrack *>(this));
    if (load_sound) {
        MD.insertMachineInKit(track_number, &(t.machine), false);
        SET_BIT32(MDSeqTrack::load_machine_cache, track_number);
        load_sound = false;
    }
}

// ============================================================================
// Trigger Methods
// ============================================================================

void SPSXSeqTrack::send_trig() {
    send_trig_inline(127);
}

void SPSXSeqTrack::send_trig_inline(uint8_t velocity) {
#ifdef LFO_TRACKS
    mcl_seq.report_track_trig(DeviceIdx::Primary, track_number);
#endif
    mixer_page.trig(track_number);
    if (velocity >= 127) {
        SPSX_SET_BIT16(MDSeqTrack::md_trig_mask, track_number);
        return;
    }
    MD.triggerTrack(track_number, velocity, port);
}

void SPSXSeqTrack::schedule_retrig(uint8_t velocity, uint8_t rtrg, uint8_t rtim, uint8_t renv) {
    if (rtrg == 0) return;
    if (rtim >= SPSX_RTIM_COUNT) rtim = SPSX_RTIM_COUNT - 1;

    uint16_t base_ticks = SPSX_RTIM_TICKS_96PPQN[rtim];
    uint16_t interval = (uint16_t)(((uint32_t)base_ticks * SPSX_TICKS_PER_STEP_1X + 12) / 24);
    if (interval < 1) interval = 1;

    retrig.remaining = (rtrg >= 127) ? SPSX_RETRIG_INFINITE : rtrg;
    retrig.tick_interval = interval;
    retrig.ticks_countdown = interval;
    retrig.env_reset = (renv != 0);
    retrig.velocity = velocity;
}

void SPSXSeqTrack::fire_subtrig() {
    send_trig_inline(retrig.velocity);
}

uint8_t SPSXSeqTrack::trig_conditional(uint8_t step, uint8_t condition) {
    if (SPSX_IS_BIT_SET64(mute_mask, step)) {
        return SPSX_TRIG_ONESHOT;
    }

    if (condition == SPSX_COND_ONESHOT) {
        if (SPSX_IS_BIT_SET64(oneshot_mask, step)) {
            return SPSX_TRIG_ONESHOT;
        }
        SPSX_SET_BIT64(oneshot_mask, step);
        return SPSX_TRIG_TRUE;
    }

    return conditional(condition) ? SPSX_TRIG_TRUE : SPSX_TRIG_FALSE;
}

// ============================================================================
// MID Machine Note Methods
// ============================================================================

void SPSXSeqTrack::send_notes_on(MidiUartClass *uart2_) {
    MidiUartClass *out = uart2_ ? uart2_ : port2;
    if (!out) out = mcl_seq.secondary_output;
    if (!out) return;
    uint8_t channel = get_midi_channel();
    if (notes.note1 != 255) {
#ifdef LFO_TRACKS
        mcl_seq.report_track_trig(DeviceIdx::Primary, track_number);
#endif
        mixer_page.trig(track_number);
        out->sendNoteOn(channel, notes.note1, notes.vel);
        if (notes.note2 != 64) {
            out->sendNoteOn(channel, notes.note1 + notes.note2 - 64, notes.vel);
        }
        if (notes.note3 != 64) {
            out->sendNoteOn(channel, notes.note1 + notes.note3 - 64, notes.vel);
        }
    }
}

void SPSXSeqTrack::send_notes_off(MidiUartClass *uart2_) {
    MidiUartClass *out = uart2_ ? uart2_ : port2;
    if (!out) out = mcl_seq.secondary_output;
    if (!out) return;
    uint8_t channel = get_midi_channel();
    if (notes.note1 != 255) {
        out->sendNoteOff(channel, notes.note1);
        if (notes.note2 != 64) {
            out->sendNoteOff(channel, notes.note1 + notes.note2 - 64);
        }
        if (notes.note3 != 64) {
            out->sendNoteOff(channel, notes.note1 + notes.note3 - 64);
        }
        notes.note1 = 255;
    }
    notes.count_down = 0;
}

void SPSXSeqTrack::send_notes(uint8_t first_note, MidiUartClass *uart2_) {
    if (notes.count_down) send_notes_off(uart2_);
    init_notes();
    if (first_note != 255) notes.note1 = first_note;
    if (notes.first_trig) {
        reset_params();
        notes.first_trig = false;
    }
    uint16_t tps = get_ticks_per_step();
    notes.count_down = notes.len == 0 ? tps / 4 : (notes.len * tps / 2);
    send_notes_on(uart2_);
}

void SPSXSeqTrack::onControlChangeCallback_Midi(uint8_t track_param, uint8_t value) {
    if (!is_midi_model() || MD.encoder_interface) return;
    if (track_param <= 4 || track_param >= 21) return;
    // Skip CC destination params (even-numbered slots in 8..19 range)
    if (!(track_param & 1) && track_param > 7 && track_param < 20) return;

    uint8_t ccs[spsx_midi_cc_array_size];
    memset(ccs, 255, sizeof(ccs));
    process_note_locks(track_param, value, ccs);
    send_notes_ccs(ccs, true);
}

void SPSXSeqTrack::send_notes_ccs(uint8_t *ccs, bool send_ccs) {
    if (!send_ccs) return;
    MidiUartClass *out = port2 ? port2 : mcl_seq.secondary_output;
    if (!out) return;
    uint8_t channel = get_midi_channel();
    for (uint8_t n = 0; n < spsx_number_midi_cc; n++) {
        if (ccs[n] == 255) continue;
        switch (n) {
        case 1: out->sendPitchBend(channel, (int16_t)(ccs[1] << 7)); break;
        case 2: out->sendCC(channel, 0x01, ccs[2]); break;
        case 3: out->sendChannelPressure(channel, ccs[3]); break;
        case 0:
            notes.ccs[0] = ccs[0];
            out->sendProgramChange(channel, ccs[0]);
            break;
        default:
            if (!(n & 1)) continue;
            { uint8_t cc_dest = ccs[n - 1];
              if (cc_dest > 0 && cc_dest != 255) {
                uint8_t cc_val = ccs[n];
                if (cc_dest == 1) cc_dest = 0;
                out->sendCC(channel, cc_dest, cc_val);
                notes.ccs[n] = cc_val;
              }
            }
            break;
        }
    }
}

void SPSXSeqTrack::process_note_locks(uint8_t param, uint8_t val, uint8_t *ccs) {
    uint8_t i = param - 5;
    switch (param) {
    case 0: case 1: case 2: case 3: case 4:
        ((uint8_t *)&notes)[param] = val;
        break;
    case 5: case 6: case 7:
        ccs[i + 1] = val;
        break;
    case 20:
        ccs[0] = (notes.ccs[0] != val) ? val : 255;
        break;
    default:
        if (param >= 8 && param < 20) {
            i = param - 8 + 4;
            uint8_t j = i - 1;
            if ((param & 1) && ccs[j] == 255) {
                ccs[j] = MD.kit.params[track_number][param - 1];
            }
            if ((ccs[j] != notes.ccs[j] || notes.ccs[i] != val)) {
                ccs[i] = val;
            } else {
                ccs[i] = 255;
            }
        }
        break;
    }
}

void SPSXSeqTrack::reset_params() {
    if (!is_midi_model()) return;
    uint8_t ccs[spsx_midi_cc_array_size];
    memcpy(ccs + 1, &MD.kit.params[track_number][5], sizeof(ccs) - 1);
    ccs[0] = 255;
    send_notes_ccs(ccs, true);
}

bool SPSXSeqTrack::get_default_lock_value(uint8_t param_id,
                                          uint8_t &value) const {
    value = MD.kit.params[track_number][param_id];
    return true;
}

uint8_t SPSXSeqTrack::velocity_lock_param() const {
    return MODEL_VOL;
}

void SPSXSeqTrack::clear_step_oneshot(uint8_t step) {
    SPSX_CLEAR_BIT64(oneshot_mask, step);
}

void SPSXSeqTrack::on_modify_track_begin() {
    oneshot_mask = 0;
}

void SPSXSeqTrack::clear_mutes() {
    oneshot_mask = 0;
    SPSXSeqDataTrack::clear_mutes();
}

void SPSXSeqTrack::clear_oneshot() {
    oneshot_mask = 0;
}

// ============================================================================
// Parameter Lock Methods
// ============================================================================

void SPSXSeqTrack::send_parameter_locks(uint8_t step, bool trig, uint16_t lock_idx) {
    uint16_t idx = (lock_idx == 0xFFFF) ? get_lockidx(step) : lock_idx;
    send_parameter_locks_inline(step, trig, idx);
}

void SPSXSeqTrack::send_parameter_locks_inline(uint8_t step, bool trig, uint16_t lock_idx) {
    uint8_t ccs[spsx_midi_cc_array_size];
    bool send_ccs = false;
    bool midi_model = is_midi_model();

    if (midi_model) {
        if (notes.first_trig) {
            memcpy(ccs + 1, &MD.kit.params[track_number][5], sizeof(ccs) - 1);
            process_note_locks(20, MD.kit.params[track_number][20], ccs);
            send_ccs = true;
            notes.first_trig = false;
        } else {
            memset(ccs, 255, sizeof(ccs));
        }
    }

    for (uint8_t c = 0; c < SPSX_NUM_LOCKS; c++) {
        bool lock_bit = steps[step].is_lock_bit(c);
        bool lock_present = steps[step].is_lock(c);
        bool send = false;
        uint8_t val = 0;
        uint8_t p = locks_params[c] - 1;

        if (locks_params[c]) {
            if (lock_present) {
                val = locks[lock_idx];
                send = true;
            } else if (trig) {
                val = MD.kit.params[track_number][p];
                send = true;
            }
        }
        lock_idx += lock_bit;

        if (send) {
            if (midi_model && p < 21) {
                process_note_locks(p, val, ccs);
                send_ccs |= (p > 4 && p < 8) || ((p > 8) && (p & 1)) || (p == 20);
            } else {
                MD.setTrackParam(track_number, p, val, port, false);
            }
        }
    }

    if (midi_model) {
        send_notes_ccs(ccs, send_ccs);
    }
}

// ============================================================================
// Slide Methods
// ============================================================================

void SPSXSeqTrack::dispatch_slide_value(uint8_t param, uint8_t value, uint8_t channel) {
    (void)channel;
    if (slide_ctx == nullptr) {
        MD.setTrackParam(track_number, param, value, port, false);
        return;
    }
    if (!slide_ctx->is_midi_model) {
        MD.setTrackParam(track_number, param, value, port, false);
        return;
    }
    slide_ctx->send_ccs |= ((param > 4 && param < 8) ||
                            ((param > 8) && (param & 1)) || (param == 20));
    process_note_locks(param, value, slide_ctx->ccs);
}

void SPSXSeqTrack::send_slides(volatile uint8_t *locks_params_arg, uint8_t channel) {
    (void)channel;
    SlideDispatchContext ctx{};
    ctx.is_midi_model = is_midi_model();

    uint8_t ccs[spsx_midi_cc_array_size];
    if (ctx.is_midi_model) {
        ctx.ccs = ccs;
        ctx.send_ccs = false;
        memset(ccs, 255, sizeof(ccs));
    } else {
        ctx.ccs = nullptr;
    }

    slide_ctx = &ctx;
    SPSXSeqSlideTrack::send_slides(locks_params_arg, track_number);
    slide_ctx = nullptr;

    if (ctx.is_midi_model) {
        send_notes_ccs(ccs, ctx.send_ccs);
    }
}

// ============================================================================
// Preview
// ============================================================================

bool SPSXSeqTrack::preview_step(uint8_t step) {
    if (step >= length) return false;

    // Send all lock values for this step (kit defaults overlaid with plocks)
    uint16_t lock_idx = get_lockidx(step);
    for (uint8_t c = 0; c < SPSX_NUM_LOCKS; c++) {
        bool lock_bit = steps[step].is_lock_bit(c);
        if (lock_bit && locks_params[c]) {
            uint8_t p = locks_params[c] - 1;
            MD.setTrackParam(track_number, p, locks[lock_idx]);
        }
        lock_idx += lock_bit;
    }

    // Handle MID machines
    bool midi_model = is_midi_model();
    if (midi_model) {
        send_notes_off();
        init_notes();
        uint16_t tps = get_ticks_per_step();
        notes.count_down = notes.len == 0 ? tps / 4 : (notes.len * tps / 2);
        send_notes_on();
    }

    // Trigger the track
    uint8_t velocity = SPSX_IS_BIT_SET64(accent_mask, step) ? 0x7F : 0x60;
    MD.triggerTrack(track_number, velocity);
#ifdef LFO_TRACKS
    mcl_seq.report_track_trig(DeviceIdx::Primary, track_number);
#endif
    mixer_page.trig(track_number);
    return true;
}

// ============================================================================
// Pattern Import
// ============================================================================

void SPSXSeqTrack::merge_from_md(uint8_t trk, MDPattern *pattern) {
    // pattern arrays are sized 16; out-of-range trk is the 0xFF
    // "no track" sentinel that crashes on dereference.
    if (trk >= 16 || pattern == nullptr) return;
    // Build trig/slide/accent/swing masks from pattern
    trig_mask = pattern->trigPatterns[trk];

    if (pattern->slideEditAll > 0) {
        slide_mask = pattern->slidePattern;
    } else {
        slide_mask = pattern->slidePatterns[trk];
    }

    if (pattern->accentEditAll > 0) {
        accent_mask = pattern->accentPattern;
    } else {
        accent_mask = pattern->accentPatterns[trk];
    }

    if (pattern->swingEditAll > 0) {
        swing_mask = pattern->swingPattern;
    } else {
        swing_mask = pattern->swingPatterns[trk];
    }
    swing_amount = spsx_swing_q14_to_amount(pattern->swingAmount);

    // Use pattern length as default, SPSX extension overrides below
    if (pattern->patternLength > 0 && pattern->patternLength <= 64) {
        length = pattern->patternLength;
        track_length = length;
    }

    // Import lock values from pattern rows. Row indices are int16_t now and
    // can exceed 64; use lock_row() to handle the extended-rows extension array.
    for (uint8_t p = 0; p < pattern->maxParams; p++) {
        int16_t row = pattern->paramLocks[trk][p];
        if (row < 0) continue;

        const int8_t *row_data = pattern->lock_row((uint16_t)row);
        for (uint8_t s = 0; s < 64; s++) {
            int8_t lockval = row_data[s];
            if (lockval >= 0 && SPSX_IS_BIT_SET64(trig_mask, s)) {
                set_track_locks(s, p, lockval);
            }
        }
    }

    // SPSX extension data (overrides V3 defaults above)
    if (pattern->version >= 0x40) {
        // Direct copy: microtiming (both are signed int8_t).
        // Native swing is kept separate and only applies to neutral steps.
        memcpy(microtiming, pattern->ext_microtiming[trk], 64);

        // Unpack step flags
        for (uint8_t s = 0; s < 64; s++) {
            uint8_t flags = pattern->ext_step_flags[trk][s];
            steps[s].cond_id = flags >> 2;
            steps[s].cond_plock = (flags >> 1) & 1;
        }

        // Copy full lock-params mapping. MD_PATTERN_LOCK_SLOTS is sized to match
        // host NUM_LOCKS (== SPSX_NUM_LOCKS), so this is a 1:1 copy.
        static_assert(MD_PATTERN_LOCK_SLOTS == SPSX_NUM_LOCKS,
                      "ext_locks_params width must match SPSX track lock slots");
        for (uint8_t i = 0; i < SPSX_NUM_LOCKS; i++) {
            locks_params[i] = pattern->ext_locks_params[trk][i];
        }

        // Per-track length/speed override pattern-level defaults
        if (pattern->ext_track_lengths[trk] != 0) {
            length = pattern->ext_track_lengths[trk];
            track_length = length;
        }
        if (pattern->ext_track_speeds[trk] != 0xFF) {
            speed = pattern->ext_track_speeds[trk];
            track_speed = speed;
        }
    }
}

#endif // !defined(__AVR__)
