#include "MCLPlatformFeatures.h"

#if MCL_FEATURE_HOST_ARRANGER

#include "Host/SpsHostArrBridge.h"
#include "SpsHostArrBridge_Internal.h"

#include <stdlib.h>

using namespace spsarr;
using namespace sps_host_arr_internal;

namespace {

static constexpr uint16_t kMaxClipStageRecords = 2048;

void putArrAutomationLane(uint8_t *dst,
                          const mclarrfile::AutomationLane &lane) {
    dst[0] = lane.track;
    dst[1] = lane.targetType;
    dst[2] = lane.device;
    dst[3] = lane.targetIndex;
    dst[4] = lane.valueType;
    dst[5] = lane.flags;
    spsArrPutU16(dst + 6, lane.pointCount);
    spsArrPutU32(dst + 8, lane.pointOffset);
    spsArrPutU32(dst + 12, lane.reserved);
}

void getArrAutomationLane(const uint8_t *src,
                          mclarrfile::AutomationLane &lane) {
    lane.track = src[0];
    lane.targetType = src[1];
    lane.device = src[2];
    lane.targetIndex = src[3];
    lane.valueType = src[4];
    lane.flags = src[5];
    lane.pointCount = spsArrGetU16(src + 6);
    lane.pointOffset = spsArrGetU32(src + 8);
    lane.reserved = spsArrGetU32(src + 12);
}

void putArrAutomationPoint(uint8_t *dst,
                           const mclarrfile::AutomationPoint &point) {
    spsArrPutU32(dst + 0, point.q12);
    spsArrPutU16(dst + 4, point.value);
    dst[6] = point.interp;
    dst[7] = (uint8_t)point.curve;
}

void getArrAutomationPoint(const uint8_t *src,
                           mclarrfile::AutomationPoint &point) {
    point.q12 = spsArrGetU32(src + 0);
    point.value = spsArrGetU16(src + 4);
    point.interp = src[6];
    point.curve = (int8_t)src[7];
}

void getArrClip(const uint8_t *src, mclarrfile::Clip &clip) {
    clip.startQ12 = spsArrGetU32(src + 0);
    clip.durationQ12 = spsArrGetU32(src + 4);
    clip.repeatQ12 = spsArrGetU32(src + 8);
    clip.track = src[12];
    clip.row = src[13];
    clip.flags = src[14];
    clip.reserved = src[15];
    clip.fadeFlags = src[16];
    clip.fadeTarget = src[17];
    clip.fadeDurationQ12 = spsArrGetU16(src + 18);
    clip.fadeAmount = src[20];
    clip.fadeCurve = (int8_t)src[21];
    clip.fadeReserved = spsArrGetU16(src + 22);
    clip.endFadeFlags = src[24];
    clip.endFadeTarget = src[25];
    clip.endFadeDurationQ12 = spsArrGetU16(src + 26);
    clip.endFadeAmount = src[28];
    clip.endFadeCurve = (int8_t)src[29];
    clip.endFadeReserved = spsArrGetU16(src + 30);
    clip.sourceKind = src[32];
    clip.sourceTrack = src[33];
    clip.sourceFlags = src[34];
    clip.sourceReserved = src[35];
    clip.sourceId = spsArrGetU32(src + 36);
}

bool automationLaneHeaderValid(const mclarrfile::AutomationLane &lane,
                               uint32_t maxPoints) {
    return lane.track < spsarr::kNumGridSlots &&
           lane.pointCount <= maxPoints &&
           lane.valueType <= mclarrfile::AUTOMATION_VALUE_U14 &&
           lane.targetType <= mclarrfile::AUTOMATION_TARGET_TEMPO;
}

bool arrPreviewTimingHasValues(const int8_t *timing, uint8_t count) {
    if (timing == nullptr)
        return false;
    for (uint8_t i = 0; i < count; i++) {
        if (timing[i] != 0)
            return true;
    }
    return false;
}

}  // namespace

void SpsHostArrBridge::clearAutomationStage() {
    if (automation_stage_points_ != nullptr) {
        free(automation_stage_points_);
        automation_stage_points_ = nullptr;
    }
    automation_stage_active_ = false;
    automation_stage_lane_ = {};
    automation_stage_total_ = 0;
    automation_stage_received_ = 0;
}

bool SpsHostArrBridge::beginAutomationStage(
    const mclarrfile::AutomationLane& lane) {
    clearAutomationStage();
    if (lane.pointCount > 0) {
        automation_stage_points_ =
            static_cast<mclarrfile::AutomationPoint*>(
                malloc(sizeof(mclarrfile::AutomationPoint) *
                       (size_t)lane.pointCount));
        if (automation_stage_points_ == nullptr) {
            return false;
        }
    }
    automation_stage_lane_ = lane;
    automation_stage_total_ = lane.pointCount;
    automation_stage_received_ = 0;
    automation_stage_active_ = true;
    return true;
}

void SpsHostArrBridge::clearClipStage() {
    if (clip_stage_clips_ != nullptr) {
        free(clip_stage_clips_);
        clip_stage_clips_ = nullptr;
    }
    clip_stage_active_ = false;
    clip_stage_total_ = 0;
    clip_stage_received_ = 0;
}

bool SpsHostArrBridge::beginClipStage(uint16_t total) {
    clearClipStage();
    if (total > kMaxClipStageRecords) {
        return false;
    }
    if (total > 0) {
        clip_stage_clips_ = static_cast<mclarrfile::Clip*>(
            malloc(sizeof(mclarrfile::Clip) * (size_t)total));
        if (clip_stage_clips_ == nullptr) {
            return false;
        }
    }
    clip_stage_total_ = total;
    clip_stage_received_ = 0;
    clip_stage_active_ = true;
    return true;
}

void SpsHostArrBridge::onReqArrMeta(uint8_t tag) {
    mclarrfile::Header header;
    if (!mcl_arrangement.readMeta(&header)) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }

    uint8_t body[8 + spsarr::kArrNameBytes];
    body[0] = mcl_cfg.active_arrangement_idx;
    spsArrPutU32(body + 1, header.clipCount);
    spsArrPutU16(body + 5, header.flags);
    body[7] = 0;
    for (uint8_t i = 0; i < spsarr::kArrNameBytes; ++i) {
        body[8 + i] = (uint8_t)header.name[i];
    }
    sendFrame(CMD_ARR_META, tag, body, (uint16_t)sizeof body);
}

void SpsHostArrBridge::onReqArrClips(uint8_t tag, const uint8_t* b,
                                     uint16_t n) {
    if (n < 11) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }

    uint32_t startQ12 = spsArrGetU32(b + 0);
    uint32_t endQ12 = spsArrGetU32(b + 4);
    uint16_t skip = spsArrGetU16(b + 8);
    uint8_t maxClips = b[10];
    if (maxClips == 0 ||
        maxClips > (uint8_t)spsarr::kMaxArrClipRecordsPerFrame) {
        maxClips = (uint8_t)spsarr::kMaxArrClipRecordsPerFrame;
    }

    mclarrfile::Clip clips[spsarr::kMaxArrClipRecordsPerFrame];
    uint32_t total = 0;
    bool more = false;
    uint16_t count = mcl_arrangement.readClips(startQ12, endQ12, skip,
                                               maxClips, clips, &total, &more);

    uint8_t body[16 + spsarr::kMaxArrClipRecordsPerFrame *
                     spsarr::kArrClipRecordBytes];
    body[0] = mcl_cfg.active_arrangement_idx;
    body[1] = more ? 1 : 0;
    spsArrPutU16(body + 2, skip);
    spsArrPutU32(body + 4, total);
    spsArrPutU32(body + 8, startQ12);
    spsArrPutU32(body + 12, endQ12);
    uint16_t off = 16;
    for (uint16_t i = 0; i < count; ++i) {
        putArrClip(body + off, clips[i]);
        off = (uint16_t)(off + spsarr::kArrClipRecordBytes);
    }
    sendFrame(CMD_ARR_CLIPS, tag, body, off);
}

