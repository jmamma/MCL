#if !defined(__AVR__)

#include "SpsHostSeqBridge.h"
#include "SpsHostSeqBridge_Internal.h"
#include "MCLPlatformFeatures.h"
#if MCL_FEATURE_HOST_ARRANGER
#include "Arrangement/MCLArrangement.h"
#endif
#include "Grid/GridTask.h"

#ifdef EXT_TRACKS

using namespace spsseq;
using namespace sps_host_seq_internal;

namespace {

static constexpr uint16_t kExtNotesHeaderBytes = 14;
static constexpr uint16_t kExtLocksHeaderBytes = 16;

static void markArrangerLocalExtEdit(uint8_t device, uint8_t trackIndex) {
#if MCL_FEATURE_HOST_ARRANGER
    if (device == EXT_DEVICE_GRID_Y && trackIndex < GRID_WIDTH)
        mcl_arrangement.markRuntimePrivateSourceEdited(
            (uint8_t)(GRID_WIDTH + trackIndex));
#else
    (void)device;
    (void)trackIndex;
#endif
}

static void fillExtMeta(uint8_t* body, uint8_t device, uint8_t trackIndex,
                        SeqExtStepTrackApi& track) {
    body[0] = device;
    body[1] = trackIndex;
    body[2] = SeqTrackUtil::use_midi_tracks_for_ext() ? 1 : 0;
    body[3] = track.length();
    body[4] = track.speed();
    body[5] = track.channel();
    putU16le(body + 6, track.ticks_per_step());
    body[8] = track.step_count();
    putU16le(body + 9, track.mod_ticks());
    body[11] = track.change_counter();
    body[12] = extStepTrackCount();
}

static bool extNoteFromEvent(SeqExtStepTrackApi& track, uint8_t step,
                             uint16_t eventIdx, uint16_t eventEnd,
                             uint32_t& startTick, uint32_t& width,
                             uint8_t& note, uint8_t& velocity,
                             uint8_t& condition) {
    SeqExtStepEvent ev = track.event(eventIdx);
    if (ev.is_lock || !ev.event_on)
        return false;

    uint16_t noteOffIdx = eventIdx;
    uint8_t offStep = track.search_note_off((uint8_t)ev.event_value, step,
                                            noteOffIdx, eventEnd);
    seq_extstep_tick_t noteStart = track.event_tick(step, ev);
    seq_extstep_tick_t noteEnd = noteStart + track.ticks_per_step();

    if (noteOffIdx != 0xFFFF) {
        SeqExtStepEvent offEvent = track.event(noteOffIdx);
        noteEnd = track.event_tick(offStep, offEvent);
        if (noteEnd <= noteStart)
            noteEnd += (seq_extstep_tick_t)track.length() *
                       (seq_extstep_tick_t)track.ticks_per_step();
    }
    if (noteStart < 0)
        noteStart = 0;
    if (noteEnd <= noteStart)
        noteEnd = noteStart + 1;

    startTick = (uint32_t)noteStart;
    width = (uint32_t)(noteEnd - noteStart);
    note = (uint8_t)(ev.event_value & 0x7F);
    velocity = track.note_velocity(step, eventIdx) & 0x7F;
    condition = ev.cond_id & 0x7F;
    return true;
}

static uint16_t countExtNotes(SeqExtStepTrackApi& track) {
    uint16_t count = 0;
    uint16_t eventIdx = 0;
    uint16_t eventEnd = 0;
    for (uint8_t step = 0; step < track.length(); step++) {
        eventEnd += track.event_bucket_size(step);
        for (; eventIdx != eventEnd; ++eventIdx) {
            SeqExtStepEvent ev = track.event(eventIdx);
            if (!ev.is_lock && ev.event_on)
                count++;
        }
    }
    return count;
}

static uint16_t countExtLocks(SeqExtStepTrackApi& track, uint8_t lockIdx) {
    uint16_t count = 0;
    uint16_t eventIdx = 0;
    uint16_t eventEnd = 0;
    for (uint8_t step = 0; step < track.length(); step++) {
        eventEnd += track.event_bucket_size(step);
        for (; eventIdx != eventEnd; ++eventIdx) {
            SeqExtStepEvent ev = track.event(eventIdx);
            if (ev.is_lock && ev.lock_idx == lockIdx)
                count++;
        }
    }
    return count;
}

} // namespace

