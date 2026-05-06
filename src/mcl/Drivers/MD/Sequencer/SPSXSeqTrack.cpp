#if !defined(__AVR__)

#include "SPSXSeqTrack.h"
#include "SPSXTrack.h"
#include "MD.h"
#include "MDPattern.h"
#include "MDParams.h"
#include "MCLSeq.h"
#include "MDSeqTrack.h"
#include "MidiClock.h"
#include "MidiUart.h"
#include "AuxPages.h"
#include "MCLSysConfig.h"

// SPSX tracks share MDSeqTrack::md_trig_mask and MDSeqTrack::gui_update statics

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

    tick_counter++;

    if (tick_counter >= tps) {
        tick_counter = 0;
        cur_event_idx += spsx_popcount(steps[step_count].locks);
        if (ignore_step == step_count) {
            ignore_step = 255;
        }
        step_count_inc();
    }

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
            SPSX_SET_BIT16(MDSeqTrack::gui_update, track_number);
        } else if (count_down <= track_number / 4 + 1) {
            if (!cache_loaded) {
                load_cache();
                cache_loaded = true;
            }
        }
        goto end;
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
            int16_t tick_offset = spsx_microtiming_to_ticks(mt_current, tps);
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
            uint8_t send_trig_result = trig_conditional(step.cond_id);

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
                    } else {
                        retrig.clear();
                    }
                }
            }
        }
    }

    // Process retrig countdown
    if (retrig.remaining > 0) {
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

uint8_t SPSXSeqTrack::trig_conditional(uint8_t condition) {
    if (SPSX_IS_BIT_SET64(oneshot_mask, step_count) ||
        SPSX_IS_BIT_SET64(mute_mask, step_count)) {
        record_trig_result(false);
        return SPSX_TRIG_ONESHOT;
    }

    bool result;
    if (condition == SPSX_COND_ONESHOT) {
        if (!SPSX_IS_BIT_SET64(oneshot_mask, step_count)) {
            SPSX_SET_BIT64(oneshot_mask, step_count);
            result = true;
        } else {
            result = false;
        }
    } else {
        result = conditional(condition);
    }

    record_trig_result(result);
    return result ? SPSX_TRIG_TRUE : SPSX_TRIG_FALSE;
}

// ============================================================================
// MID Machine Note Methods
// ============================================================================

void SPSXSeqTrack::send_notes_on() {
    if (!port2) return;
    uint8_t channel = get_midi_channel();
    if (notes.note1 != 255) {
        port2->sendNoteOn(channel, notes.note1, notes.vel);
        if (notes.note2 != 64) {
            port2->sendNoteOn(channel, notes.note1 + notes.note2 - 64, notes.vel);
        }
        if (notes.note3 != 64) {
            port2->sendNoteOn(channel, notes.note1 + notes.note3 - 64, notes.vel);
        }
    }
}

void SPSXSeqTrack::send_notes_off() {
    if (!port2) return;
    uint8_t channel = get_midi_channel();
    if (notes.note1 != 255) {
        port2->sendNoteOff(channel, notes.note1);
        if (notes.note2 != 64) {
            port2->sendNoteOff(channel, notes.note1 + notes.note2 - 64);
        }
        if (notes.note3 != 64) {
            port2->sendNoteOff(channel, notes.note1 + notes.note3 - 64);
        }
        notes.note1 = 255;
    }
    notes.count_down = 0;
}

void SPSXSeqTrack::send_notes(uint8_t first_note) {
    if (notes.count_down) send_notes_off();
    init_notes();
    if (first_note != 255) notes.note1 = first_note;
    if (notes.first_trig) {
        reset_params();
        notes.first_trig = false;
    }
    uint16_t tps = get_ticks_per_step();
    notes.count_down = notes.len == 0 ? tps / 4 : (notes.len * tps / 2);
    send_notes_on();
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
    if (!port2 || !send_ccs) return;
    uint8_t channel = get_midi_channel();
    for (uint8_t n = 0; n < spsx_number_midi_cc; n++) {
        if (ccs[n] == 255) continue;
        switch (n) {
        case 1: port2->sendPitchBend(channel, (int16_t)(ccs[1] << 7)); break;
        case 2: port2->sendCC(channel, 0x01, ccs[2]); break;
        case 3: port2->sendChannelPressure(channel, ccs[3]); break;
        case 0:
            notes.ccs[0] = ccs[0];
            port2->sendProgramChange(channel, ccs[0]);
            break;
        default:
            if (!(n & 1)) continue;
            { uint8_t cc_dest = ccs[n - 1];
              if (cc_dest > 0 && cc_dest != 255) {
                uint8_t cc_val = ccs[n];
                if (cc_dest == 1) cc_dest = 0;
                port2->sendCC(channel, cc_dest, cc_val);
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

void SPSXSeqTrack::get_step_locks(uint8_t step, uint8_t *params, bool ignore_locks_enabled) {
    uint16_t lock_idx = get_lockidx(step);
    for (uint8_t c = 0; c < SPSX_NUM_LOCKS; c++) {
        bool lock_bit = steps[step].is_lock_bit(c);
        bool lock_present = lock_bit & (steps[step].locks_enabled || ignore_locks_enabled);
        if (locks_params[c]) {
            uint8_t param = locks_params[c] - 1;
            if (lock_present) {
                params[param] = locks[lock_idx];
            }
        }
        lock_idx += lock_bit;
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

void SPSXSeqTrack::recalc_slides() {
    if (locks_slides_recalc == 255) return;

    int16_t x0, x1;
    int8_t y0, y1;
    uint8_t step = locks_slides_recalc;
    uint16_t tps = get_ticks_per_step();

    uint64_t find_mask = 0;
    uint64_t cur_mask = 1ULL;
    for (uint8_t i = 0; i < SPSX_NUM_LOCKS; i++) {
        if (locks_params[i] && (steps[step].locks & cur_mask)) {
            find_mask |= cur_mask;
        }
        cur_mask <<= 1;
    }

    auto lockidx = locks_slides_idx;
    if (find_mask == 0) goto end;

    find_next_locks((uint8_t)lockidx, step, find_mask);

    for (uint8_t c = 0; c < SPSX_NUM_LOCKS; c++) {
        if (!locks_params[c] || !steps[step].is_lock_bit(c)) continue;

        auto cur_lockidx = lockidx++;
        if (!steps[step].locks_enabled) continue;

        if (find_mask & (1ULL << c)) {
            locks_slide_data[c].init();
            continue;
        }

        auto next_lockstep = locks_slide_next_lock_step[c];
        if (step == next_lockstep) {
            locks_slide_data[c].init();
            continue;
        }

        int16_t mt_offset = spsx_microtiming_to_ticks(microtiming[step], tps);
        int16_t mt_next_offset = spsx_microtiming_to_ticks(microtiming[next_lockstep], tps);

        x0 = (int16_t)(step * tps + mt_offset + 1);
        if (next_lockstep < step) {
            x1 = (int16_t)((length + next_lockstep) * tps + mt_next_offset - 1);
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

void SPSXSeqTrack::find_next_locks(uint8_t curidx, uint8_t step, uint64_t &mask) {
    uint8_t next_step = step + 1;
    uint8_t max_len = length;
    curidx += spsx_popcount(steps[step].locks);

again:
    for (; next_step < max_len; next_step++) {
        uint64_t cur_mask = 1ULL;
        auto raw_locks = steps[next_step].locks;
        auto lcks = steps[next_step].locks_enabled ? raw_locks : (uint64_t)0;
        bool next_step_has_trig = SPSX_IS_BIT_SET64(trig_mask, next_step);

        if (!raw_locks && !next_step_has_trig) continue;

        for (uint8_t i = 0; i < SPSX_NUM_LOCKS; ++i) {
            if (mask & cur_mask) {
                if (lcks & cur_mask) {
                    locks_slide_next_lock_val[i] = locks[curidx];
                    locks_slide_next_lock_step[i] = next_step;
                    mask &= ~cur_mask;
                } else if (next_step_has_trig) {
                    locks_slide_next_lock_val[i] = MD.kit.params[track_number][locks_params[i] - 1];
                    locks_slide_next_lock_step[i] = next_step;
                    mask &= ~cur_mask;
                }
                if (!mask) return;
            }
            if (raw_locks & cur_mask) curidx++;
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
// Step Accessors
// ============================================================================

void SPSXSeqTrack::get_mask(uint64_t *_pmask, uint8_t mask_type) const {
    switch (mask_type) {
    case SPSX_MASK_PATTERN: *_pmask = trig_mask; return;
    case SPSX_MASK_SLIDE:   *_pmask = slide_mask; return;
    case SPSX_MASK_MUTE:    *_pmask = mute_mask; return;
    default: break;
    }
    *_pmask = 0;
    for (uint8_t i = 0; i < SPSX_NUM_MD_STEPS; i++) {
        bool set_bit = false;
        switch (mask_type) {
        case SPSX_MASK_LOCKS_ON_STEP: if (steps[i].locks) set_bit = true; break;
        case SPSX_MASK_LOCK: if (steps[i].locks_enabled) set_bit = true; break;
        }
        if (set_bit) *_pmask |= (1ULL << i);
    }
}

bool SPSXSeqTrack::get_step(uint8_t step, uint8_t mask_type) const {
    switch (mask_type) {
    case SPSX_MASK_PATTERN: return SPSX_IS_BIT_SET64(trig_mask, step);
    case SPSX_MASK_LOCK:    return steps[step].locks_enabled;
    case SPSX_MASK_MUTE:    return SPSX_IS_BIT_SET64(mute_mask, step);
    case SPSX_MASK_SLIDE:   return SPSX_IS_BIT_SET64(slide_mask, step);
    default: return false;
    }
}

void SPSXSeqTrack::set_step(uint8_t step, uint8_t mask_type, bool val) {
    switch (mask_type) {
    case SPSX_MASK_PATTERN:
        if (val) SPSX_SET_BIT64(trig_mask, step); else SPSX_CLEAR_BIT64(trig_mask, step); break;
    case SPSX_MASK_LOCK:    steps[step].locks_enabled = val; break;
    case SPSX_MASK_MUTE:
        if (val) SPSX_SET_BIT64(mute_mask, step); else SPSX_CLEAR_BIT64(mute_mask, step); break;
    case SPSX_MASK_SLIDE:
        if (val) SPSX_SET_BIT64(slide_mask, step); else SPSX_CLEAR_BIT64(slide_mask, step); break;
    }
}

// ============================================================================
// Lock Management
// ============================================================================

uint8_t SPSXSeqTrack::get_track_lock_implicit(uint8_t step, uint8_t param) {
    uint8_t lock_idx = find_param(param);
    if (lock_idx < SPSX_NUM_LOCKS) return get_track_lock(step, lock_idx);
    return 255;
}

uint8_t SPSXSeqTrack::get_track_lock(uint8_t step, uint8_t lock_idx) {
    auto idx = get_lockidx(step, lock_idx);
    if (idx < SPSX_NUM_MD_LOCK_SLOTS && steps[step].locks_enabled) {
        return locks[idx];
    }
    return 255;
}

bool SPSXSeqTrack::set_track_locks(uint8_t step, uint8_t track_param, uint8_t value) {
    uint8_t match = find_param(track_param);
    for (uint8_t c = 0; c < SPSX_NUM_LOCKS && match == 255; c++) {
        if (locks_params[c] == 0) {
            locks_params[c] = track_param + 1;
            match = c;
        }
    }
    if (match != 255) return set_track_locks_i(step, match, value);
    return false;
}

bool SPSXSeqTrack::set_track_locks_i(uint8_t step, uint8_t lockidx, uint8_t value) {
    auto lock_slot = get_lockidx(step, lockidx);
    if (lock_slot == SPSX_NUM_MD_LOCK_SLOTS) {
        auto idx = get_lockidx(step);
        auto nlock = spsx_popcount(steps[step].locks & ((1ULL << lockidx) - 1));
        lock_slot = idx + nlock;
        if (lock_slot >= SPSX_NUM_MD_LOCK_SLOTS) return false;
        memmove(locks + lock_slot + 1, locks + lock_slot,
                SPSX_NUM_MD_LOCK_SLOTS - lock_slot - 1);
        if (step < step_count) cur_event_idx++;
        steps[step].locks |= (1ULL << lockidx);
    }
    locks[lock_slot] = (value > 127) ? 127 : value;
    steps[step].locks_enabled = true;
    return true;
}

void SPSXSeqTrack::set_track_pitch(uint8_t step, uint8_t pitch) {
    set_track_locks(step, 0, pitch);
}

void SPSXSeqTrack::set_track_step(uint8_t step, int8_t microtiming_val, uint8_t velocity) {
    SPSX_CLEAR_BIT64(oneshot_mask, step);
    SPSX_SET_BIT64(trig_mask, step);
    if (velocity >= 127) {
        SPSX_SET_BIT64(accent_mask, step);
    } else {
        SPSX_CLEAR_BIT64(accent_mask, step);
        set_track_locks(step, MODEL_VOL, velocity);
    }
    steps[step].cond_id = 0;
    steps[step].cond_plock = false;
    microtiming[step] = microtiming_val;
}

// ============================================================================
// Recording
// ============================================================================

void SPSXSeqTrack::record_track(uint8_t velocity) {
    if (step_count >= length) return;
    int8_t mt = 0;
    uint8_t step = get_quantized_step(mt);
    ignore_step = step;
    set_track_step(step, mt, velocity);
}

void SPSXSeqTrack::record_track_locks(uint8_t track_param, uint8_t value) {
    if (step_count >= length) return;
    int8_t mt = 0;
    set_track_locks(get_quantized_step(mt), track_param, value);
}

void SPSXSeqTrack::record_track_pitch(uint8_t pitch) {
    if (step_count >= length) return;
    int8_t mt = 0;
    set_track_pitch(get_quantized_step(mt), pitch);
}

// ============================================================================
// Clear/Reset Methods
// ============================================================================

void SPSXSeqTrack::clear_mute() { mute_mask = 0; }
void SPSXSeqTrack::clear_mutes() { oneshot_mask = 0; mute_mask = 0; }
void SPSXSeqTrack::clear_slide_data() { slide_mask = 0; }

void SPSXSeqTrack::clear_step(uint8_t step) {
    uint16_t idx16 = get_lockidx(step);
    uint8_t cnt = spsx_popcount(steps[step].locks);
    // Defensive: in normal state idx+cnt <= SPSX_NUM_MD_LOCK_SLOTS, but guard
    // against any corruption that would make memmove size underflow.
    if (cnt != 0 && idx16 + cnt <= SPSX_NUM_MD_LOCK_SLOTS) {
        uint8_t idx = (uint8_t)idx16;
        memmove(locks + idx, locks + idx + cnt, SPSX_NUM_MD_LOCK_SLOTS - idx - cnt);
        if (step < step_count) cur_event_idx -= cnt;
    }
    steps[step].locks = 0;
    steps[step].locks_enabled = false;
    steps[step].cond_id = 0;
    steps[step].cond_plock = 0;
    microtiming[step] = 0;
}

// Clear lock values on a step while preserving trig/condition/microtiming.
// SPSX equivalent of MDSeqTrack::clear_step_locks — used by the "clear locks"
// UI gesture that should leave the step's trigger and timing intact.
void SPSXSeqTrack::clear_step_locks(uint8_t step) {
    uint16_t idx16 = get_lockidx(step);
    uint8_t cnt = spsx_popcount(steps[step].locks);
    if (cnt == 0 || idx16 + cnt > SPSX_NUM_MD_LOCK_SLOTS) return;
    uint8_t idx = (uint8_t)idx16;
    memmove(locks + idx, locks + idx + cnt, SPSX_NUM_MD_LOCK_SLOTS - idx - cnt);
    if (step < step_count) cur_event_idx -= cnt;
    steps[step].locks = 0;
    steps[step].locks_enabled = false;
    clean_params();
}

void SPSXSeqTrack::disable_step_locks(uint8_t step) { steps[step].locks_enabled = false; }
void SPSXSeqTrack::enable_step_locks(uint8_t step) { steps[step].locks_enabled = true; }
uint64_t SPSXSeqTrack::get_step_locks_mask(uint8_t step) {
    return steps[step].locks_enabled ? steps[step].locks : 0;
}

void SPSXSeqTrack::clear_conditional() {
    for (uint8_t c = 0; c < SPSX_NUM_MD_STEPS; c++) {
        steps[c].cond_id = 0;
        steps[c].cond_plock = 0;
        microtiming[c] = 0;
    }
    clear_mutes();
    ignore_step = 255;
}

void SPSXSeqTrack::clear_step_lock(uint8_t step, uint8_t param_id) {
    uint8_t match = find_param(param_id);
    if (match == 255) return;
    uint64_t mask = (1ULL << match);
    uint16_t idx = get_lockidx(step);
    if (!(steps[step].locks & mask)) return;
    uint8_t offset = spsx_popcount(steps[step].locks & (mask - 1));
    memmove(locks + idx + offset, locks + idx + offset + 1,
            SPSX_NUM_MD_LOCK_SLOTS - idx - offset - 1);
    steps[step].locks &= ~mask;
    if (steps[step].locks == 0) steps[step].locks_enabled = false;
    locks_slide_data[match].init();
    cur_event_idx = get_lockidx(step_count);
    clean_params();
}

void SPSXSeqTrack::clear_locks() {
    for (uint8_t c = 0; c < SPSX_NUM_LOCKS; c++) locks_params[c] = 0;
    memset(locks, 0, sizeof(locks));
    cur_event_idx = 0;
}

void SPSXSeqTrack::clear_track(bool clear_locks_too) {
    clear_conditional();
    if (clear_locks_too) clear_locks();
    memset(steps, 0, sizeof(steps));
    trig_mask = 0;
    slide_mask = 0;
    accent_mask = 0;
    swing_mask = 0;
    memset(microtiming, 0, sizeof(microtiming));
}

void SPSXSeqTrack::clear_param_locks(uint8_t param_id) {
    uint8_t match = find_param(param_id);
    if (match == 255) return;

    uint64_t mask = 1ULL << match;
    uint64_t nmask = ~mask;
    uint64_t rmask = mask - 1;
    bool remove[SPSX_NUM_MD_STEPS];

    for (uint8_t x = 0; x < SPSX_NUM_MD_STEPS; x++) {
        remove[x] = (steps[x].locks & mask) != 0;
        steps[x].locks &= nmask;
        if (steps[x].locks == 0) steps[x].locks_enabled = false;
    }

    uint16_t rd = 0, wr = 0;
    for (uint8_t i = 0; i < SPSX_NUM_MD_STEPS; ++i) {
        uint64_t _locks = steps[i].locks;
        uint8_t nlocks = spsx_popcount(_locks);
        uint8_t total = remove[i] ? (uint8_t)(nlocks + 1) : nlocks;
        uint8_t skip = remove[i] ? spsx_popcount(_locks & rmask) : total;
        for (uint8_t j = 0; j < total; ++j) {
            if (skip == j) { ++rd; }
            else { locks[wr++] = locks[rd++]; }
        }
    }

    locks_slide_data[match].init();
    cur_event_idx = get_lockidx(step_count);
    clean_params();
}

bool SPSXSeqTrack::is_param(uint8_t param_id) {
    for (uint8_t c = 0; c < SPSX_NUM_LOCKS; c++) {
        if (locks_params[c] > 0 && locks_params[c] - 1 == param_id) return true;
    }
    return false;
}

// ============================================================================
// Track Editing
// ============================================================================

void SPSXSeqTrack::set_length(uint8_t len, bool expand) {
    uint8_t old_length = length;
    if (len == 0) len = 64;
    if (len > 64) len = 64;
    length = len;
    track_length = len;

    uint8_t step = step_count;
    if (step >= length) step = step % length;
    uint8_t idx = (uint8_t)get_lockidx(step);
    cur_event_idx = idx;
    step_count = step;

    if (expand && old_length > 0 && length > old_length) {
        bool has_existing_data = false;
        for (uint8_t n = old_length; n < length && n < 64; n++) {
            if (get_step(n, SPSX_MASK_PATTERN) || get_step(n, SPSX_MASK_LOCK) ||
                steps[n].locks != 0 || steps[n].locks_enabled) {
                has_existing_data = true;
                break;
            }
        }
        if (!has_existing_data) {
            SPSXSeqStep step_copy;
            uint8_t src_idx = 0;
            for (uint8_t n = old_length; n < length && n < 64; n++) {
                copy_step(src_idx, &step_copy);
                paste_step(n, &step_copy);
                microtiming[n] = microtiming[src_idx];
                src_idx++;
                if (src_idx >= old_length) src_idx = 0;
            }
            uint64_t old_trig = trig_mask & ((1ULL << old_length) - 1);
            uint64_t old_accent = accent_mask & ((1ULL << old_length) - 1);
            uint64_t old_swing = swing_mask & ((1ULL << old_length) - 1);
            uint64_t old_slide = slide_mask & ((1ULL << old_length) - 1);
            for (uint8_t page = 1; page * old_length < length && page < 4; page++) {
                trig_mask |= (old_trig << (page * old_length));
                accent_mask |= (old_accent << (page * old_length));
                swing_mask |= (old_swing << (page * old_length));
                slide_mask |= (old_slide << (page * old_length));
            }
        }
    }
}

void SPSXSeqTrack::set_speed(uint8_t new_speed, uint8_t old_speed, bool timing_adjust) {
    if (old_speed == 255) old_speed = speed;
    (void)timing_adjust; // microtiming is relative, no adjustment needed
    speed = new_speed;
    track_speed = new_speed;
    uint16_t tps = get_ticks_per_step();
    if (tick_counter > tps) tick_counter = tick_counter % tps;
}

void SPSXSeqTrack::store_mute_state() {
    for (uint8_t n = 0; n < SPSX_NUM_MD_STEPS; n++) {
        if (SPSX_IS_BIT_SET64(mute_mask, n)) {
            set_step(n, SPSX_MASK_PATTERN, false);
            set_step(n, SPSX_MASK_LOCK, false);
        }
    }
    clear_mutes();
}

void SPSXSeqTrack::modify_track(uint8_t dir) {
    uint8_t old_mute_state = mute_state;
    oneshot_mask = 0;

    constexpr size_t ncopy = sizeof(steps) - sizeof(SPSXSeqStepDescriptor);
    uint8_t lock_buf[SPSX_NUM_LOCKS];
    SPSXSeqStepDescriptor step_buf;
    int8_t mt_buf;
    uint16_t total_nlock = get_lockidx(length);

    mute_state = SPSX_MUTE_ON;

    switch (dir) {
    case SPSX_DIR_LEFT: {
        uint8_t nlock = spsx_popcount(steps[0].locks);
        memcpy(lock_buf, locks, nlock);
        memmove(locks, locks + nlock, total_nlock - nlock);
        memcpy(locks + total_nlock - nlock, lock_buf, nlock);

        step_buf = steps[0];
        mt_buf = microtiming[0];
        memmove(steps, steps + 1, ncopy);
        memmove(microtiming, microtiming + 1, length - 1);
        steps[length - 1] = step_buf;
        microtiming[length - 1] = mt_buf;
        SPSX_ROTATE_LEFT(mute_mask, length);
        SPSX_ROTATE_LEFT(trig_mask, length);
        SPSX_ROTATE_LEFT(slide_mask, length);
        SPSX_ROTATE_LEFT(accent_mask, length);
        SPSX_ROTATE_LEFT(swing_mask, length);
        break;
    }
    case SPSX_DIR_RIGHT: {
        uint8_t nlock = spsx_popcount(steps[length - 1].locks);
        memcpy(lock_buf, locks + total_nlock - nlock, nlock);
        memmove(locks + nlock, locks, total_nlock - nlock);
        memcpy(locks, lock_buf, nlock);

        step_buf = steps[length - 1];
        mt_buf = microtiming[length - 1];
        memmove(steps + 1, steps, ncopy);
        memmove(microtiming + 1, microtiming, length - 1);
        steps[0] = step_buf;
        microtiming[0] = mt_buf;
        SPSX_ROTATE_RIGHT(mute_mask, length);
        SPSX_ROTATE_RIGHT(trig_mask, length);
        SPSX_ROTATE_RIGHT(slide_mask, length);
        SPSX_ROTATE_RIGHT(accent_mask, length);
        SPSX_ROTATE_RIGHT(swing_mask, length);
        break;
    }
    case SPSX_DIR_REVERSE: {
        uint8_t rev_locks[SPSX_NUM_MD_LOCK_SLOTS];
        memcpy(rev_locks, locks, sizeof(locks));
        uint16_t l = 0, r = 0;

        for (uint8_t i = 0; i <= length / 2; ++i) {
            int j = length - i - 1;
            if (j < (int)i) break;

            uint8_t ni = spsx_popcount(steps[i].locks);
            uint8_t nj = spsx_popcount(steps[j].locks);
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

            bool a = SPSX_IS_BIT_SET64(mute_mask, i);
            bool b = SPSX_IS_BIT_SET64(mute_mask, j);
            if (a) SPSX_SET_BIT64(mute_mask, j); else SPSX_CLEAR_BIT64(mute_mask, j);
            if (b) SPSX_SET_BIT64(mute_mask, i); else SPSX_CLEAR_BIT64(mute_mask, i);
        }
        trig_mask = spsx_reverse_mask64(trig_mask, length);
        slide_mask = spsx_reverse_mask64(slide_mask, length);
        accent_mask = spsx_reverse_mask64(accent_mask, length);
        swing_mask = spsx_reverse_mask64(swing_mask, length);
        break;
    }
    }

    cur_event_idx = get_lockidx(step_count);
    mute_state = old_mute_state;
}

void SPSXSeqTrack::copy_step(uint8_t n, SPSXSeqStep *step) {
    step->active = true;
    step->microtiming = microtiming[n];
    uint16_t idx = get_lockidx(n);
    uint64_t lcks = steps[n].locks;
    uint64_t mask = 1ULL;
    for (uint8_t a = 0; a < SPSX_NUM_LOCKS; a++) {
        if (lcks & mask) {
            step->locks[a] = locks[idx++] + 1;
        } else {
            step->locks[a] = 0;
        }
        mask <<= 1;
    }
    memcpy(&step->data, &steps[n], sizeof(SPSXSeqStepDescriptor));
}

void SPSXSeqTrack::paste_step(uint8_t n, SPSXSeqStep *step, const uint8_t *source_locks_params) {
    clear_step(n);
    microtiming[n] = step->microtiming;
    const uint8_t *lp = source_locks_params ? source_locks_params : locks_params;
    for (uint8_t a = 0; a < SPSX_NUM_LOCKS; a++) {
        if (step->locks[a] != 0 && lp[a] != 0) {
            set_track_locks(n, lp[a] - 1, step->locks[a] - 1);
        }
    }
    steps[n].locks_enabled = step->data.locks_enabled;
    steps[n].cond_plock = step->data.cond_plock;
    steps[n].cond_id = step->data.cond_id;
}

// ============================================================================
// Preview
// ============================================================================

void SPSXSeqTrack::preview_step(uint8_t step) {
    if (step >= length) return;

    // Send all lock values for this step (kit defaults overlaid with plocks)
    uint16_t lock_idx = get_lockidx(step);
    for (uint8_t c = 0; c < SPSX_NUM_LOCKS; c++) {
        bool lock_bit = steps[step].is_lock_bit(c);
        if (lock_bit && locks_params[c] && steps[step].locks_enabled) {
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
    mixer_page.trig(track_number);
}

// ============================================================================
// Transpose
// ============================================================================

void SPSXSeqTrack::transpose(int8_t offset) {
    for (uint8_t n = 0; n < 64; n++) {
        uint8_t pitch = get_track_lock_implicit(n, 0);
        if (pitch == 255) continue;
        int16_t new_pitch = pitch + offset;
        if (new_pitch < 0) new_pitch = 0;
        if (new_pitch > 127) new_pitch = 127;
        set_track_pitch(n, (uint8_t)new_pitch);
    }
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

    // Use pattern length as default, SPSX extension overrides below
    if (pattern->patternLength > 0 && pattern->patternLength <= 64) {
        length = pattern->patternLength;
        track_length = length;
    }

    // Convert V3 swing amount to SPSX microtiming for swung steps.
    // swingAmount is Q14 format (50<<14 = neutral). The legacy formula adds
    // (swingAmount * timing_mid + 8192) >> 14 ticks of offset at the legacy
    // resolution. We convert that offset into the SPSX -127..+127 range.
    // Only applies to V3 patterns (version < 0x40); SPSX patterns have
    // their own microtiming data that gets copied directly below.
    if (pattern->version < 0x40) {
        uint32_t swing_q14 = pattern->swingAmount;
        // timing_mid at 1x legacy speed = 12 (ticks per half-step)
        uint16_t timing_mid = 12;
        int32_t swing_offset = (int32_t)((swing_q14 * timing_mid + 8192) >> 14);
        // Convert to SPSX: offset relative to timing_mid, scaled to -127..+127
        // At 1x: quarter-step = timing_mid/2 = 6 ticks = microtiming ±127
        if (swing_offset > 0 && timing_mid > 0) {
            int8_t mt = (int8_t)((int32_t)swing_offset * 127 / (timing_mid / 2));
            if (mt > 127) mt = 127;
            for (uint8_t s = 0; s < 64; s++) {
                if (SPSX_IS_BIT_SET64(swing_mask, s)) {
                    microtiming[s] = mt;
                }
            }
        }
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
        // Direct copy: microtiming (both are signed int8_t)
        // This replaces any V3 swing conversion above
        memcpy(microtiming, pattern->ext_microtiming[trk], 64);

        // Unpack step flags
        for (uint8_t s = 0; s < 64; s++) {
            uint8_t flags = pattern->ext_step_flags[trk][s];
            steps[s].cond_id = flags >> 2;
            steps[s].cond_plock = (flags >> 1) & 1;
            steps[s].locks_enabled = flags & 1;
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