void SpsHostArrBridge::onReqArrMarkers(uint8_t tag, const uint8_t* b,
                                       uint16_t n) {
    if (n < 11) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }

    uint32_t startQ12 = spsArrGetU32(b + 0);
    uint32_t endQ12 = spsArrGetU32(b + 4);
    uint16_t skip = spsArrGetU16(b + 8);
    uint8_t maxMarkers = b[10];
    if (maxMarkers == 0 ||
        maxMarkers > (uint8_t)spsarr::kMaxArrMarkerRecordsPerFrame) {
        maxMarkers = (uint8_t)spsarr::kMaxArrMarkerRecordsPerFrame;
    }

    mclarrfile::Marker markers[spsarr::kMaxArrMarkerRecordsPerFrame];
    uint32_t total = 0;
    bool more = false;
    uint16_t count = mcl_arrangement.readMarkers(startQ12, endQ12, skip,
                                                 maxMarkers, markers, &total,
                                                 &more);

    uint8_t body[16 + spsarr::kMaxArrMarkerRecordsPerFrame *
                     spsarr::kArrMarkerRecordBytes];
    body[0] = mcl_cfg.active_arrangement_idx;
    body[1] = more ? 1 : 0;
    spsArrPutU16(body + 2, skip);
    spsArrPutU32(body + 4, total);
    spsArrPutU32(body + 8, startQ12);
    spsArrPutU32(body + 12, endQ12);
    uint16_t off = 16;
    for (uint16_t i = 0; i < count; ++i) {
        putArrMarker(body + off, markers[i]);
        off = (uint16_t)(off + spsarr::kArrMarkerRecordBytes);
    }
    sendFrame(CMD_ARR_MARKERS, tag, body, off);
}

void SpsHostArrBridge::onReqArrLoopRegions(uint8_t tag, const uint8_t* b,
                                           uint16_t n) {
    if (n < 11) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }

    uint32_t startQ12 = spsArrGetU32(b + 0);
    uint32_t endQ12 = spsArrGetU32(b + 4);
    uint16_t skip = spsArrGetU16(b + 8);
    uint8_t maxRegions = b[10];
    if (maxRegions == 0 ||
        maxRegions > (uint8_t)spsarr::kMaxArrLoopRegionRecordsPerFrame) {
        maxRegions = (uint8_t)spsarr::kMaxArrLoopRegionRecordsPerFrame;
    }

    mclarrfile::LoopRegion regions[spsarr::kMaxArrLoopRegionRecordsPerFrame];
    uint32_t total = 0;
    bool more = false;
    uint16_t count =
        mcl_arrangement.readLoopRegions(startQ12, endQ12, skip, maxRegions,
                                        regions, &total, &more);

    uint8_t body[16 + spsarr::kMaxArrLoopRegionRecordsPerFrame *
                     spsarr::kArrLoopRegionRecordBytes];
    body[0] = mcl_cfg.active_arrangement_idx;
    body[1] = more ? 1 : 0;
    spsArrPutU16(body + 2, skip);
    spsArrPutU32(body + 4, total);
    spsArrPutU32(body + 8, startQ12);
    spsArrPutU32(body + 12, endQ12);
    uint16_t off = 16;
    for (uint16_t i = 0; i < count; ++i) {
        putArrLoopRegion(body + off, regions[i]);
        off = (uint16_t)(off + spsarr::kArrLoopRegionRecordBytes);
    }
    sendFrame(CMD_ARR_LOOP_REGIONS, tag, body, off);
}

void SpsHostArrBridge::onReqArrAutomationLanes(uint8_t tag,
                                               const uint8_t* b,
                                               uint16_t n) {
    if (n < 3) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }

    uint16_t skip = spsArrGetU16(b + 0);
    uint8_t maxLanes = b[2];
    if (maxLanes == 0 ||
        maxLanes > (uint8_t)spsarr::kMaxArrAutomationLaneRecordsPerFrame) {
        maxLanes = (uint8_t)spsarr::kMaxArrAutomationLaneRecordsPerFrame;
    }

    mclarrfile::AutomationLane
        lanes[spsarr::kMaxArrAutomationLaneRecordsPerFrame];
    uint32_t total = 0;
    bool more = false;
    uint16_t count = mcl_arrangement.readAutomationLanes(
        skip, maxLanes, lanes, &total, &more);

    uint8_t body[8 + spsarr::kMaxArrAutomationLaneRecordsPerFrame *
                         spsarr::kArrAutomationLaneRecordBytes];
    body[0] = mcl_cfg.active_arrangement_idx;
    body[1] = more ? 1 : 0;
    spsArrPutU16(body + 2, skip);
    spsArrPutU32(body + 4, total);
    uint16_t off = 8;
    for (uint16_t i = 0; i < count; ++i) {
        putArrAutomationLane(body + off, lanes[i]);
        off = (uint16_t)(off + spsarr::kArrAutomationLaneRecordBytes);
    }
    sendFrame(CMD_ARR_AUTOMATION_LANES, tag, body, off);
}

void SpsHostArrBridge::onReqArrAutomationPoints(uint8_t tag,
                                                const uint8_t* b,
                                                uint16_t n) {
    if (n < 9) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }

    uint32_t pointOffset = spsArrGetU32(b + 0);
    uint16_t pointCount = spsArrGetU16(b + 4);
    uint16_t skip = spsArrGetU16(b + 6);
    uint8_t maxPoints = b[8];
    if (maxPoints == 0 ||
        maxPoints > (uint8_t)spsarr::kMaxArrAutomationPointRecordsPerFrame) {
        maxPoints = (uint8_t)spsarr::kMaxArrAutomationPointRecordsPerFrame;
    }

    mclarrfile::AutomationPoint
        points[spsarr::kMaxArrAutomationPointRecordsPerFrame];
    uint32_t total = 0;
    bool more = false;
    uint16_t count = mcl_arrangement.readAutomationPoints(
        pointOffset, pointCount, skip, maxPoints, points, &total, &more);

    uint8_t body[16 + spsarr::kMaxArrAutomationPointRecordsPerFrame *
                          spsarr::kArrAutomationPointRecordBytes];
    body[0] = mcl_cfg.active_arrangement_idx;
    body[1] = more ? 1 : 0;
    spsArrPutU16(body + 2, skip);
    spsArrPutU32(body + 4, total);
    spsArrPutU32(body + 8, pointOffset);
    spsArrPutU16(body + 12, pointCount);
    body[14] = 0;
    body[15] = 0;
    uint16_t off = 16;
    for (uint16_t i = 0; i < count; ++i) {
        putArrAutomationPoint(body + off, points[i]);
        off = (uint16_t)(off + spsarr::kArrAutomationPointRecordBytes);
    }
    sendFrame(CMD_ARR_AUTOMATION_POINTS, tag, body, off);
}