void SpsHostSeqBridge::sendExtTrackMeta(uint8_t tag, uint8_t device,
                                        int trackIndex) {
    if (!validExtStepTrack(device, trackIndex)) {
        sendErr(tag, ERR_BAD_TRACK, (uint8_t)trackIndex);
        return;
    }
    SeqExtStepTrackApi track =
        SeqTrackUtil::get_ext_step_track((uint8_t)trackIndex);
    uint8_t body[13];
    fillExtMeta(body, device, (uint8_t)trackIndex, track);
    sendFrame(CMD_EXT_TRACK_META, tag, body, (uint16_t)sizeof body);
}

void SpsHostSeqBridge::sendExtNotes(uint8_t tag, uint8_t device,
                                    int trackIndex) {
    if (!validExtStepTrack(device, trackIndex)) {
        sendErr(tag, ERR_BAD_TRACK, (uint8_t)trackIndex);
        return;
    }

    SeqExtStepTrackApi track =
        SeqTrackUtil::get_ext_step_track((uint8_t)trackIndex);
    const uint16_t maxNotesPerPage =
        (uint16_t)((kMaxBodyRaw - kExtNotesHeaderBytes) /
                   kExtNoteWireBytes);
    uint16_t noteTotal = countExtNotes(track);
    uint8_t pageCount = (uint8_t)(noteTotal == 0
                                      ? 1
                                      : ((noteTotal + maxNotesPerPage - 1) /
                                         maxNotesPerPage));
    if (pageCount == 0)
        pageCount = 1;

    uint8_t body[kMaxBodyRaw];
    uint8_t page = 0;
    uint8_t count = 0;
    uint16_t off = kExtNotesHeaderBytes;
    auto beginPage = [&]() {
        body[0] = device;
        body[1] = (uint8_t)trackIndex;
        body[2] = page;
        body[3] = pageCount;
        body[4] = 0;
        body[5] = track.length();
        body[6] = track.speed();
        body[7] = track.channel();
        putU16le(body + 8, track.ticks_per_step());
        body[10] = track.step_count();
        putU16le(body + 11, track.mod_ticks());
        body[13] = track.change_counter();
        count = 0;
        off = kExtNotesHeaderBytes;
    };
    auto flushPage = [&]() {
        body[4] = count;
        sendFrame(CMD_EXT_NOTES, tag, body, off);
        page++;
        beginPage();
    };

    beginPage();
    uint16_t eventIdx = 0;
    uint16_t eventEnd = 0;
    for (uint8_t step = 0; step < track.length(); step++) {
        eventEnd += track.event_bucket_size(step);
        for (; eventIdx != eventEnd; ++eventIdx) {
            uint32_t startTick = 0;
            uint32_t width = 0;
            uint8_t note = 0;
            uint8_t velocity = 0;
            uint8_t condition = 0;
            if (!extNoteFromEvent(track, step, eventIdx, eventEnd, startTick,
                                  width, note, velocity, condition)) {
                continue;
            }
            if (count >= maxNotesPerPage)
                flushPage();
            putU32le(body + off, startTick);
            putU32le(body + off + 4, width);
            body[off + 8] = note;
            body[off + 9] = velocity;
            body[off + 10] = condition;
            off = (uint16_t)(off + kExtNoteWireBytes);
            count++;
        }
    }
    flushPage();
}

void SpsHostSeqBridge::sendExtLocks(uint8_t tag, uint8_t device,
                                    int trackIndex, uint8_t lockIdx) {
    if (!validExtStepTrack(device, trackIndex)) {
        sendErr(tag, ERR_BAD_TRACK, (uint8_t)trackIndex);
        return;
    }

    SeqExtStepTrackApi track =
        SeqTrackUtil::get_ext_step_track((uint8_t)trackIndex);
    const uint16_t maxLocksPerPage =
        (uint16_t)((kMaxBodyRaw - kExtLocksHeaderBytes) /
                   kExtLockWireBytes);
    uint16_t lockTotal = countExtLocks(track, lockIdx);
    uint8_t pageCount = (uint8_t)(lockTotal == 0
                                      ? 1
                                      : ((lockTotal + maxLocksPerPage - 1) /
                                         maxLocksPerPage));
    if (pageCount == 0)
        pageCount = 1;

    uint8_t selectedParam = 0xFF;
    track.locks().selected_lock_param_id(lockIdx, selectedParam);

    uint8_t body[kMaxBodyRaw];
    uint8_t page = 0;
    uint8_t count = 0;
    uint16_t off = kExtLocksHeaderBytes;
    auto beginPage = [&]() {
        body[0] = device;
        body[1] = (uint8_t)trackIndex;
        body[2] = lockIdx;
        body[3] = page;
        body[4] = pageCount;
        body[5] = 0;
        body[6] = track.length();
        body[7] = track.speed();
        body[8] = track.channel();
        putU16le(body + 9, track.ticks_per_step());
        body[11] = track.step_count();
        putU16le(body + 12, track.mod_ticks());
        body[14] = track.change_counter();
        body[15] = selectedParam;
        count = 0;
        off = kExtLocksHeaderBytes;
    };
    auto flushPage = [&]() {
        body[5] = count;
        sendFrame(CMD_EXT_LOCKS, tag, body, off);
        page++;
        beginPage();
    };

    beginPage();
    uint16_t eventIdx = 0;
    uint16_t eventEnd = 0;
    for (uint8_t step = 0; step < track.length(); step++) {
        eventEnd += track.event_bucket_size(step);
        for (; eventIdx != eventEnd; ++eventIdx) {
            SeqExtStepEvent ev = track.event(eventIdx);
            if (!ev.is_lock || ev.lock_idx != lockIdx)
                continue;
            if (count >= maxLocksPerPage)
                flushPage();
            seq_extstep_tick_t tick = track.event_tick(step, ev);
            if (tick < 0)
                tick = 0;
            putU32le(body + off, (uint32_t)tick);
            body[off + 4] = (uint8_t)(ev.event_value & 0x7F);
            body[off + 5] = ev.event_on ? 1 : 0;
            off = (uint16_t)(off + kExtLockWireBytes);
            count++;
        }
    }
    flushPage();
}

