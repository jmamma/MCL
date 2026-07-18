#if !defined(__AVR__)

#include "Host/SpsHostSeqBridge.h"
#include "SpsHostSeqBridge_Internal.h"
#include "MCLPlatformFeatures.h"
#if MCL_FEATURE_HOST_ARRANGER
#include "Arrangement/MCLArrangement.h"
#endif
#include "Grid/GridTask.h"

#include <string.h>

using namespace spsseq;
using namespace sps_host_seq_internal;

namespace {

static void markArrangerLocalEdit(uint8_t slot) {
#if MCL_FEATURE_HOST_ARRANGER
    mcl_arrangement.markRuntimePrivateSourceEdited(slot);
#else
    (void)slot;
#endif
}

struct HostStepTrackSnapshot {
    SPSXSeqTrackData data;
    uint8_t effectiveLength = 16;
    uint8_t effectiveSpeed = STEPSEQ_SPEED_1X;
};

struct HostStepClipboard {
    HostStepTrackSnapshot tracks[NUM_MD_TRACKS];
    uint8_t scope = STEP_CLIP_STEP;
    uint8_t sourceTrack = 0;
    uint8_t sourcePage = 0;
    uint8_t sourceStep = 0;
    uint8_t accentAmount = 0;
    bool valid = false;
};

HostStepClipboard hostStepClipboard;

static void captureTrack(const SPSXSeqTrack& track,
                         HostStepTrackSnapshot& out) {
    memcpy(&out.data, static_cast<const SPSXSeqTrackData*>(&track),
           sizeof(out.data));
    out.effectiveLength = track.length;
    out.effectiveSpeed = track.speed;
}

static void copySnapshotStep(const HostStepTrackSnapshot& source,
                             uint8_t stepIndex, StepSeqStep& out) {
    const SPSXSeqTrackData& data = source.data;
    out.active = true;
    out.microtiming = data.microtiming[stepIndex];
    out.trig = STEPSEQ_IS_BIT_SET64(data.trig_mask, stepIndex);
    out.slide = STEPSEQ_IS_BIT_SET64(data.slide_mask, stepIndex);
    out.accent = STEPSEQ_IS_BIT_SET64(data.accent_mask, stepIndex);
    out.swing = STEPSEQ_IS_BIT_SET64(data.swing_mask, stepIndex);
    out.mute = STEPSEQ_IS_BIT_SET64(data.mute_mask, stepIndex);
    memset(out.locks, 0, sizeof(out.locks));

    uint16_t valueIndex = data.get_lockidx(stepIndex);
    uint64_t lockMask = data.steps[stepIndex].locks;
    for (uint8_t lock = 0; lock < SPSX_NUM_LOCKS; ++lock) {
        if ((lockMask & (1ULL << lock)) == 0)
            continue;
        if (valueIndex < STEPSEQ_NUM_LOCK_SLOTS)
            out.locks[lock] = (uint8_t)(data.locks[valueIndex] + 1);
        ++valueIndex;
    }
    memcpy(&out.data, &data.steps[stepIndex], sizeof(out.data));
}

static void restoreWholeTrack(SPSXSeqTrack& destination,
                              const HostStepTrackSnapshot& source) {
    memcpy(static_cast<SPSXSeqTrackData*>(&destination), &source.data,
           sizeof(source.data));

    // The stored 0/0xFF values mean "inherit pattern".  Apply the captured
    // effective runtime values, then put those inheritance markers back.
    const uint8_t storedLength = destination.track_length;
    const uint8_t storedSpeed = destination.track_speed;
    destination.length = source.effectiveLength;
    destination.speed = source.effectiveSpeed;
    destination.clear_oneshot();
    destination.set_length(source.effectiveLength);
    destination.track_length = storedLength;
    destination.set_speed(source.effectiveSpeed, source.effectiveSpeed, false);
    destination.track_speed = storedSpeed;
    destination.notes.init();
    destination.clean_params();
}

static void pasteSnapshotStep(SPSXSeqTrack& destination, uint8_t destStep,
                              const HostStepTrackSnapshot& source,
                              uint8_t sourceStep) {
    StepSeqStep step;
    copySnapshotStep(source, sourceStep, step);
    destination.paste_step(destStep, &step, source.data.locks_params);
}

static void clearRange(SPSXSeqTrack& track, uint8_t first, uint8_t count) {
    uint16_t endValue = (uint16_t)first + count;
    if (endValue > STEPSEQ_NUM_STEPS)
        endValue = STEPSEQ_NUM_STEPS;
    const uint8_t end = (uint8_t)endValue;
    for (uint8_t step = first; step < end; ++step)
        track.clear_step(step);
    track.clean_params();
}

} // namespace