void SpsHostArrBridge::onReqArrLocalPreview(uint8_t tag, const uint8_t* b,
                                            uint16_t n) {
    if (n < 4) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }

    uint32_t sourceId = spsArrGetU32(b + 0);
    uint8_t trackType = EMPTY_TRACK_TYPE;
    uint8_t length = 16;
    uint8_t speed = 0;
    uint64_t trigMask = 0;
    uint8_t noteCount = 0;
    uint8_t noteMin = 0;
    uint8_t noteMax = 0;
    uint8_t noteFlags = 0;
    MCLArrangement::PrivateSourcePreviewNote
        notes[spsarr::kArrLocalPreviewMaxNoteRecords] = {};
    int8_t trigTiming[spsarr::kArrLocalPreviewMaxTrigTimingRecords] = {};
    bool available = mcl_arrangement.privateSourcePreview(
        sourceId, &trackType, &length, &speed, &trigMask, &noteCount, &noteMin,
        &noteMax, &noteFlags, notes,
        (uint8_t)spsarr::kArrLocalPreviewMaxNoteRecords, trigTiming,
        (uint8_t)spsarr::kArrLocalPreviewMaxTrigTimingRecords);

    uint8_t body[spsarr::kArrLocalPreviewBytes +
                 spsarr::kArrLocalPreviewNoteHeaderBytes +
                 spsarr::kArrLocalPreviewMaxNoteRecords *
                     spsarr::kArrLocalPreviewNoteRecordBytes +
                 spsarr::kArrLocalPreviewNoteTimingBytes +
                 spsarr::kArrLocalPreviewMaxTrigTimingRecords] = {};
    spsArrPutU32(body + 0, sourceId);
    body[4] = available ? 1 : 0;
    body[5] = trackType;
    body[6] = length;
    body[7] = speed;
    spsArrPutU64(body + 8, trigMask);
    uint16_t bodyLen = spsarr::kArrLocalPreviewBytes;
    bool hasTrigTiming = arrPreviewTimingHasValues(
        trigTiming, (uint8_t)spsarr::kArrLocalPreviewMaxTrigTimingRecords);
    if (hasTrigTiming)
        noteFlags |= spsarr::kArrLocalPreviewNoteFlagTrigMicrotiming;
    bool hasNoteTiming =
        (noteFlags & spsarr::kArrLocalPreviewNoteFlagMicrotiming) != 0;
    if (available && (noteCount > 0 || hasTrigTiming)) {
        if (noteCount > spsarr::kArrLocalPreviewMaxNoteRecords) {
            noteCount = spsarr::kArrLocalPreviewMaxNoteRecords;
        }
        uint16_t off = spsarr::kArrLocalPreviewBytes;
        body[off++] = noteCount;
        body[off++] = noteMin;
        body[off++] = noteMax;
        body[off++] = noteFlags;
        for (uint8_t i = 0; i < noteCount; i++) {
            body[off++] = notes[i].start;
            body[off++] = notes[i].length;
            body[off++] = notes[i].note;
            body[off++] = notes[i].velocity;
        }
        if (hasNoteTiming) {
            for (uint8_t i = 0; i < noteCount; i++)
                body[off++] = (uint8_t)notes[i].timing;
        }
        if (hasTrigTiming) {
            for (uint8_t i = 0;
                 i < spsarr::kArrLocalPreviewMaxTrigTimingRecords; i++) {
                body[off++] = (uint8_t)trigTiming[i];
            }
        }
        bodyLen = off;
    }
    sendFrame(CMD_ARR_LOCAL_PREVIEW, tag, body, bodyLen);
}

void SpsHostArrBridge::onReqArrTrackLabels(uint8_t tag) {
    char labels[mclarrfile::kTrackLabelCount][mclarrfile::kTrackLabelBytes];
    if (!mcl_arrangement.readTrackLabels(labels)) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }

    uint8_t body[4 + spsarr::kArrTrackLabelCount *
                         spsarr::kArrTrackLabelBytes];
    body[0] = mcl_cfg.active_arrangement_idx;
    body[1] = spsarr::kArrTrackLabelCount;
    body[2] = spsarr::kArrTrackLabelBytes;
    body[3] = 0;
    uint16_t off = 4;
    for (uint8_t track = 0; track < spsarr::kArrTrackLabelCount; ++track) {
        for (uint8_t i = 0; i < spsarr::kArrTrackLabelBytes; ++i)
            body[off++] = (uint8_t)labels[track][i];
    }
    sendFrame(CMD_ARR_TRACK_LABELS, tag, body, off);
}

void SpsHostArrBridge::onLoadSlots(uint8_t tag, const uint8_t* b, uint16_t n) {
    if (load_slots_service_active_ && tag == load_slots_service_tag_) {
        uint8_t ack[2] = {CMD_LOAD_SLOTS, 1};
        sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
        return;
    }
    if (n < 24) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }

    uint8_t mode = b[0];
    uint8_t flags = b[1];
    uint32_t startStep = spsArrGetU32(b + 2);
    uint16_t trackMask = spsArrGetU16(b + 6);
    bool usePrivateSources = (flags & ARR_LOAD_PRIVATE_SOURCES) != 0;
#if defined(__AVR__)
    if (usePrivateSources) {
        sendErr(tag, ERR_UNSUPPORTED, flags);
        return;
    }
#endif
    GridIndex gridBank = 0;
    GridSlot loadOffset =
        (n >= 25 && b[24] < NUM_SLOTS) ? (GridSlot)b[24] : (GridSlot)255;
    if ((flags & ARR_LOAD_GRID_BANK) != 0) {
        if (n < 26 || b[25] >= NUM_GRIDS) {
            sendErr(tag, ERR_RANGE, n);
            return;
        }
        gridBank = (GridIndex)b[25];
    }
    if (mode < ARR_LOAD_MANUAL || mode > ARR_LOAD_ARRANG) {
        sendErr(tag, ERR_UNSUPPORTED, mode);
        return;
    }

    if ((flags & ARR_LOAD_GROUP_SELECT) != 0) {
        GridRow row = b[8];
        if (row >= GRID_LENGTH || !trackMask) {
            sendErr(tag, ERR_RANGE, row);
            return;
        }
        uint8_t oldMode = mcl_cfg.load_mode;
        uint16_t oldGroupMask = mcl_cfg.track_type_select;
        uint8_t trackSelect[NUM_SLOTS];
        memset(trackSelect, 0, sizeof(trackSelect));
        mcl_cfg.load_mode = mode;
        mcl_cfg.track_type_select = trackMask;
        grid_load_page.track_select_array_from_type_select(trackSelect);
        mcl_cfg.track_type_select = oldGroupMask;
        mcl_cfg.load_mode = oldMode;

        GridRow rowSelect[NUM_SLOTS];
        memset(rowSelect, 255, sizeof(rowSelect));
        bool any = false;
        for (uint8_t slot = 0; slot < NUM_SLOTS; ++slot) {
            if (trackSelect[slot] == 0)
                continue;

            EmptyTrack scratch;
            DeviceTrack* tr = scratch.load_from_grid_512(slot, row);
            if (!tr || !tr->is_active())
                continue;

            rowSelect[slot] = row;
            any = true;
        }

        uint8_t queueMode = hostLoadQueueMode(mode, flags);
        if (any) {
            releaseHostLoadedArrangementTracks(mode, rowSelect, loadOffset);
            grid_task.load_queue.put(queueMode, rowSelect, loadOffset);
        }

        uint8_t ack[2] = {CMD_LOAD_SLOTS, any ? (uint8_t)1 : (uint8_t)0};
        sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
        return;
    }

    GridRow rowSelect[NUM_SLOTS];
    memset(rowSelect, 255, sizeof(rowSelect));