bool SpsHostSeqBridge::applyExtAddNote(const uint8_t* b, uint16_t n) {
    if (n < 13 || !validExtStepTrack(b[0], b[1]))
        return false;
    grid_task.service_host_arranger_load_before_edit();
    SeqExtStepTrackApi track = SeqTrackUtil::get_ext_step_track(b[1]);
    uint32_t startTick = getU32le(b + 2);
    uint32_t width = getU32le(b + 6);
    uint8_t note = b[10] & 0x7F;
    uint8_t velocity = b[11] & 0x7F;
    uint8_t condition = b[12] & 0x7F;
    if (width == 0)
        return false;
    track.replace_note((seq_extstep_tick_t)startTick,
                       (seq_extstep_tick_t)width, note, velocity, condition);
    markArrangerLocalExtEdit(b[0], b[1]);
    return true;
}

bool SpsHostSeqBridge::applyExtDeleteNote(const uint8_t* b, uint16_t n) {
    if (n < 11 || !validExtStepTrack(b[0], b[1]))
        return false;
    grid_task.service_host_arranger_load_before_edit();
    SeqExtStepTrackApi track = SeqTrackUtil::get_ext_step_track(b[1]);
    uint32_t startTick = getU32le(b + 2);
    uint32_t width = getU32le(b + 6);
    uint8_t note = b[10] & 0x7F;
    if (width == 0)
        width = 1;
    bool changed = track.delete_note((seq_extstep_tick_t)startTick,
                                     (seq_extstep_tick_t)(width - 1), note);
    if (changed)
        markArrangerLocalExtEdit(b[0], b[1]);
    return changed;
}

bool SpsHostSeqBridge::applyExtToggleNote(const uint8_t* b, uint16_t n) {
    if (n < 13 || !validExtStepTrack(b[0], b[1]))
        return false;
    grid_task.service_host_arranger_load_before_edit();
    SeqExtStepTrackApi track = SeqTrackUtil::get_ext_step_track(b[1]);
    uint32_t startTick = getU32le(b + 2);
    uint32_t width = getU32le(b + 6);
    uint8_t note = b[10] & 0x7F;
    uint8_t velocity = b[11] & 0x7F;
    uint8_t condition = b[12] & 0x7F;
    if (width == 0)
        return false;
    bool changed = track.toggle_note((seq_extstep_tick_t)startTick,
                                     (seq_extstep_tick_t)width, note,
                                     velocity, condition);
    if (changed)
        markArrangerLocalExtEdit(b[0], b[1]);
    return changed;
}

bool SpsHostSeqBridge::applyExtClearRange(const uint8_t* b, uint16_t n) {
    if (n < 12 || !validExtStepTrack(b[0], b[1]))
        return false;
    grid_task.service_host_arranger_load_before_edit();
    SeqExtStepTrackApi track = SeqTrackUtil::get_ext_step_track(b[1]);
    uint32_t startTick = getU32le(b + 2);
    uint32_t width = getU32le(b + 6);
    uint8_t noteMin = b[10] & 0x7F;
    uint8_t noteMax = b[11] & 0x7F;
    if (noteMax < noteMin)
        noteMax = noteMin;
    if (width == 0)
        width = 1;
    bool changed = track.delete_notes((seq_extstep_tick_t)startTick,
                                      (seq_extstep_tick_t)(width - 1),
                                      noteMin, noteMax);
    if (changed)
        markArrangerLocalExtEdit(b[0], b[1]);
    return changed;
}