int SpsHostSeqBridge::wireToMclMask(int w) {
    switch (w) {
        case WMASK_TRIG:          return STEPSEQ_MASK_PATTERN;
        case WMASK_MUTE:          return STEPSEQ_MASK_MUTE;
        case WMASK_SLIDE:         return STEPSEQ_MASK_SLIDE;
        case WMASK_SWING:         return STEPSEQ_MASK_SWING;
        // No WMASK_LOCK_ENABLED: MCL has no lock-enable gate. Lock values use
        // SET_LOCK/CLR_LOCK; WMASK_LOCKS_ON_STEP is read-only (no SET_STEP).
        default:                  return -1;
    }
}

bool SpsHostSeqBridge::applySetStep(const uint8_t* b, uint16_t n) {
    if (n < 4) return false;
    grid_task.service_host_arranger_load_before_edit();
    SPSXSeqTrack* tr = spsxTrack(b[0]);
    if (!tr || b[1] >= kNumSteps) return false;
    if (b[2] == WMASK_ACCENT) {
        if (b[3]) STEPSEQ_SET_BIT64(tr->accent_mask, b[1]);
        else STEPSEQ_CLEAR_BIT64(tr->accent_mask, b[1]);
        tr->clear_step_oneshot_state(b[1]);
        markArrangerLocalEdit(b[0]);
        return true;
    }
    int mclMask = wireToMclMask(b[2]);
    if (mclMask < 0) return false;
    tr->set_step(b[1], (uint8_t)mclMask, b[3] != 0);
    markArrangerLocalEdit(b[0]);
    return true;
}

bool SpsHostSeqBridge::applySetMicroTiming(const uint8_t* b, uint16_t n) {
    if (n < 3) return false;
    grid_task.service_host_arranger_load_before_edit();
    SPSXSeqTrack* tr = spsxTrack(b[0]);
    if (!tr || b[1] >= kNumSteps) return false;
    int8_t mt = (int8_t)b[2];
    if (tr->microtiming[b[1]] != mt) {
        tr->clear_step_oneshot_state(b[1]);
        tr->microtiming[b[1]] = mt;
    }
    markArrangerLocalEdit(b[0]);
    return true;
}

bool SpsHostSeqBridge::applySetCondition(const uint8_t* b, uint16_t n) {
    if (n < 3) return false;
    grid_task.service_host_arranger_load_before_edit();
    SPSXSeqTrack* tr = spsxTrack(b[0]);
    if (!tr || b[1] >= kNumSteps) return false;
    uint8_t cond = (uint8_t)(b[2] & 0x3F);
    bool plock = (b[2] & 0x80) != 0;
    if (tr->steps[b[1]].cond_id != cond ||
        tr->steps[b[1]].cond_plock != plock) {
        tr->clear_step_oneshot_state(b[1]);
    }
    tr->steps[b[1]].cond_id = cond;
    tr->steps[b[1]].cond_plock = plock;
    markArrangerLocalEdit(b[0]);
    return true;
}

bool SpsHostSeqBridge::applySetLock(const uint8_t* b, uint16_t n) {
    if (n < 4) return false;
    grid_task.service_host_arranger_load_before_edit();
    SPSXSeqTrack* tr = spsxTrack(b[0]);
    if (!tr || b[1] >= kNumSteps || b[2] >= negotiated_lock_params_)
        return false;
    tr->set_track_locks(b[1], b[2], (uint8_t)(b[3] & 0x7F));
    markArrangerLocalEdit(b[0]);
    return true;
}