#if !defined(__AVR__)
    uint32_t privateSourceIds[NUM_SLOTS];
    memset(privateSourceIds, 0, sizeof(privateSourceIds));
#endif
    bool any = false;
    bool allowClear = (flags & ARR_LOAD_CLEAR_EMPTY) != 0;
    for (uint8_t slot = 0; slot < NUM_SLOTS && slot < spsarr::kNumTracks; slot++) {
        if (((trackMask >> slot) & 1u) == 0)
            continue;
        uint8_t row = b[8 + slot];
        GridSlot sourceSlot = visibleSlotToGridSlot(slot, gridBank);
        if (sourceSlot >= NUM_SLOTS)
            continue;
        if (row < GRID_LENGTH) {
            rowSelect[sourceSlot] = row;
            any = true;
            continue;
        }
#if !defined(__AVR__)
        if (usePrivateSources && row == spsarr::kArrPrivateSourceRow) {
            rowSelect[sourceSlot] = LOAD_QUEUE_PRIVATE_ROW;
            any = true;
            continue;
        }
#endif
        if (allowClear && row == 255) {
            rowSelect[sourceSlot] = LOAD_QUEUE_CLEAR_ROW;
            any = true;
        }
    }

    if (!any) {
        sendErr(tag, ERR_RANGE, 1);
        return;
    }

    if ((flags & (ARR_LOAD_START_TRANSPORT | ARR_LOAD_SEEK_POSITION)) != 0) {
        static constexpr uint32_t kHostTicksPer16th = 6u * 16u;
        uint32_t tick96 = startStep > 0xFFFFFFFFu / kHostTicksPer16th
                              ? 0xFFFFFFFFu
                              : startStep * kHostTicksPer16th;
        MidiClock.set_transport_position(tick96);
        mcl_seq.set_transport_position(tick96);
        if (mode == ARR_LOAD_ARRANG)
            mcl_arrangement.setHostPlaybackSuspended(false);
        mcl_arrangement.resetPlaybackForTransport(
            (flags & ARR_LOAD_SEEK_POSITION) != 0);
    }

    uint32_t positionQ12 = startStep > 0xFFFFFFFFu / 12u
                               ? 0xFFFFFFFFu
                               : startStep * 12u;
    bool isArrangerLoad = mode == ARR_LOAD_ARRANG;
    uint16_t payloadOff = (flags & ARR_LOAD_GRID_BANK) != 0
                              ? 26
                              : (n >= 25 ? 25 : 24);

#if !defined(__AVR__)
    if (usePrivateSources) {
        if (payloadOff + 2 > n) {
            sendErr(tag, ERR_RANGE, n);
            return;
        }
        uint16_t privateMask = spsArrGetU16(b + payloadOff) & trackMask;
        payloadOff += 2;
        for (uint8_t slot = 0;
             slot < NUM_SLOTS && slot < spsarr::kNumTracks; slot++) {
            GridSlot sourceSlot = visibleSlotToGridSlot(slot, gridBank);
            if (sourceSlot >= NUM_SLOTS)
                continue;
            if (((privateMask >> slot) & 1u) != 0) {
                if (payloadOff + 4 > n ||
                    rowSelect[sourceSlot] != LOAD_QUEUE_PRIVATE_ROW) {
                    sendErr(tag, ERR_RANGE, slot);
                    return;
                }
                uint32_t sourceId = spsArrGetU32(b + payloadOff);
                payloadOff += 4;
                if (sourceId == 0) {
                    sendErr(tag, ERR_RANGE, slot);
                    return;
                }
                privateSourceIds[sourceSlot] = sourceId;
            } else if (rowSelect[sourceSlot] == LOAD_QUEUE_PRIVATE_ROW) {
                sendErr(tag, ERR_RANGE, slot);
                return;
            }
        }
    }
#endif

    if (isArrangerLoad) {
#if !defined(__AVR__)
        mcl_arrangement.armRuntimeForHostLoad(positionQ12, rowSelect,
                                              trackMask, loadOffset, gridBank,
                                              privateSourceIds);
#else
        mcl_arrangement.armRuntimeForHostLoad(positionQ12, rowSelect,
                                              trackMask, loadOffset, gridBank);
#endif
    }

    if (isArrangerLoad && (flags & ARR_LOAD_RUNTIME_FADES) != 0) {
        if (payloadOff + 2 <= n) {
            uint16_t fadeMask = spsArrGetU16(b + payloadOff) & trackMask;
            payloadOff += 2;
            GridSlot firstSource = 255;
            for (uint8_t src = 0; src < GRID_WIDTH && src < spsarr::kNumTracks;
                 ++src) {
                GridSlot sourceSlot = visibleSlotToGridSlot(src, gridBank);
                if (sourceSlot < NUM_SLOTS &&
                    ((trackMask >> src) & 1u) != 0 &&
                    (rowSelect[sourceSlot] < GRID_LENGTH ||
                     rowSelect[sourceSlot] == LOAD_QUEUE_PRIVATE_ROW)) {
                    firstSource = sourceSlot;
                    break;
                }
            }
            for (uint8_t src = 0; src < GRID_WIDTH && src < spsarr::kNumTracks;
                 ++src) {
                if (((fadeMask >> src) & 1u) == 0)
                    continue;
                GridSlot sourceSlot = visibleSlotToGridSlot(src, gridBank);
                if (sourceSlot >= NUM_SLOTS)
                    continue;
                if (payloadOff + spsarr::kArrClipFadeBytes > n)
                    break;
                TrackLoadFadeData fade;
                fade.flags = b[payloadOff] & 0x7F;
                fade.target = b[payloadOff + 1];
                fade.duration_q12 = spsArrGetU16(b + payloadOff + 2);
                fade.amount = b[payloadOff + 4] & 0x7F;
                fade.curve = (int8_t)b[payloadOff + 5];
                fade.reserved[0] = b[payloadOff + 6];
                fade.reserved[1] = b[payloadOff + 7];
                payloadOff += spsarr::kArrClipFadeBytes;

                GridSlot dst = sourceSlot;
                if ((rowSelect[sourceSlot] < GRID_LENGTH ||
                     rowSelect[sourceSlot] == LOAD_QUEUE_PRIVATE_ROW) &&
                    loadOffset < NUM_SLOTS) {
                    if (firstSource == 255)
                        continue;
                    int mapped =
                        (int)sourceSlot - (int)firstSource + (int)loadOffset;
                    if (mapped < 0 || mapped >= (int)NUM_SLOTS)
                        continue;
                    dst = (GridSlot)mapped;
                }
                mcl_arrangement.armRuntimeFade(dst, fade);
            }
        }
    }

    uint8_t queueMode = hostLoadQueueMode(mode, flags);
    releaseHostLoadedArrangementTracks(mode, rowSelect, loadOffset);