bool SpsHostSeqBridge::applyExtSetTrackProp(const uint8_t* b, uint16_t n) {
    if (n < 4 || !validExtStepTrack(b[0], b[1]))
        return false;
    grid_task.service_host_arranger_load_before_edit();
    SeqExtStepTrackApi track = SeqTrackUtil::get_ext_step_track(b[1]);
    switch (b[2]) {
        case EXTPROP_LENGTH:
            track.set_length(b[3]);
            markArrangerLocalExtEdit(b[0], b[1]);
            return true;
        case EXTPROP_SPEED:
            track.set_speed(b[3]);
            markArrangerLocalExtEdit(b[0], b[1]);
            return true;
        case EXTPROP_CHANNEL:
            track.set_channel(b[3] & 0x0F);
            markArrangerLocalExtEdit(b[0], b[1]);
            return true;
        default:
            return false;
    }
}

bool SpsHostSeqBridge::applyExtSetLock(const uint8_t* b, uint16_t n) {
    if (n < 6 || !validExtStepTrack(b[0], b[1]))
        return false;
    grid_task.service_host_arranger_load_before_edit();
    SeqExtStepTrackApi track = SeqTrackUtil::get_ext_step_track(b[1]);
    uint8_t lockIdx = b[2];
    uint8_t step = b[3];
    uint8_t value = b[4] & 0x7F;
    bool slide = b[5] != 0;
    if (step >= track.length())
        return false;
    uint8_t param = 0;
    SeqExtStepLockApi locks = track.locks();
    if (!locks.selected_lock_param_id(lockIdx, param))
        return false;
    locks.clear_step_locks(step, lockIdx);
    bool changed = locks.add_lock(step, track.ticks_per_step(), param, value,
                                  slide, lockIdx);
    if (changed)
        markArrangerLocalExtEdit(b[0], b[1]);
    return changed;
}

bool SpsHostSeqBridge::applyExtClearLock(const uint8_t* b, uint16_t n) {
    if (n < 4 || !validExtStepTrack(b[0], b[1]))
        return false;
    grid_task.service_host_arranger_load_before_edit();
    SeqExtStepTrackApi track = SeqTrackUtil::get_ext_step_track(b[1]);
    uint8_t lockIdx = b[2];
    uint8_t step = b[3];
    if (step >= track.length())
        return false;
    track.locks().clear_step_locks(step, lockIdx);
    markArrangerLocalExtEdit(b[0], b[1]);
    return true;
}

bool SpsHostSeqBridge::applyExtClearLocks(const uint8_t* b, uint16_t n) {
    if (n < 3 || !validExtStepTrack(b[0], b[1]))
        return false;
    grid_task.service_host_arranger_load_before_edit();
    SeqExtStepTrackApi track = SeqTrackUtil::get_ext_step_track(b[1]);
    track.clear_track_locks(b[2]);
    markArrangerLocalExtEdit(b[0], b[1]);
    return true;
}

#else

void SpsHostSeqBridge::sendExtTrackMeta(uint8_t tag, uint8_t, int track) {
    sendErr(tag, ERR_UNSUPPORTED, (uint8_t)track);
}
void SpsHostSeqBridge::sendExtNotes(uint8_t tag, uint8_t, int track) {
    sendErr(tag, ERR_UNSUPPORTED, (uint8_t)track);
}
void SpsHostSeqBridge::sendExtLocks(uint8_t tag, uint8_t, int track,
                                    uint8_t) {
    sendErr(tag, ERR_UNSUPPORTED, (uint8_t)track);
}
bool SpsHostSeqBridge::applyExtAddNote(const uint8_t*, uint16_t) {
    return false;
}
bool SpsHostSeqBridge::applyExtDeleteNote(const uint8_t*, uint16_t) {
    return false;
}
bool SpsHostSeqBridge::applyExtToggleNote(const uint8_t*, uint16_t) {
    return false;
}
bool SpsHostSeqBridge::applyExtClearRange(const uint8_t*, uint16_t) {
    return false;
}
bool SpsHostSeqBridge::applyExtSetTrackProp(const uint8_t*, uint16_t) {
    return false;
}
bool SpsHostSeqBridge::applyExtSetLock(const uint8_t*, uint16_t) {
    return false;
}
bool SpsHostSeqBridge::applyExtClearLock(const uint8_t*, uint16_t) {
    return false;
}
bool SpsHostSeqBridge::applyExtClearLocks(const uint8_t*, uint16_t) {
    return false;
}

#endif // EXT_TRACKS

#endif  // !defined(__AVR__)
