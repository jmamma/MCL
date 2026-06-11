#include "MCLPlatformFeatures.h"

#if MCL_FEATURE_HOST_ARRANGER

#include "Host/SpsHostArrBridge.h"
#include "SpsHostArrBridge_Internal.h"

using namespace spsarr;
using namespace sps_host_arr_internal;

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
    if (n < 24) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }

    uint8_t mode = b[0];
    uint8_t flags = b[1];
    uint32_t startStep = spsArrGetU32(b + 2);
    uint16_t trackMask = spsArrGetU16(b + 6);
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

        uint8_t ack[2] = {CMD_LOAD_SLOTS, any ? 1 : 0};
        sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
        return;
    }

    GridRow rowSelect[NUM_SLOTS];
    memset(rowSelect, 255, sizeof(rowSelect));
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
        mcl_arrangement.resetPlaybackForTransport(
            (flags & ARR_LOAD_SEEK_POSITION) != 0);
    }

    uint32_t positionQ12 = startStep > 0xFFFFFFFFu / 12u
                               ? 0xFFFFFFFFu
                               : startStep * 12u;
    bool isArrangerLoad = mode == ARR_LOAD_ARRANG;
    bool armedRuntimeFade = false;
    if (isArrangerLoad) {
        armedRuntimeFade = mcl_arrangement.armRuntimeForHostLoad(
            positionQ12, rowSelect, trackMask, loadOffset, gridBank);
    }

    if (isArrangerLoad && (flags & ARR_LOAD_RUNTIME_FADES) != 0) {
        uint16_t off = (flags & ARR_LOAD_GRID_BANK) != 0
                           ? 26
                           : (n >= 25 ? 25 : 24);
        if (off + 2 <= n) {
            uint16_t fadeMask = spsArrGetU16(b + off) & trackMask;
            off += 2;
            GridSlot firstSource = 255;
            for (uint8_t src = 0; src < GRID_WIDTH && src < spsarr::kNumTracks;
                 ++src) {
                GridSlot sourceSlot = visibleSlotToGridSlot(src, gridBank);
                if (sourceSlot < NUM_SLOTS &&
                    ((trackMask >> src) & 1u) != 0 &&
                    rowSelect[sourceSlot] < GRID_LENGTH) {
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
                if (off + spsarr::kArrClipFadeBytes > n)
                    break;
                TrackLoadFadeData fade;
                fade.flags = b[off] & 0x7F;
                fade.target = b[off + 1];
                fade.duration_q12 = spsArrGetU16(b + off + 2);
                fade.amount = b[off + 4] & 0x7F;
                fade.curve = (int8_t)b[off + 5];
                fade.reserved[0] = b[off + 6];
                fade.reserved[1] = b[off + 7];
                off += spsarr::kArrClipFadeBytes;

                GridSlot dst = sourceSlot;
                if (rowSelect[sourceSlot] < GRID_LENGTH &&
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
                armedRuntimeFade = armedRuntimeFade || fade.enabled();
                ARR_FADE_TRACE("payload fade src=%u dst=%u flags=%u dur=%u amount=%u curve=%d enabled=%u",
                               sourceSlot, dst, fade.flags,
                               fade.duration_q12,
                               fade.amount, (int)fade.curve,
                               fade.enabled() ? 1 : 0);
            }
        }
    }

    uint8_t queueMode = hostLoadQueueMode(mode, flags);
    ARR_FADE_TRACE("load-slots mode=%u qmode=%u flags=%u step=%lu immediate=%u",
                   mode, queueMode, flags, (unsigned long)startStep,
                   (flags & ARR_LOAD_IMMEDIATE) ? 1 : 0);
    if (armedRuntimeFade) {
        ARR_FADE_TRACE("armed runtime fade step=%lu q12=%lu mask=%u",
                       (unsigned long)startStep, (unsigned long)positionQ12,
                       trackMask);
    }
    releaseHostLoadedArrangementTracks(mode, rowSelect, loadOffset);
    grid_task.load_queue.put(queueMode, rowSelect, loadOffset);
    uint8_t ack[2] = {CMD_LOAD_SLOTS, 1};
    sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
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

    if (saveNeedsMdCurrentPattern())
        MD.getCurrentPattern(kCurrentPatternTimeoutMs);

    mcl_actions.save_tracks(row, trackSelect, SAVE_SEQ);
    grid_page.row_scan = GRID_LENGTH;
    grid_page.reload_slot_models = false;
    uint8_t ack[2] = {CMD_SAVE_SLOTS, 1};
    sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
    notifyDirty(0xFF, DIRTY_CELLS);
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
    uint8_t ack[2] = {CMD_ARR_SELECT, 1};
    sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
    notifyDirty(0xFF, DIRTY_ARRANGEMENT);
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
    if (track >= spsarr::kNumGridSlots || row >= GRID_LENGTH ||
        sourceSlot >= NUM_SLOTS || durationQ12 == 0) {
        sendErr(tag, ERR_RANGE, track);
        return;
    }

    if (!mcl_arrangement.makeClipLocal(startQ12, durationQ12, track, row,
                                       sourceSlot)) {
        sendErr(tag, ERR_BUSY, track);
        return;
    }

    uint8_t ack[2] = {CMD_ARR_MAKE_LOCAL, 1};
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
    mcl_arrangement.resetPlaybackForTransport();

    bool queued = mcl_arrangement.seekLoad(
        positionQ12, (flags & ARR_LOAD_IMMEDIATE) != 0,
        (flags & ARR_LOAD_START_TRANSPORT) != 0);
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
    bool enabled = (b[0] & 1u) != 0;
    uint32_t startQ12 = spsArrGetU32(b + 1);
    uint32_t endQ12 = spsArrGetU32(b + 5);
    bool active = enabled && endQ12 > startQ12 &&
                  endQ12 - startQ12 >= spsarr::kMinArrLoopQ12;
    if (active) {
        mcl_arrangement.setLoopRegion(startQ12, endQ12);
    } else {
        mcl_arrangement.clearLoopRegion();
    }
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
    ARR_FADE_TRACE("set track=%u row=%u flags=%u target=%u dur=%u amount=%u curve=%d ok=%u",
                   b[0], b[1], fade.flags, fade.target, fade.duration_q12,
                   fade.amount, (int)fade.curve, ok ? 1 : 0);
    return ok;
}

#endif  // MCL_FEATURE_HOST_ARRANGER