#if !defined(__AVR__)
    if (isArrangerLoad || usePrivateSources) {
        grid_task.load_queue.put_arrangement(queueMode, rowSelect,
                                             privateSourceIds, loadOffset);
    } else {
        grid_task.load_queue.put(queueMode, rowSelect, loadOffset);
    }
#else
    grid_task.load_queue.put(queueMode, rowSelect, loadOffset);
#endif
    uint8_t ack[2] = {CMD_LOAD_SLOTS, 1};
    sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);

    if ((flags & (ARR_LOAD_START_TRANSPORT | ARR_LOAD_SEEK_POSITION)) != 0) {
        load_slots_service_active_ = true;
        load_slots_service_tag_ = tag;
        grid_task.service_host_arranger_load_before_edit();
        load_slots_service_active_ = false;
        if (isArrangerLoad) {
            mcl_arrangement.setHostPlaybackSuspended(true);
        }
    }
}

void SpsHostArrBridge::onSaveSlots(uint8_t tag, const uint8_t* b,
                                   uint16_t n) {
    if (n < 4) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }

    uint8_t flags = b[0];
    GridRow row = b[1];
    uint16_t mask = spsArrGetU16(b + 2);
    GridIndex gridBank = n >= 5 ? sanitizeGridBank(b[4]) : (GridIndex)0;
    if (row >= GRID_LENGTH || !mask) {
        sendErr(tag, ERR_RANGE, row);
        return;
    }

    uint8_t trackSelect[NUM_SLOTS];
    memset(trackSelect, 0, sizeof(trackSelect));
    if ((flags & GRID_SAVE_GROUP_SELECT) != 0) {
        uint16_t oldGroupMask = mcl_cfg.track_type_select;
        mcl_cfg.track_type_select = mask;
        grid_save_page.track_select_array_from_type_select(trackSelect);
        mcl_cfg.track_type_select = oldGroupMask;
    } else {
        for (uint8_t slot = 0;
             slot < NUM_SLOTS && slot < spsarr::kNumTracks; ++slot) {
            if ((mask & (uint16_t)(1u << slot)) != 0) {
                GridSlot targetSlot = visibleSlotToGridSlot(slot, gridBank);
                if (targetSlot < NUM_SLOTS)
                    trackSelect[targetSlot] = 1;
            }
        }
    }

    bool any = false;
    for (uint8_t slot = 0; slot < NUM_SLOTS; ++slot) {
        if (trackSelect[slot] != 0) {
            any = true;
            break;
        }
    }
    if (!any) {
        sendErr(tag, ERR_RANGE, 1);
        return;
    }

#if defined(__AVR__)
    if (saveNeedsMdCurrentPattern())
        MD.getCurrentPattern(kCurrentPatternTimeoutMs);

    mcl_actions.save_tracks(row, trackSelect, SAVE_SEQ);
    grid_page.row_scan = GRID_LENGTH;
    grid_page.reload_slot_models = false;
#else
    if (!grid_task.save_queue.put(row, trackSelect, SAVE_SEQ, tag, true)) {
        sendErr(tag, ERR_BUSY, row);
        return;
    }
    return;
#endif
    uint8_t ack[2] = {CMD_SAVE_SLOTS, 1};
    sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
#if defined(__AVR__)
    notifyDirty(0xFF, DIRTY_CELLS);
#endif
}

void SpsHostArrBridge::onArrClear(uint8_t tag) {
    if (!mcl_arrangement.clearActive()) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }
    mcl_arrangement.resetPlayback();
    uint8_t ack[2] = {CMD_ARR_CLEAR, 1};
    sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
    notifyDirty(0xFF, DIRTY_ARRANGEMENT);
    onReqArrMeta(tag);
}

void SpsHostArrBridge::onArrImportGrid(uint8_t tag, const uint8_t* b,
                                       uint16_t n) {
    uint16_t trackMask = 0xFFFF;
    uint8_t startRow = 255;
    if (n >= 2) {
        trackMask = spsArrGetU16(b);
    }
    if (n >= 3) {
        startRow = b[2];
    }
    if (!mcl_arrangement.importGrid(trackMask, startRow)) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }
    mcl_arrangement.resetPlayback();
    uint8_t ack[2] = {CMD_ARR_IMPORT_GRID, 1};
    sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
    notifyDirty(0xFF, DIRTY_ARRANGEMENT);
    onReqArrMeta(tag);
}

void SpsHostArrBridge::onArrSelect(uint8_t tag, const uint8_t* b,
                                   uint16_t n) {
    if (n < 1) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }
    if (!mcl_arrangement.select(b[0])) {
        sendErr(tag, ERR_RANGE, b[0]);
        return;
    }
    mcl_arrangement.resetPlayback();
    mcl_arrangement.seekLoadCurrentPosition(true, true);
    uint8_t ack[2] = {CMD_ARR_SELECT, 1};
    sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
    notifyDirty(0xFF, (uint8_t)(DIRTY_ARRANGEMENT | DIRTY_ACTIVE));
    onReqArrMeta(tag);
}

void SpsHostArrBridge::onArrNew(uint8_t tag) {
    uint8_t idx = 0;
    if (!mcl_arrangement.createFirst(&idx) || !mcl_arrangement.select(idx)) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }
    mcl_arrangement.resetPlayback();
    uint8_t ack[2] = {CMD_ARR_NEW, idx};
    sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
    notifyDirty(0xFF, DIRTY_ARRANGEMENT);
    onReqArrMeta(tag);
}

void SpsHostArrBridge::onArrSave(uint8_t tag) {
    if (!mcl_arrangement.saveActive()) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }
    uint8_t ack[2] = {CMD_ARR_SAVE, 1};
    sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
    onReqArrMeta(tag);
}

void SpsHostArrBridge::onSetArrMarker(uint8_t tag, const uint8_t* b,
                                      uint16_t n) {
    if (n < spsarr::kArrMarkerRecordBytes) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }

    uint32_t startQ12 = spsArrGetU32(b + 0);
    uint8_t track = b[4];
    uint8_t flags = b[5];
    if (track != spsarr::kArrMarkerGlobalTrack &&
        track >= spsarr::kNumGridSlots) {
        sendErr(tag, ERR_BAD_TRACK, track);
        return;
    }

    char label[spsarr::kArrMarkerLabelBytes + 1] = {};
    if ((flags & mclarrfile::MARKER_LABEL) != 0) {
        for (uint8_t i = 0; i < spsarr::kArrMarkerLabelBytes; i++) {
            uint8_t c = b[6 + i];
            if (c == 0)
                break;
            label[i] = labelChar((char)c);
        }
    }

    if (!mcl_arrangement.setMarkerLabel(startQ12, track, label)) {
        sendErr(tag, ERR_BUSY, track);
        return;
    }

    uint8_t ack[2] = {CMD_SET_ARR_MARKER, 1};
    sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
    notifyDirty(0xFF, DIRTY_ARRANGEMENT);
}