bool SpsHostSeqBridge::applyClrLock(const uint8_t* b, uint16_t n) {
    if (n < 3) return false;
    grid_task.service_host_arranger_load_before_edit();
    SPSXSeqTrack* tr = spsxTrack(b[0]);
    if (!tr || b[1] >= kNumSteps || b[2] >= negotiated_lock_params_)
        return false;
    tr->clear_step_lock(b[1], b[2]);
    markArrangerLocalEdit(b[0]);
    return true;
}

bool SpsHostSeqBridge::applyClearStepLocks(const uint8_t* b, uint16_t n) {
    if (n < 2) return false;
    grid_task.service_host_arranger_load_before_edit();
    SPSXSeqTrack* tr = spsxTrack(b[0]);
    if (!tr || b[1] >= kNumSteps) return false;
    tr->clear_step_locks(b[1]);
    markArrangerLocalEdit(b[0]);
    return true;
}

bool SpsHostSeqBridge::applyClearStepRange(const uint8_t* b, uint16_t n) {
    if (n < 4)
        return false;

    uint16_t trackMask = getU16le(b);
    uint8_t startStep = b[2];
    uint8_t stepCount = b[3];
    if (trackMask == 0 || startStep >= kNumSteps || stepCount == 0)
        return false;

    uint8_t endStep = startStep + stepCount;
    if (endStep < startStep || endStep > kNumSteps)
        endStep = kNumSteps;
    if (endStep <= startStep)
        return false;

    grid_task.service_host_arranger_load_before_edit();

    bool changed = false;
    for (uint8_t track = 0; track < NUM_MD_TRACKS; track++) {
        if ((trackMask & (uint16_t)(1u << track)) == 0)
            continue;

        SPSXSeqTrack* tr = spsxTrack(track);
        if (!tr)
            continue;

        for (uint8_t step = startStep; step < endStep; step++)
            tr->clear_step(step);
        markArrangerLocalEdit(track);
        changed = true;
    }

    return changed;
}

