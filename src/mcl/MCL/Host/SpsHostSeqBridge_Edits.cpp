#if !defined(__AVR__)

#include "Host/SpsHostSeqBridge.h"
#include "SpsHostSeqBridge_Internal.h"
#include "MCLPlatformFeatures.h"
#if MCL_FEATURE_HOST_ARRANGER
#include "Arrangement/MCLArrangement.h"
#endif
#include "Grid/GridTask.h"

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
    int mclMask = wireToMclMask(b[2]);
    if (!tr || mclMask < 0 || b[1] >= kNumSteps) return false;
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
    if (!tr || b[1] >= kNumSteps || b[2] >= kNumLockParams) return false;  // param range
    tr->set_track_locks(b[1], b[2], (uint8_t)(b[3] & 0x7F));
    markArrangerLocalEdit(b[0]);
    return true;
}

bool SpsHostSeqBridge::applyClrLock(const uint8_t* b, uint16_t n) {
    if (n < 3) return false;
    grid_task.service_host_arranger_load_before_edit();
    SPSXSeqTrack* tr = spsxTrack(b[0]);
    if (!tr || b[1] >= kNumSteps || b[2] >= kNumLockParams) return false;  // param range
    tr->clear_step_lock(b[1], b[2]);
    markArrangerLocalEdit(b[0]);
    return true;
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
        default: return false;  // scale/swing/mute: TODO
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
        default:
            return false;  // length/scale/etc.: TODO
    }
}

#endif  // !defined(__AVR__)