void SpsHostArrBridge::onSetArrLoopRegion(uint8_t tag, const uint8_t* b,
                                          uint16_t n) {
    if (n < spsarr::kArrLoopRegionRecordBytes) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }

    mclarrfile::LoopRegion region;
    memset(&region, 0, sizeof(region));
    region.startQ12 = spsArrGetU32(b + 0);
    region.endQ12 = spsArrGetU32(b + 4);
    region.repeatCount = spsArrGetU16(b + 8);
    region.id = spsArrGetU16(b + 10);
    region.flags = b[12];
    for (uint8_t i = 0; i < spsarr::kArrLoopRegionLabelBytes; i++) {
        uint8_t c = b[16 + i];
        if (c == 0)
            break;
        region.label[i] = labelChar((char)c);
    }

    bool enabled =
        (region.flags & mclarrfile::LOOP_REGION_ENABLED) != 0;
    if (enabled &&
        (region.endQ12 <= region.startQ12 ||
         region.endQ12 - region.startQ12 < spsarr::kMinArrLoopQ12)) {
        sendErr(tag, ERR_RANGE, 1);
        return;
    }

    if (!mcl_arrangement.setLoopRegionRecord(region)) {
        sendErr(tag, ERR_BUSY, 0);
        return;
    }

    uint8_t ack[2] = {CMD_SET_ARR_LOOP_REGION, 1};
    sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
    notifyDirty(0xFF, DIRTY_ARRANGEMENT);
}

void SpsHostArrBridge::onSetArrAutomationLane(uint8_t tag, const uint8_t* b,
                                              uint16_t n) {
    if (n < spsarr::kArrAutomationLaneRecordBytes) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }

    mclarrfile::AutomationLane lane;
    getArrAutomationLane(b, lane);
    if (!automationLaneHeaderValid(
            lane, spsarr::kMaxArrAutomationPointRecordsPerFrame) ||
        n < spsarr::kArrAutomationLaneRecordBytes +
                lane.pointCount * spsarr::kArrAutomationPointRecordBytes) {
        sendErr(tag, ERR_RANGE, lane.track);
        return;
    }

    mclarrfile::AutomationPoint
        points[spsarr::kMaxArrAutomationPointRecordsPerFrame];
    uint16_t off = spsarr::kArrAutomationLaneRecordBytes;
    for (uint16_t i = 0; i < lane.pointCount; ++i) {
        getArrAutomationPoint(b + off, points[i]);
        off = (uint16_t)(off + spsarr::kArrAutomationPointRecordBytes);
    }

    if (!mcl_arrangement.setAutomationLanePoints(lane, points,
                                                 lane.pointCount)) {
        sendErr(tag, ERR_BUSY, lane.track);
        return;
    }

    uint8_t ack[2] = {CMD_SET_ARR_AUTOMATION_LANE, 1};
    sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
    notifyDirty(0xFF, DIRTY_ARRANGEMENT);
}

void SpsHostArrBridge::onSetArrAutomationLaneChunk(uint8_t tag,
                                                   const uint8_t* b,
                                                   uint16_t n) {
    if (n < 2) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }

    uint8_t op = b[0];
    if (op == ARR_AUTOMATION_WRITE_BEGIN) {
        if (n < spsarr::kArrAutomationChunkBeginBytes) {
            sendErr(tag, ERR_RANGE, op);
            return;
        }

        mclarrfile::AutomationLane lane;
        getArrAutomationLane(b + 2, lane);
        lane.pointOffset = 0;
        if (!automationLaneHeaderValid(
                lane, mclarrfile::kMaxAutomationPoints)) {
            sendErr(tag, ERR_RANGE, lane.track);
            return;
        }

        if (!beginAutomationStage(lane)) {
            sendErr(tag, ERR_BUSY, op);
            return;
        }
        uint8_t ack[2] = {CMD_SET_ARR_AUTOMATION_LANE_CHUNK, op};
        sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
        return;
    }

    if (op == ARR_AUTOMATION_WRITE_ABORT) {
        clearAutomationStage();
        uint8_t ack[2] = {CMD_SET_ARR_AUTOMATION_LANE_CHUNK, op};
        sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
        return;
    }

    if (!automation_stage_active_) {
        sendErr(tag, ERR_BUSY, op);
        return;
    }

    if (op == ARR_AUTOMATION_WRITE_POINTS) {
        if (n < spsarr::kArrAutomationChunkPointHeaderBytes) {
            sendErr(tag, ERR_RANGE, op);
            return;
        }
        uint16_t offset = spsArrGetU16(b + 2);
        uint8_t count = b[4];
        if (count > spsarr::kMaxArrAutomationChunkPointRecordsPerFrame ||
            n < spsarr::kArrAutomationChunkPointHeaderBytes +
                    count * spsarr::kArrAutomationPointRecordBytes ||
            offset != automation_stage_received_ ||
            (uint32_t)offset + count > automation_stage_total_) {
            sendErr(tag, ERR_RANGE, op);
            return;
        }
        if (count > 0 && automation_stage_points_ == nullptr) {
            sendErr(tag, ERR_BUSY, op);
            return;
        }

        uint16_t bodyOff = spsarr::kArrAutomationChunkPointHeaderBytes;
        for (uint8_t i = 0; i < count; ++i) {
            getArrAutomationPoint(
                b + bodyOff,
                automation_stage_points_[automation_stage_received_ + i]);
            bodyOff = (uint16_t)(bodyOff +
                                 spsarr::kArrAutomationPointRecordBytes);
        }
        automation_stage_received_ =
            (uint16_t)(automation_stage_received_ + count);
        uint8_t ack[2] = {CMD_SET_ARR_AUTOMATION_LANE_CHUNK, op};
        sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
        return;
    }

    if (op == ARR_AUTOMATION_WRITE_COMMIT) {
        if (automation_stage_received_ != automation_stage_total_) {
            sendErr(tag, ERR_RANGE, op);
            return;
        }
        bool ok = mcl_arrangement.setAutomationLanePoints(
            automation_stage_lane_, automation_stage_points_,
            automation_stage_total_);
        clearAutomationStage();
        if (!ok) {
            sendErr(tag, ERR_BUSY, op);
            return;
        }

        uint8_t ack[2] = {CMD_SET_ARR_AUTOMATION_LANE_CHUNK, op};
        sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
        notifyDirty(0xFF, DIRTY_ARRANGEMENT);
        return;
    }

    sendErr(tag, ERR_RANGE, op);
}