bool SpsHostSeqBridge::applyStepClipboard(const uint8_t* b, uint16_t n) {
    if (n < 5)
        return false;

    const uint8_t action = b[0];
    const uint8_t scope = b[1];
    const uint8_t track = b[2];
    const uint8_t page = b[3];
    const uint8_t step = b[4];
    if (action > STEP_CLIP_CLEAR || scope > STEP_CLIP_ALL ||
        track >= NUM_MD_TRACKS || page >= 4 || step >= kNumSteps) {
        return false;
    }

    grid_task.service_host_arranger_load_before_edit();

    if (action == STEP_CLIP_COPY) {
        for (uint8_t i = 0; i < NUM_MD_TRACKS; ++i)
            captureTrack(mcl_seq.spsx_tracks[i], hostStepClipboard.tracks[i]);
        hostStepClipboard.scope = scope;
        hostStepClipboard.sourceTrack = track;
        hostStepClipboard.sourcePage = page;
        hostStepClipboard.sourceStep = step;
        hostStepClipboard.accentAmount = mcl_seq.spsx_accent_amount;
        hostStepClipboard.valid = true;
        return true;
    }

    if (action == STEP_CLIP_PASTE &&
        (!hostStepClipboard.valid || hostStepClipboard.scope != scope)) {
        return false;
    }

    const uint8_t pageStart = (uint8_t)(page * 16);
    const uint8_t sourcePageStart =
        (uint8_t)(hostStepClipboard.sourcePage * 16);
    bool changed = false;

    auto mutateTrack = [&](uint8_t index) {
        markArrangerLocalEdit(index);
        changed = true;
    };

    if (action == STEP_CLIP_CLEAR) {
        switch (scope) {
            case STEP_CLIP_STEP:
                mcl_seq.spsx_tracks[track].clear_step(step);
                mcl_seq.spsx_tracks[track].clean_params();
                mutateTrack(track);
                break;
            case STEP_CLIP_PAGE:
                clearRange(mcl_seq.spsx_tracks[track], pageStart, 16);
                mutateTrack(track);
                break;
            case STEP_CLIP_TRACK:
                clearRange(mcl_seq.spsx_tracks[track], 0, kNumSteps);
                mutateTrack(track);
                break;
            case STEP_CLIP_ALL_PAGE:
                for (uint8_t i = 0; i < NUM_MD_TRACKS; ++i) {
                    clearRange(mcl_seq.spsx_tracks[i], pageStart, 16);
                    mutateTrack(i);
                }
                break;
            case STEP_CLIP_ALL:
                for (uint8_t i = 0; i < NUM_MD_TRACKS; ++i) {
                    clearRange(mcl_seq.spsx_tracks[i], 0, kNumSteps);
                    mutateTrack(i);
                }
                break;
        }
        return changed;
    }

    switch (scope) {
        case STEP_CLIP_STEP:
            pasteSnapshotStep(mcl_seq.spsx_tracks[track], step,
                              hostStepClipboard.tracks[
                                  hostStepClipboard.sourceTrack],
                              hostStepClipboard.sourceStep);
            mcl_seq.spsx_tracks[track].clean_params();
            mutateTrack(track);
            break;
        case STEP_CLIP_PAGE:
            for (uint8_t offset = 0; offset < 16; ++offset) {
                pasteSnapshotStep(mcl_seq.spsx_tracks[track],
                                  (uint8_t)(pageStart + offset),
                                  hostStepClipboard.tracks[
                                      hostStepClipboard.sourceTrack],
                                  (uint8_t)(sourcePageStart + offset));
            }
            mcl_seq.spsx_tracks[track].clean_params();
            mutateTrack(track);
            break;
        case STEP_CLIP_TRACK:
            restoreWholeTrack(mcl_seq.spsx_tracks[track],
                              hostStepClipboard.tracks[
                                  hostStepClipboard.sourceTrack]);
            mutateTrack(track);
            break;
        case STEP_CLIP_ALL_PAGE:
            for (uint8_t i = 0; i < NUM_MD_TRACKS; ++i) {
                for (uint8_t offset = 0; offset < 16; ++offset) {
                    pasteSnapshotStep(mcl_seq.spsx_tracks[i],
                                      (uint8_t)(pageStart + offset),
                                      hostStepClipboard.tracks[i],
                                      (uint8_t)(sourcePageStart + offset));
                }
                mcl_seq.spsx_tracks[i].clean_params();
                mutateTrack(i);
            }
            break;
        case STEP_CLIP_ALL:
            for (uint8_t i = 0; i < NUM_MD_TRACKS; ++i) {
                restoreWholeTrack(mcl_seq.spsx_tracks[i],
                                  hostStepClipboard.tracks[i]);
                mutateTrack(i);
            }
            mcl_seq.set_spsx_accent_amount(hostStepClipboard.accentAmount,
                                           false);
            break;
    }
    return changed;
}

bool SpsHostSeqBridge::applySetTrackProp(const uint8_t* b, uint16_t n) {
    if (n < 3) return false;
    grid_task.service_host_arranger_load_before_edit();
    SPSXSeqTrack* tr = spsxTrack(b[0]);
    if (!tr) return false;
    switch (b[1]) {
        // Set the stored override AND apply via the canonical setters so the
        // effective length/speed and runtime timing update live.
        case TPROP_LENGTH: tr->track_length = b[2]; tr->set_length(b[2]); break;
        case TPROP_SPEED:  tr->track_speed  = b[2]; tr->set_speed(b[2]);  break;
        case TPROP_SWING_AMOUNT:
            if (b[2] > 30) return false;
            tr->request_swing_amount_change(b[2]);
            break;
        default: return false;  // scale/mute: TODO
    }
    markArrangerLocalEdit(b[0]);
    return true;
}

bool SpsHostSeqBridge::applySetPatternProp(const uint8_t* b, uint16_t n) {
    if (n < 3) return false;
    switch (b[0]) {
        case PPROP_TEMPO:
            setMclLiveTempoRaw(getU16le(b + 1));
            return true;
        case PPROP_ACCENT_AMOUNT:
            if (getU16le(b + 1) > 15) return false;
            mcl_seq.set_spsx_accent_amount((uint8_t)getU16le(b + 1), false);
            return true;
        default:
            return false;  // length/scale/etc.: TODO
    }
}

#endif  // !defined(__AVR__)