void SpsHostArrBridge::onSetArrClips(uint8_t tag, const uint8_t* b,
                                     uint16_t n) {
    if (n < 2) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }

    uint8_t op = b[0];
    if (op == ARR_CLIP_WRITE_BEGIN) {
        if (n < spsarr::kArrClipChunkBeginBytes) {
            sendErr(tag, ERR_RANGE, op);
            return;
        }
        uint16_t total = spsArrGetU16(b + 2);
        if (!beginClipStage(total)) {
            sendErr(tag, ERR_BUSY, op);
            return;
        }
        uint8_t ack[2] = {CMD_SET_ARR_CLIPS, op};
        sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
        return;
    }

    if (op == ARR_CLIP_WRITE_ABORT) {
        clearClipStage();
        uint8_t ack[2] = {CMD_SET_ARR_CLIPS, op};
        sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
        return;
    }

    if (!clip_stage_active_) {
        sendErr(tag, ERR_BUSY, op);
        return;
    }

    if (op == ARR_CLIP_WRITE_RECORDS) {
        if (n < spsarr::kArrClipChunkRecordHeaderBytes) {
            sendErr(tag, ERR_RANGE, op);
            return;
        }
        uint16_t offset = spsArrGetU16(b + 2);
        uint8_t count = b[4];
        if (count > spsarr::kMaxArrClipChunkRecordsPerFrame ||
            n < spsarr::kArrClipChunkRecordHeaderBytes +
                    count * spsarr::kArrClipRecordBytes ||
            offset != clip_stage_received_ ||
            (uint32_t)offset + count > clip_stage_total_) {
            sendErr(tag, ERR_RANGE, op);
            return;
        }
        if (count > 0 && clip_stage_clips_ == nullptr) {
            sendErr(tag, ERR_BUSY, op);
            return;
        }

        uint16_t bodyOff = spsarr::kArrClipChunkRecordHeaderBytes;
        for (uint8_t i = 0; i < count; ++i) {
            getArrClip(b + bodyOff,
                       clip_stage_clips_[clip_stage_received_ + i]);
            bodyOff = (uint16_t)(bodyOff + spsarr::kArrClipRecordBytes);
        }
        clip_stage_received_ = (uint16_t)(clip_stage_received_ + count);
        uint8_t ack[2] = {CMD_SET_ARR_CLIPS, op};
        sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
        return;
    }

    if (op == ARR_CLIP_WRITE_COMMIT) {
        if (clip_stage_received_ != clip_stage_total_) {
            sendErr(tag, ERR_RANGE, op);
            return;
        }
        bool ok = mcl_arrangement.replaceClips(clip_stage_clips_,
                                               clip_stage_total_);
        clearClipStage();
        if (!ok) {
            sendErr(tag, ERR_BUSY, op);
            return;
        }

        uint8_t ack[2] = {CMD_SET_ARR_CLIPS, op};
        sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
        notifyDirty(0xFF, (uint8_t)(DIRTY_ARRANGEMENT | DIRTY_ACTIVE));
        return;
    }

    sendErr(tag, ERR_RANGE, op);
}

void SpsHostArrBridge::onSetArrTrackLabel(uint8_t tag, const uint8_t* b,
                                          uint16_t n) {
    if (n < 1 + spsarr::kArrTrackLabelBytes) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }
    uint8_t track = b[0];
    if (track >= spsarr::kNumTracks) {
        sendErr(tag, ERR_BAD_TRACK, track);
        return;
    }

    char label[spsarr::kArrTrackLabelBytes + 1] = {};
    for (uint8_t i = 0; i < spsarr::kArrTrackLabelBytes; i++) {
        uint8_t c = b[1 + i];
        if (c == 0)
            break;
        label[i] = labelChar((char)c);
    }

    if (!mcl_arrangement.setTrackLabel(track, label)) {
        sendErr(tag, ERR_BUSY, track);
        return;
    }

    uint8_t ack[2] = {CMD_SET_ARR_TRACK_LABEL, 1};
    sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
    notifyDirty(0xFF, DIRTY_ARRANGEMENT);
}

void SpsHostArrBridge::onSetArrClipFade(uint8_t tag, const uint8_t* b,
                                        uint16_t n) {
    if (n < 18) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }

    uint32_t startQ12 = spsArrGetU32(b + 0);
    uint32_t durationQ12 = spsArrGetU32(b + 4);
    uint8_t track = b[8];
    uint8_t row = b[9];
    bool overrideFade = b[10] != 0;
    if (track >= spsarr::kNumGridSlots || row >= GRID_LENGTH ||
        durationQ12 == 0) {
        sendErr(tag, ERR_RANGE, track);
        return;
    }

    TrackLoadFadeData fade;
    fade.flags = b[11] & 0x7F;
    fade.target = b[12];
    fade.duration_q12 = spsArrGetU16(b + 13);
    fade.amount = b[15] & 0x7F;
    fade.curve = (int8_t)b[16];
    fade.reserved[0] = 0;
    fade.reserved[1] = 0;
    bool fadeOut = b[17] != 0 || (fade.flags & TRACK_LOAD_FADE_FLAG_OUT) != 0;

    if (!mcl_arrangement.setClipFade(startQ12, durationQ12, track, row,
                                     fadeOut, overrideFade, fade)) {
        sendErr(tag, ERR_BUSY, track);
        return;
    }
    if (MidiClock.state != MidiClockClass::STARTED) {
        mcl_arrangement.seekLoadCurrentPosition(true, true, false);
    }

    uint8_t ack[2] = {CMD_SET_ARR_CLIP_FADE, 1};
    sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
    notifyDirty(0xFF, DIRTY_ARRANGEMENT);
}

void SpsHostArrBridge::onArrMakeLocal(uint8_t tag, const uint8_t* b,
                                      uint16_t n) {
    if (n < 11) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }

    uint32_t startQ12 = spsArrGetU32(b + 0);
    uint32_t durationQ12 = spsArrGetU32(b + 4);
    uint8_t track = b[8];
    uint8_t row = b[9];
    uint8_t sourceSlot = b[10];
    bool allowStandaloneSource = n >= 12 && b[11] != 0;
    if (track >= spsarr::kNumGridSlots || row >= GRID_LENGTH ||
        sourceSlot >= NUM_SLOTS || durationQ12 == 0) {
        sendErr(tag, ERR_RANGE, track);
        return;
    }

    uint32_t sourceId = 0;
    bool convertedArrangementClip = mcl_arrangement.makeClipLocal(
        startQ12, durationQ12, track, row, sourceSlot, &sourceId);
    if (!convertedArrangementClip &&
        (!allowStandaloneSource ||
         !mcl_arrangement.createPrivateSourceFromGrid(sourceSlot, row, track,
                                                      &sourceId))) {
        sendErr(tag, ERR_BUSY, track);
        return;
    }

    uint8_t ack[18] = {CMD_ARR_MAKE_LOCAL, 1};
    spsArrPutU32(ack + 2, startQ12);
    spsArrPutU32(ack + 6, durationQ12);
    ack[10] = track;
    ack[11] = row;
    ack[12] = sourceSlot;
    spsArrPutU32(ack + 13, sourceId);
    ack[17] = mclarrfile::encodePrivateSourceSlot(track);
    sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
    notifyDirty(0xFF, DIRTY_ARRANGEMENT);
}

void SpsHostArrBridge::onArrDuplicateLocalSource(uint8_t tag,
                                                 const uint8_t* b,
                                                 uint16_t n) {
    if (n < 16) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }

    uint32_t oldSourceId = spsArrGetU32(b + 0);
    uint32_t startQ12 = spsArrGetU32(b + 4);
    uint32_t durationQ12 = spsArrGetU32(b + 8);
    uint8_t track = b[12];
    uint8_t row = b[13];
    uint8_t sourceSlot = b[14];
    uint8_t targetSlot = b[15];
    if (oldSourceId == 0 || track >= spsarr::kNumGridSlots ||
        row >= GRID_LENGTH || sourceSlot >= NUM_SLOTS ||
        targetSlot >= NUM_SLOTS || durationQ12 == 0) {
        sendErr(tag, ERR_RANGE, track);
        return;
    }

    uint32_t newSourceId = 0;
    if (!mcl_arrangement.duplicatePrivateSource(oldSourceId, sourceSlot,
                                                targetSlot, &newSourceId)) {
        sendErr(tag, ERR_BUSY, track);
        return;
    }

    uint8_t ack[18] = {CMD_ARR_MAKE_LOCAL, 1};
    spsArrPutU32(ack + 2, startQ12);
    spsArrPutU32(ack + 6, durationQ12);
    ack[10] = track;
    ack[11] = row;
    ack[12] = targetSlot;
    spsArrPutU32(ack + 13, newSourceId);
    ack[17] = mclarrfile::encodePrivateSourceSlot(targetSlot);
    sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
    notifyDirty(0xFF, DIRTY_ARRANGEMENT);
}

void SpsHostArrBridge::onArrLocalToGrid(uint8_t tag, const uint8_t* b,
                                        uint16_t n) {
    if (n < 8) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }

    uint32_t sourceId = spsArrGetU32(b + 0);
    GridSlot sourceSlot = b[4];
    GridRow sourceRow = b[5];
    GridSlot targetSlot = b[6];
    GridRow targetRow = b[7];
    if (sourceId == 0 || sourceSlot >= NUM_SLOTS ||
        sourceRow >= GRID_LENGTH || targetSlot >= NUM_SLOTS ||
        targetRow >= GRID_LENGTH) {
        sendErr(tag, ERR_RANGE, targetSlot);
        return;
    }

    ArrCell targetCell = readCell((uint8_t)(targetSlot % GRID_WIDTH),
                                  targetRow,
                                  (GridIndex)(targetSlot / GRID_WIDTH));
    if (mcl_clipboard.copy(targetSlot, targetRow, 1, 1)) {
        grid_page.slot_undo = 0;
    } else if (targetCell.active) {
        sendErr(tag, ERR_BUSY, 2);
        return;
    }

    if (!mcl_arrangement.exportPrivateSourceToGrid(sourceId, sourceSlot,
                                                   sourceRow, targetSlot,
                                                   targetRow)) {
        sendErr(tag, ERR_BUSY, 1);
        return;
    }

    grid_page.load_slot_models();
    uint8_t ack[2] = {CMD_ARR_LOCAL_TO_GRID, 1};
    sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
    notifyDirty(0xFF, DIRTY_CELLS);
}

void SpsHostArrBridge::onArrSeekLoad(uint8_t tag, const uint8_t* b,
                                     uint16_t n) {
    if (n < 5) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }

    uint32_t positionQ12 = spsArrGetU32(b + 0);
    uint8_t flags = b[4];
    if ((flags & (ARR_LOAD_START_TRANSPORT | ARR_LOAD_SEEK_POSITION)) != 0) {
        uint32_t tick96 = positionQ12 > 0xFFFFFFFFu / 8u
                              ? 0xFFFFFFFFu
                              : positionQ12 * 8u;
        MidiClock.set_transport_position(tick96);
        mcl_seq.set_transport_position(tick96);
    }
    mcl_arrangement.setHostPlaybackSuspended(false);
    mcl_arrangement.resetPlaybackForTransport();

    bool queued = mcl_arrangement.seekLoad(
        positionQ12, (flags & ARR_LOAD_IMMEDIATE) != 0,
        (flags & (ARR_LOAD_START_TRANSPORT | ARR_LOAD_SEEK_POSITION)) != 0);
    if (queued &&
        (flags & (ARR_LOAD_IMMEDIATE | ARR_LOAD_START_TRANSPORT |
                  ARR_LOAD_SEEK_POSITION)) != 0) {
        grid_task.service_host_arranger_load_before_edit();
    }
    notifyDirty(0xFF, DIRTY_ACTIVE);

    uint8_t ack[2] = {CMD_ARR_SEEK_LOAD, queued ? (uint8_t)1 : (uint8_t)0};
    sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
}

void SpsHostArrBridge::onArrSetLoop(uint8_t tag, const uint8_t* b,
                                    uint16_t n) {
    if (n < 9) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }
    bool enabled = (b[0] & ARR_LOOP_ENABLED) != 0;
    bool disarmPlayback = (b[0] & ARR_LOOP_DISARM_PLAYBACK) != 0;
    uint32_t startQ12 = spsArrGetU32(b + 1);
    uint32_t endQ12 = spsArrGetU32(b + 5);
    bool active = enabled && endQ12 > startQ12 &&
                  endQ12 - startQ12 >= spsarr::kMinArrLoopQ12;
    if (active) {
        mcl_arrangement.setLoopRegion(startQ12, endQ12);
    } else {
        mcl_arrangement.clearLoopRegion();
    }
    if (disarmPlayback)
        mcl_arrangement.setHostPlaybackSuspended(true);
    uint8_t ack[2] = {CMD_ARR_SET_LOOP, active ? (uint8_t)1 : (uint8_t)0};
    sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
}

void SpsHostArrBridge::onSetLoadSettings(uint8_t tag, const uint8_t* b,
                                         uint16_t n) {
    if (n < 3) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }

    uint8_t fields = b[0];
    if ((fields & LOAD_SETTINGS_MODE) != 0) {
        uint8_t mode = b[1];
        if (mode < ARR_LOAD_MANUAL || mode > ARR_LOAD_QUEUE) {
            sendErr(tag, ERR_RANGE, mode);
            return;
        }
        mcl_cfg.load_mode = mode;
    }
    if ((fields & LOAD_SETTINGS_QUANT) != 0) {
        uint8_t quant = b[2];
        if (quant < 1)
            quant = 1;
        if (quant > 64)
            quant = 64;
        mcl_cfg.chain_load_quant = quant;
    }
    if ((fields & LOAD_SETTINGS_QUEUE_LENGTH) != 0) {
        if (n < 4) {
            sendErr(tag, ERR_RANGE, 0);
            return;
        }
        uint8_t queueLength = b[3];
        if (queueLength < 1)
            queueLength = 1;
        if (queueLength > 64)
            queueLength = 64;
        mcl_cfg.chain_queue_length = queueLength;
    }

    uint8_t ack[2] = {CMD_SET_LOAD_SETTINGS, 1};
    sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
}

bool SpsHostArrBridge::applySetLink(const uint8_t* b, uint16_t n) {
    if (n < 7)
        return false;
    GridLink link;
    link.row = b[2] < GRID_LENGTH ? b[2] : 0;
    link.loops = b[3] & 0x7F;
    link.length = b[4] & 0x7F;
    link.speed = 0;
    link.set_speed(b[5] & 0x7F);
    return writeCellLink(b[0], b[1], link, b[6] != 0);
}

bool SpsHostArrBridge::applySetFade(const uint8_t* b, uint16_t n) {
    if (n < 8)
        return false;
    TrackLoadFadeData fade;
    fade.flags = b[2] & 0x7F;
    fade.target = b[3];
    fade.duration_q12 = spsArrGetU16(b + 4);
    fade.amount = b[6] & 0x7F;
    fade.curve = (int8_t)b[7];
    fade.reserved[0] = 0;
    fade.reserved[1] = 0;
    const bool ok = writeCellFade(b[0], b[1], fade);
    return ok;
}

#endif  // MCL_FEATURE_HOST_ARRANGER
