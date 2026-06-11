#include "MCLPlatformFeatures.h"

#if MCL_FEATURE_HOST_ARRANGER

#include "SpsHostArrBridge_Internal.h"

using namespace spsarr;

namespace sps_host_arr_internal {

#if MCL_FEATURE_HOST_GRID_MOVE_UNDO
#define FILENAME_SPS_GRID_JOURNAL "sps_grid_journal_"
static constexpr uint8_t kSpsGridJournalDepth = 16;
#endif



#if MCL_FEATURE_HOST_GRID_MOVE_UNDO


static Grid sps_grid_journal[kSpsGridJournalDepth][NUM_GRIDS];
static uint16_t
    sps_grid_journal_masks[kSpsGridJournalDepth][NUM_GRIDS][GRID_LENGTH];
static bool sps_grid_journal_valid[kSpsGridJournalDepth];
static uint8_t sps_grid_journal_head = 0;
static uint8_t sps_grid_journal_count = 0;
#endif

uint32_t linkDurationQ12(const GridLink& link) {
    if (link.loops == 0)
        return 0;
    uint32_t q12 = (uint32_t)link.loops * link.length *
                   SeqTrack::get_speed_multiplier_int(link.speed_value());
    if (q12 < 12)
        q12 = 48;
    return q12;
}

char labelChar(char c) {
    unsigned char u = (unsigned char)c;
    return u >= 32 && u <= 126 ? c : ' ';
}

void setLabel2(char label[3], char a, char b) {
    label[0] = labelChar(a);
    label[1] = labelChar(b);
    label[2] = '\0';
}

void copyLabelPair(const char* src, char* dst) {
    dst[0] = src && src[0] ? labelChar(src[0]) : ' ';
    dst[1] = src && src[1] ? labelChar(src[1]) : ' ';
}

GridIndex sanitizeGridBank(uint8_t gridBank) {
    return gridBank < NUM_GRIDS ? (GridIndex)gridBank : (GridIndex)0;
}

GridSlot visibleSlotToGridSlot(uint8_t visibleSlot, GridIndex gridBank) {
    return (GridSlot)(visibleSlot + gridBank * GRID_WIDTH);
}

void copyRowName(GridRow row, GridIndex gridBank, uint8_t* dst) {
    if (!dst)
        return;
    for (uint8_t i = 0; i < spsarr::kRowNameBytes; i++)
        dst[i] = 0;

    GridRowHeader header;
    if (!proj.read_grid_row_header(&header, row, gridBank))
        return;
    if (gridBank == 0 && !header.active)
        return;
    if (gridBank != 0 && (!header.active || header.name[0] == '\0')) {
        GridRowHeader gridXHeader;
        if (proj.read_grid_row_header(&gridXHeader, row, 0) &&
            gridXHeader.active && gridXHeader.name[0] != '\0') {
            strncpy(header.name, gridXHeader.name, sizeof(header.name));
            header.name[sizeof(header.name) - 1] = '\0';
        }
    }
    if (header.name[0] == '\0')
        return;

    for (uint8_t i = 0; i < spsarr::kRowNameBytes && header.name[i] != '\0';
         i++) {
        dst[i] = (uint8_t)labelChar(header.name[i]);
    }
}

const char* shortNamePart(uint8_t trackType, uint8_t model,
                                 uint8_t part) {
    switch (trackType) {
        case MD_TRACK_TYPE_270:
        case MD_TRACK_TYPE:
        case MDSPSX_TRACK_TYPE:
            return getMDMachineNameShort(model, part);
        case MNM_TRACK_TYPE:
        case MNM_MIDI_TRACK_TYPE:
            return getMNMMachineNameShort(model, part);
        default:
            return nullptr;
    }
}

void populateCellLabels(ArrCell& cell, DeviceTrack* tr,
                               GridSlot slot, GridRow row) {
    setLabel2(cell.label2, '-', '-');
    cell.label4[0] = '-';
    cell.label4[1] = '-';
    cell.label4[2] = ' ';
    cell.label4[3] = ' ';
    cell.label4[4] = '\0';

    if (!tr || !tr->is_active())
        return;

    uint8_t visibleTrack = slot % GRID_WIDTH;
    GridSlotLabelContext ctx = {tr->get_model(), visibleTrack};
#if defined(PLATFORM_TBD)
    ctx.slot = slot;
    ctx.row = row;
#else
    (void)row;
#endif
    uint16_t packed = tr->grid_slot_label(ctx);
    if (packed != 0)
        setLabel2(cell.label2, (char)(packed >> 8), (char)packed);

    const char* part1 = shortNamePart(tr->active, tr->get_model(), 1);
    const char* part2 = shortNamePart(tr->active, tr->get_model(), 2);
    if (part1 || part2) {
        copyLabelPair(part1, cell.label4);
        copyLabelPair(part2, cell.label4 + 2);
    } else {
        cell.label4[0] = cell.label2[0];
        cell.label4[1] = cell.label2[1];
    }
}

void addDependencyTrack(uint16_t& mask, uint8_t track) {
    if (track < spsarr::kNumTracks)
        mask |= (uint16_t)(1u << track);
}

uint16_t directCellDependencyMask(DeviceTrack* tr) {
    uint16_t mask = 0;
    if (!tr || !tr->is_active())
        return mask;

    if (MDTrack* md = tr->as<MDTrack>()) {
        addDependencyTrack(mask, md->machine.trigGroup);
        addDependencyTrack(mask, md->machine.muteGroup);
        addDependencyTrack(mask, md->machine.lfo.destinationTrack);
        return mask;
    }

    if (SPSXTrack* spsx = tr->as<SPSXTrack>()) {
        addDependencyTrack(mask, spsx->machine.trigGroup);
        addDependencyTrack(mask, spsx->machine.muteGroup);
        for (uint8_t i = 0; i < 2; i++)
            addDependencyTrack(mask, spsx->machine.lfos[i].destinationTrack);
    }

    return mask;
}

uint16_t cellDependencyMask(uint8_t track, GridRow row,
                                   GridIndex gridBank,
                                   DeviceTrack* tr) {
    uint16_t mask = directCellDependencyMask(tr);
    uint16_t visited = track < spsarr::kNumTracks
                           ? (uint16_t)(1u << track)
                           : 0;

    for (uint8_t guard = 0; guard < spsarr::kNumTracks; guard++) {
        uint16_t pending = (uint16_t)(mask & ~visited);
        if (!pending)
            break;

        bool changed = false;
        for (uint8_t dep = 0; dep < spsarr::kNumTracks; dep++) {
            uint16_t bit = (uint16_t)(1u << dep);
            if ((pending & bit) == 0)
                continue;

            visited |= bit;
            EmptyTrack scratch;
            DeviceTrack* depTrack = scratch.load_from_grid_512(
                visibleSlotToGridSlot(dep, gridBank), row);
            uint16_t next =
                (uint16_t)(mask | directCellDependencyMask(depTrack));
            if (next != mask) {
                mask = next;
                changed = true;
            }
        }
        if (!changed)
            break;
    }

    return mask;
}

uint8_t groupSelectIndexForSlot(GridSlot slot) {
    if (slot >= NUM_SLOTS)
        return 0xFF;

    GridDeviceTrack* gdt = mcl_actions.get_grid_dev_track(slot);
    if (gdt == nullptr || !gdt->isActive())
        return 0xFF;

    switch (gdt->group_type) {
    case GROUP_DEV:
        return gdt->device_idx < 2 ? gdt->device_idx : 0xFF;
    case GROUP_PERF:
        return 2;
    case GROUP_AUX:
        return 3;
    case GROUP_TEMPO:
        return 4;
    }
    return 0xFF;
}

uint8_t hostLoadQueueMode(uint8_t mode, uint8_t flags) {
    uint8_t queueMode = mode;
    if ((flags & ARR_LOAD_IMMEDIATE) != 0) {
        queueMode |= LOAD_QUEUE_FLAG_IMMEDIATE;
    }
    if ((flags & ARR_LOAD_START_TRANSPORT) != 0) {
        queueMode |= LOAD_QUEUE_FLAG_PRESTART_FADE;
    }
    return queueMode;
}

uint32_t hostLoadDestinationMask(const GridRow rowSelect[NUM_SLOTS],
                                        GridSlot loadOffset) {
    if (rowSelect == nullptr)
        return 0;

    uint8_t loadSelect[NUM_SLOTS];
    memset(loadSelect, 0, sizeof(loadSelect));
    uint32_t clearMask = 0;
    for (uint8_t slot = 0; slot < NUM_SLOTS; ++slot) {
        GridRow row = rowSelect[slot];
        if (row < GRID_LENGTH || row == LOAD_QUEUE_PRIVATE_ROW) {
            loadSelect[slot] = 1;
        } else if (row == LOAD_QUEUE_CLEAR_ROW && slot < 32) {
            clearMask |= (uint32_t)(1ul << slot);
        }
    }
    return selected_destination_mask(loadSelect, loadOffset) | clearMask;
}

void releaseHostLoadedArrangementTracks(
    uint8_t mode, const GridRow rowSelect[NUM_SLOTS], GridSlot loadOffset) {
    if (mode == ARR_LOAD_ARRANG)
        return;

    uint32_t mask = hostLoadDestinationMask(rowSelect, loadOffset);
    if (mask == 0)
        return;

    if (mcl_arrangement.releasePlaybackTracks(mask)) {
        sps_host_arr_bridge.notifyDirty(0xFF, (uint8_t)DIRTY_ACTIVE);
    }
}

ArrCell readCell(uint8_t track, GridRow row, GridIndex gridBank) {
    ArrCell cell;
    if (track >= spsarr::kNumTracks || row >= GRID_LENGTH)
        return cell;

    GridSlot slot = visibleSlotToGridSlot(track, gridBank);
    if (slot >= NUM_SLOTS)
        return cell;

    EmptyTrack scratch;
    DeviceTrack* tr = scratch.load_from_grid_512(slot, row);
    if (!tr)
        return cell;

    cell.ok = true;
    cell.active = tr->is_active();
    cell.loadSound = tr->load_sound();
    cell.link = tr->link;
    cell.durationQ12 = linkDurationQ12(cell.link);
    populateCellLabels(cell, tr, slot, row);
    cell.dependencyMask = cellDependencyMask(track, row, gridBank, tr);
    cell.groupIndex = groupSelectIndexForSlot(slot);
    if (const TrackLoadFadeData* fade = tr->load_fade_data()) {
        cell.fade = *fade;
        cell.hasFade = fade->enabled();
    } else {
        cell.fade.init();
    }
    return cell;
}

bool writeCellLink(uint8_t track, GridRow row, const GridLink& link,
                          bool loadSound) {
    if (track >= spsarr::kNumTracks || row >= GRID_LENGTH)
        return false;
    EmptyTrack scratch;
    DeviceTrack* tr = scratch.load_from_grid_512(track, row);
    if (!tr || !tr->is_active())
        return false;
    tr->link = link;
    tr->set_load_sound(loadSound);
    return tr->write_grid(tr->_this(), tr->get_track_size(), track, row);
}

bool writeCellFade(uint8_t track, GridRow row,
                          const TrackLoadFadeData& fade) {
    if (track >= spsarr::kNumTracks || row >= GRID_LENGTH) {
        ARR_FADE_TRACE("write reject range track=%u row=%u", track, row);
        return false;
    }
    EmptyTrack scratch;
    DeviceTrack* tr = scratch.load_from_grid_512(track, row);
    if (!tr || !tr->is_active()) {
        ARR_FADE_TRACE("write reject inactive track=%u row=%u tr=%p",
                       track, row, tr);
        return false;
    }
    TrackLoadFadeData* dst = tr->load_fade_data();
    if (!dst) {
        ARR_FADE_TRACE("write reject no-data track=%u row=%u type=%u",
                       track, row, tr->active);
        return false;
    }
    *dst = fade;
    const bool ok = tr->write_grid(tr->_this(), tr->get_track_size(), track, row);
    ARR_FADE_TRACE("write track=%u row=%u type=%u flags=%u target=%u dur=%u amount=%u curve=%d ok=%u",
                   track, row, tr->active, fade.flags, fade.target,
                   fade.duration_q12, fade.amount, (int)fade.curve, ok ? 1 : 0);
    return ok;
}

GridRow activeRowOrZero() {
    return grid_task.last_active_row < GRID_LENGTH ? grid_task.last_active_row : 0;
}

void putArrClip(uint8_t* dst, const mclarrfile::Clip& clip) {
    spsArrPutU32(dst + 0, clip.startQ12);
    spsArrPutU32(dst + 4, clip.durationQ12);
    spsArrPutU32(dst + 8, clip.repeatQ12);
    dst[12] = clip.track;
    dst[13] = clip.row;
    dst[14] = clip.flags;
    dst[15] = clip.reserved;
    dst[16] = clip.fadeFlags;
    dst[17] = clip.fadeTarget;
    spsArrPutU16(dst + 18, clip.fadeDurationQ12);
    dst[20] = clip.fadeAmount;
    dst[21] = (uint8_t)clip.fadeCurve;
    spsArrPutU16(dst + 22, clip.fadeReserved);
    dst[24] = clip.endFadeFlags;
    dst[25] = clip.endFadeTarget;
    spsArrPutU16(dst + 26, clip.endFadeDurationQ12);
    dst[28] = clip.endFadeAmount;
    dst[29] = (uint8_t)clip.endFadeCurve;
    spsArrPutU16(dst + 30, clip.endFadeReserved);
    dst[32] = clip.sourceKind;
    dst[33] = clip.sourceTrack;
    dst[34] = clip.sourceFlags;
    dst[35] = clip.sourceReserved;
    spsArrPutU32(dst + 36, clip.sourceId);
}

void putArrMarker(uint8_t* dst, const mclarrfile::Marker& marker) {
    spsArrPutU32(dst + 0, marker.startQ12);
    dst[4] = marker.track;
    dst[5] = marker.flags;
    for (uint8_t i = 0; i < spsarr::kArrMarkerLabelBytes; ++i)
        dst[6 + i] = (uint8_t)marker.label[i];
    dst[22] = 0;
    dst[23] = 0;
}

void putArrLoopRegion(uint8_t* dst,
                             const mclarrfile::LoopRegion& region) {
    spsArrPutU32(dst + 0, region.startQ12);
    spsArrPutU32(dst + 4, region.endQ12);
    spsArrPutU16(dst + 8, region.repeatCount);
    spsArrPutU16(dst + 10, region.id);
    dst[12] = region.flags;
    dst[13] = region.reserved[0];
    dst[14] = region.reserved[1];
    dst[15] = region.reserved[2];
    for (uint8_t i = 0; i < spsarr::kArrLoopRegionLabelBytes; ++i)
        dst[16 + i] = (uint8_t)region.label[i];
}

bool parseGridRect(const uint8_t* b, uint16_t n, GridSlot& col,
                          GridRow& row, GridSpan& w, GridSpan& h) {
    if (n < 4)
        return false;
    GridIndex gridBank = n >= 5 ? sanitizeGridBank(b[4]) : (GridIndex)0;
    col = visibleSlotToGridSlot(b[0], gridBank);
    row = b[1];
    w = b[2];
    h = b[3];
    if (b[0] >= spsarr::kNumTracks || col >= NUM_SLOTS ||
        row >= GRID_LENGTH || w == 0 || h == 0)
        return false;
    GridSpan bankRemaining = (GridSpan)(GRID_WIDTH - (col % GRID_WIDTH));
    if (w > bankRemaining)
        w = bankRemaining;
    if ((uint16_t)row + h > GRID_LENGTH)
        h = (GridSpan)(GRID_LENGTH - row);
    return w > 0 && h > 0;
}

#if MCL_FEATURE_HOST_GRID_MOVE_UNDO
uint16_t gridMoveWidthMask(GridSpan width) {
    if (width >= GRID_WIDTH)
        return 0xFFFFu;
    return (uint16_t)((1u << width) - 1u);
}

bool parseGridMove(const uint8_t* b, uint16_t n,
                          GridMoveRequest& req) {
    if (n < 9)
        return false;

    GridIndex sourceBank = sanitizeGridBank(b[6]);
    GridIndex targetBank = sanitizeGridBank(b[7]);
    if (b[0] >= spsarr::kNumTracks || b[4] >= spsarr::kNumTracks ||
        b[1] >= GRID_LENGTH || b[5] >= GRID_LENGTH || b[2] == 0 ||
        b[3] == 0) {
        return false;
    }

    req.sourceCol = visibleSlotToGridSlot(b[0], sourceBank);
    req.sourceRow = b[1];
    req.width = b[2];
    req.height = b[3];
    req.targetCol = visibleSlotToGridSlot(b[4], targetBank);
    req.targetRow = b[5];
    req.sparse = (b[8] & 1) != 0;

    if (req.sourceCol >= NUM_SLOTS || req.targetCol >= NUM_SLOTS)
        return false;

    GridSpan sourceBankRemaining =
        (GridSpan)(GRID_WIDTH - (req.sourceCol % GRID_WIDTH));
    GridSpan targetBankRemaining =
        (GridSpan)(GRID_WIDTH - (req.targetCol % GRID_WIDTH));
    if (req.width > sourceBankRemaining)
        req.width = sourceBankRemaining;
    if (req.width > targetBankRemaining)
        req.width = targetBankRemaining;
    if ((uint16_t)req.sourceRow + req.height > GRID_LENGTH)
        req.height = (GridSpan)(GRID_LENGTH - req.sourceRow);
    if ((uint16_t)req.targetRow + req.height > GRID_LENGTH)
        req.height = (GridSpan)(GRID_LENGTH - req.targetRow);
    if (req.width == 0 || req.height == 0)
        return false;

    uint16_t fullMask = gridMoveWidthMask(req.width);
    uint16_t offset = 9;
    bool any = false;
    for (GridSpan y = 0; y < req.height; y++) {
        uint16_t mask = fullMask;
        if (req.sparse) {
            if ((uint16_t)(offset + 2) > n)
                return false;
            mask = (uint16_t)(spsArrGetU16(b + offset) & fullMask);
            offset = (uint16_t)(offset + 2);
        }
        req.rowMasks[y] = mask;
        any = any || mask != 0;
    }
    return any;
}
#endif

bool clearGridRect(GridSlot col, GridRow row, GridSpan w, GridSpan h) {
    bool ok = true;
    for (GridSpan y = 0; y < h && (uint16_t)row + y < GRID_LENGTH; y++) {
        GridRow dstRow = (GridRow)(row + y);
        uint8_t touchedGrids = 0;
        for (GridSpan x = 0; x < w && (uint16_t)col + x < NUM_SLOTS; x++) {
            GridSlot dstCol = (GridSlot)(col + x);
            touchedGrids |= (uint8_t)(1u << (dstCol / GRID_WIDTH));
            if (!proj.clear_slot_grid(dstCol, dstRow))
                ok = false;
        }

        for (uint8_t grid = 0; grid < NUM_GRIDS; grid++) {
            if ((touchedGrids & (uint8_t)(1u << grid)) == 0)
                continue;
            GridRowHeader header;
            if (proj.read_grid_row_header(&header, dstRow, grid) &&
                header.is_empty()) {
                header.active = false;
                header.name[0] = '\0';
                if (!proj.write_grid_row_header(&header, dstRow, grid))
                    ok = false;
            }
        }
    }

    for (uint8_t grid = 0; grid < NUM_GRIDS; grid++)
        proj.sync_grid(grid);
    grid_page.slot_undo = 1;
    grid_page.slot_undo_x = (GridColumn)(col & 0x0F);
    grid_page.slot_undo_y = row;
    grid_page.load_slot_models();
    return ok;
}

#if MCL_FEATURE_HOST_GRID_MOVE_UNDO
uint8_t spsGridJournalPrev(uint8_t index) {
    return index == 0 ? (uint8_t)(kSpsGridJournalDepth - 1)
                      : (uint8_t)(index - 1);
}

void resetSpsGridJournalTransaction(uint8_t tx) {
    if (tx >= kSpsGridJournalDepth)
        return;
    memset(sps_grid_journal_masks[tx], 0, sizeof(sps_grid_journal_masks[tx]));
    sps_grid_journal_valid[tx] = false;
}

bool closeSpsGridJournalFiles(uint8_t tx);

bool openSpsGridJournalFiles(uint8_t tx) {
    if (tx >= kSpsGridJournalDepth)
        return false;
#ifndef __AVR__
    SD.chdir(mcl_sd.mcl_root[0] == '\0' ? "/" : mcl_sd.mcl_root);
#else
    SD.chdir("/");
#endif
    char gridFilename[] = FILENAME_SPS_GRID_JOURNAL "00.0";
    const uint8_t prefixLen =
        (uint8_t)(sizeof(FILENAME_SPS_GRID_JOURNAL) - 1);
    gridFilename[prefixLen] = (char)('0' + (tx / 10));
    gridFilename[prefixLen + 1] = (char)('0' + (tx % 10));
    for (uint8_t i = 0; i < NUM_GRIDS; i++) {
        gridFilename[prefixLen + 3] = (char)('0' + i);
        if (!SD.exists(gridFilename)) {
            if (!sps_grid_journal[tx][i].new_file(gridFilename)) {
                closeSpsGridJournalFiles(tx);
                return false;
            }
        } else if (!sps_grid_journal[tx][i].open_file(gridFilename)) {
            closeSpsGridJournalFiles(tx);
            return false;
        }
    }
    return true;
}

bool closeSpsGridJournalFiles(uint8_t tx) {
    if (tx >= kSpsGridJournalDepth)
        return false;
    bool ok = true;
    for (uint8_t i = 0; i < NUM_GRIDS; i++)
        ok = sps_grid_journal[tx][i].close_file() && ok;
    return ok;
}

bool beginSpsGridJournalTransaction(uint8_t& tx) {
    tx = sps_grid_journal_head;
    if (sps_grid_journal_count == kSpsGridJournalDepth) {
        resetSpsGridJournalTransaction(tx);
        sps_grid_journal_count--;
    } else {
        resetSpsGridJournalTransaction(tx);
    }
    if (!openSpsGridJournalFiles(tx)) {
        resetSpsGridJournalTransaction(tx);
        return false;
    }
    return true;
}

bool commitSpsGridJournalTransaction(uint8_t tx) {
    if (tx >= kSpsGridJournalDepth || tx != sps_grid_journal_head)
        return false;
    sps_grid_journal_valid[tx] = true;
    sps_grid_journal_head = (uint8_t)((tx + 1) % kSpsGridJournalDepth);
    if (sps_grid_journal_count < kSpsGridJournalDepth)
        sps_grid_journal_count++;
    return true;
}

void discardSpsGridJournalTransaction(uint8_t tx) {
    resetSpsGridJournalTransaction(tx);
}

bool snapshotSpsGridJournalCell(uint8_t tx, GridSlot slot,
                                       GridRow row) {
    if (tx >= kSpsGridJournalDepth || slot >= NUM_SLOTS ||
        row >= GRID_LENGTH)
        return false;
    GridIndex grid = (GridIndex)(slot / GRID_WIDTH);
    GridColumn localCol = (GridColumn)(slot & 0x0F);
    uint16_t bit = (uint16_t)(1u << localCol);
    uint16_t& mask = sps_grid_journal_masks[tx][grid][row];
    if ((mask & bit) != 0)
        return true;

    if (mask == 0) {
        GridRowHeader header;
        if (!proj.read_grid_row_header(&header, row, grid) ||
            !sps_grid_journal[tx][grid].write_row_header(&header, row)) {
            return false;
        }
    }

    EmptyTrack scratch;
    DeviceTrack* track = scratch.load_from_grid_512(slot, row);
    if (!track ||
        !track->store_in_grid(localCol, row, nullptr, 0, false,
                              &sps_grid_journal[tx][grid])) {
        return false;
    }
    mask |= bit;
    return true;
}

bool prepareSpsGridMoveJournal(const GridMoveRequest& req,
                                      uint8_t& tx) {
    if (!beginSpsGridJournalTransaction(tx))
        return false;

    bool ok = true;
    for (GridSpan y = 0; y < req.height && ok; y++) {
        GridRow sourceRow = (GridRow)(req.sourceRow + y);
        GridRow targetRow = (GridRow)(req.targetRow + y);
        uint16_t mask = req.rowMasks[y];
        for (GridSpan x = 0; x < req.width && ok; x++) {
            if ((mask & (uint16_t)(1u << x)) == 0)
                continue;
            ok = snapshotSpsGridJournalCell(
                     tx, (GridSlot)(req.sourceCol + x), sourceRow) &&
                 snapshotSpsGridJournalCell(
                     tx, (GridSlot)(req.targetCol + x), targetRow);
        }
    }

    for (uint8_t grid = 0; grid < NUM_GRIDS; grid++)
        ok = sps_grid_journal[tx][grid].sync() && ok;
    ok = closeSpsGridJournalFiles(tx) && ok;
    if (!ok) {
        discardSpsGridJournalTransaction(tx);
        return false;
    }
    return true;
}

bool restoreSpsGridJournalTransaction(uint8_t tx) {
    if (tx >= kSpsGridJournalDepth)
        return false;
    if (!openSpsGridJournalFiles(tx))
        return false;

    bool ok = true;
    for (uint8_t grid = 0; grid < NUM_GRIDS; grid++) {
        for (GridRow row = 0; row < GRID_LENGTH; row++) {
            uint16_t mask = sps_grid_journal_masks[tx][grid][row];
            if (mask == 0)
                continue;
            for (GridColumn col = 0; col < GRID_WIDTH; col++) {
                if ((mask & (uint16_t)(1u << col)) == 0)
                    continue;
                EmptyTrack scratch;
                DeviceTrack* track =
                    scratch.load_from_grid_512(col, row,
                                               &sps_grid_journal[tx][grid]);
                GridSlot fullSlot = (GridSlot)(grid * GRID_WIDTH + col);
                if (!track ||
                    !track->store_in_grid(fullSlot, row, nullptr, 0, false)) {
                    ok = false;
                }
            }
            GridRowHeader header;
            if (!sps_grid_journal[tx][grid].read_row_header(&header, row) ||
                !proj.write_grid_row_header(&header, row, grid)) {
                ok = false;
            }
        }
    }
    for (uint8_t grid = 0; grid < NUM_GRIDS; grid++)
        ok = proj.sync_grid(grid) && ok;
    ok = closeSpsGridJournalFiles(tx) && ok;
    grid_page.slot_undo = 0;
    grid_page.load_slot_models();
    return ok;
}

uint16_t spsGridJournalLocalTrackMask(uint8_t tx) {
    if (tx >= kSpsGridJournalDepth)
        return 0;
    uint16_t trackMask = 0;
    for (uint8_t grid = 0; grid < NUM_GRIDS; grid++) {
        for (GridRow row = 0; row < GRID_LENGTH; row++)
            trackMask |= sps_grid_journal_masks[tx][grid][row];
    }
    return trackMask;
}

bool restoreLatestSpsGridJournalTransaction(uint16_t& trackMask) {
    trackMask = 0;
    if (sps_grid_journal_count == 0)
        return false;

    uint8_t tx = sps_grid_journal_head;
    for (uint8_t scanned = 0;
         scanned < kSpsGridJournalDepth && sps_grid_journal_count > 0;
         scanned++) {
        tx = spsGridJournalPrev(tx);
        if (!sps_grid_journal_valid[tx])
            continue;
        uint16_t restoredTrackMask = spsGridJournalLocalTrackMask(tx);
        if (!restoreSpsGridJournalTransaction(tx))
            return false;
        resetSpsGridJournalTransaction(tx);
        sps_grid_journal_head = tx;
        sps_grid_journal_count--;
        trackMask = restoredTrackMask;
        return true;
    }
    sps_grid_journal_count = 0;
    return false;
}

bool cleanupGridRowIfEmpty(GridIndex grid, GridRow row) {
    GridRowHeader header;
    if (!proj.read_grid_row_header(&header, row, grid))
        return false;
    if (!header.is_empty())
        return true;
    header.active = false;
    header.name[0] = '\0';
    return proj.write_grid_row_header(&header, row, grid);
}

bool copyGridMoveCell(GridSlot sourceSlot, GridRow sourceRow,
                             GridSlot targetSlot, GridRow targetRow,
                             bool destinationSame) {
    EmptyTrack scratch;
    DeviceTrack* source = scratch.load_from_grid_512(sourceSlot, sourceRow);
    if (!source)
        return false;

    GridDeviceTrack* gdt = mcl_actions.get_grid_dev_track(targetSlot);
    if (!gdt)
        return false;

    if (source->active != EMPTY_TRACK_TYPE) {
        source = source->materialize_as(gdt->track_type,
                                        (uint8_t)(targetSlot & 0x0F),
                                        gdt->seq_track);
        if (!source)
            return false;
    }

    int16_t linkRow = (int16_t)targetRow +
                      ((int16_t)source->link.row - (int16_t)sourceRow);
    if (linkRow < 0 || linkRow >= (int16_t)GRID_LENGTH)
        linkRow = targetRow;
    source->link.row = (uint8_t)linkRow;
    source->on_copy((GridColumn)(sourceSlot & 0x0F),
                    (GridColumn)(targetSlot & 0x0F), destinationSame);
    if (!source->store_in_grid(targetSlot, targetRow, nullptr, 0, false))
        return false;

    GridIndex targetGrid = (GridIndex)(targetSlot / GRID_WIDTH);
    GridColumn targetCol = (GridColumn)(targetSlot & 0x0F);
    GridRowHeader header;
    if (proj.read_grid_row_header(&header, targetRow, targetGrid)) {
        header.active = true;
        header.name[0] = '\0';
        header.update_model(targetCol, source->get_model(), source->active);
        return proj.write_grid_row_header(&header, targetRow, targetGrid);
    }
    return true;
}

bool moveSourceCellIsDestination(const GridMoveRequest& req,
                                        GridSlot sourceSlot,
                                        GridRow sourceRow) {
    int x = (int)sourceSlot - (int)req.targetCol;
    int y = (int)sourceRow - (int)req.targetRow;
    if (x < 0 || y < 0 || x >= req.width || y >= req.height)
        return false;
    return (req.rowMasks[y] & (uint16_t)(1u << x)) != 0;
}

bool clearGridMoveSources(const GridMoveRequest& req) {
    bool ok = true;
    for (GridSpan y = 0; y < req.height; y++) {
        GridRow sourceRow = (GridRow)(req.sourceRow + y);
        uint16_t mask = req.rowMasks[y];
        for (GridSpan x = 0; x < req.width; x++) {
            if ((mask & (uint16_t)(1u << x)) == 0)
                continue;
            GridSlot sourceSlot = (GridSlot)(req.sourceCol + x);
            if (moveSourceCellIsDestination(req, sourceSlot, sourceRow))
                continue;
            GridIndex grid = (GridIndex)(sourceSlot / GRID_WIDTH);
            ok = proj.clear_slot_grid(sourceSlot, sourceRow) && ok;
            ok = cleanupGridRowIfEmpty(grid, sourceRow) && ok;
        }
    }
    return ok;
}

bool applySparseGridMove(const GridMoveRequest& req) {
    int rowStart = 0;
    int rowEnd = req.height;
    int rowStep = 1;
    int colStart = 0;
    int colEnd = req.width;
    int colStep = 1;

    if (req.sourceCol / GRID_WIDTH == req.targetCol / GRID_WIDTH) {
        if (req.targetRow > req.sourceRow) {
            rowStart = req.height - 1;
            rowEnd = -1;
            rowStep = -1;
        }
        if (req.targetCol > req.sourceCol) {
            colStart = req.width - 1;
            colEnd = -1;
            colStep = -1;
        }
    }

    bool destinationSame = req.sourceCol == req.targetCol || req.width == 1;
    bool ok = true;
    for (int y = rowStart; y != rowEnd; y += rowStep) {
        uint16_t mask = req.rowMasks[y];
        for (int x = colStart; x != colEnd; x += colStep) {
            if ((mask & (uint16_t)(1u << x)) == 0)
                continue;
            GridSlot sourceSlot = (GridSlot)(req.sourceCol + x);
            GridSlot targetSlot = (GridSlot)(req.targetCol + x);
            GridRow sourceRow = (GridRow)(req.sourceRow + y);
            GridRow targetRow = (GridRow)(req.targetRow + y);
            if (sourceSlot == targetSlot && sourceRow == targetRow)
                continue;
            ok = copyGridMoveCell(sourceSlot, sourceRow, targetSlot,
                                  targetRow, destinationSame) &&
                 ok;
        }
    }
    return ok;
}

bool applyGridMove(const GridMoveRequest& req) {
    uint8_t journalTx = 0;
    if (!prepareSpsGridMoveJournal(req, journalTx))
        return false;

    bool ok = true;
    if (!req.sparse) {
        ok = mcl_clipboard.copy(req.sourceCol, req.sourceRow, req.width,
                                req.height) &&
             mcl_clipboard.paste(req.targetCol, req.targetRow);
    } else {
        ok = applySparseGridMove(req);
    }

    if (ok)
        ok = clearGridMoveSources(req);

    for (uint8_t grid = 0; grid < NUM_GRIDS; grid++)
        ok = proj.sync_grid(grid) && ok;
    grid_page.slot_undo = 0;
    grid_page.load_slot_models();
    if (ok) {
        ok = commitSpsGridJournalTransaction(journalTx);
    } else {
        restoreSpsGridJournalTransaction(journalTx);
        discardSpsGridJournalTransaction(journalTx);
    }
    return ok;
}
#endif

bool saveNeedsMdCurrentPattern() {
#ifdef PLATFORM_TBD
    return MD.connected &&
           (device_manager.primary_device() == &MD ||
            device_manager.secondary_device() == &MD);
#else
    return true;
#endif
}

void copyBounded(char* dst, size_t dstLen, const char* src) {
    if (!dst || dstLen == 0)
        return;
    if (!src)
        src = "";
    strncpy(dst, src, dstLen - 1);
    dst[dstLen - 1] = '\0';
}

bool validProjectRelPath(const char* path, bool allowEmpty) {
    if (!path)
        return false;
    if (path[0] == '\0')
        return allowEmpty;
    if (path[0] == '/' || strlen(path) >= PRJ_PATH_LEN)
        return false;

    uint8_t componentLen = 0;
    const char* componentStart = path;
    for (const char* p = path; ; ++p) {
        unsigned char c = (unsigned char)*p;
        bool end = c == '\0';
        bool slash = c == '/';
        if (end || slash) {
            if (componentLen == 0 || componentLen > PRJ_NAME_LEN)
                return false;
            if (componentLen == 1 && componentStart[0] == '.')
                return false;
            if (componentLen == 2 && componentStart[0] == '.' &&
                componentStart[1] == '.')
                return false;
            if (end)
                return true;
            componentLen = 0;
            componentStart = p + 1;
            continue;
        }
        if (c < 32 || c > 126 || c == ':' || c == '\\')
            return false;
        componentLen++;
    }
}

bool readProjectBodyString(const uint8_t* b, uint16_t n,
                                  uint16_t& off, char* out, size_t outLen,
                                  bool allowEmpty) {
    if (!out || outLen == 0 || off >= n)
        return false;
    uint8_t len = b[off++];
    if (len >= outLen || len > spsarr::kProjectPathBytes || off + len > n)
        return false;
    for (uint8_t i = 0; i < len; ++i) {
        unsigned char c = b[off + i];
        if (c < 32 || c > 126)
            return false;
        out[i] = (char)c;
    }
    out[len] = '\0';
    off = (uint16_t)(off + len);
    return validProjectRelPath(out, allowEmpty);
}

bool simpleProjectName(const char* name) {
    return validProjectRelPath(name, false) && strchr(name, '/') == nullptr;
}

bool splitProjectRelPath(const char* path, char* parent,
                                size_t parentLen, char* base,
                                size_t baseLen) {
    if (!validProjectRelPath(path, false) || !parent || !base ||
        parentLen == 0 || baseLen == 0)
        return false;
    const char* slash = strrchr(path, '/');
    const char* name = slash ? slash + 1 : path;
    if (strlen(name) == 0 || strlen(name) >= baseLen)
        return false;
    copyBounded(base, baseLen, name);
    if (!slash) {
        parent[0] = '\0';
        return true;
    }
    size_t len = (size_t)(slash - path);
    if (len >= parentLen)
        return false;
    memcpy(parent, path, len);
    parent[len] = '\0';
    return true;
}

bool joinProjectRelPath(const char* parent, const char* entry,
                               char* out, size_t outLen) {
    if (!out || outLen == 0 || !simpleProjectName(entry))
        return false;
    if (!parent || parent[0] == '\0') {
        if (strlen(entry) >= outLen)
            return false;
        strcpy(out, entry);
        return true;
    }
    return validProjectRelPath(parent, true) &&
           MCLSd::join_path(out, (uint8_t)outLen, parent, entry) &&
           validProjectRelPath(out, false);
}

bool buildProjectRootedPath(const char* rel, char* out, size_t outLen) {
    if (!validProjectRelPath(rel, true) || !out || outLen == 0)
        return false;
    char root[64];
    const char* rootPath = mcl_sd.full_path(PRJ_DIR, root, sizeof(root));
    if (!rel || rel[0] == '\0') {
        copyBounded(out, outLen, rootPath);
        return true;
    }
    return MCLSd::join_path(out, (uint8_t)outLen, rootPath, rel);
}

bool chdirProjectParent(const char* rel, char* base, size_t baseLen) {
    char parent[PRJ_PATH_LEN];
    if (!splitProjectRelPath(rel, parent, sizeof(parent), base, baseLen))
        return false;
    char parentRoot[128];
    if (!buildProjectRootedPath(parent, parentRoot, sizeof(parentRoot)))
        return false;
    return SD.chdir(parentRoot);
}

bool pathStartsWithProjectDir(const char* path, const char* dir) {
    if (!path || !dir || dir[0] == '\0')
        return false;
    while (*dir != '\0' && *path == *dir) {
        path++;
        dir++;
    }
    return *dir == '\0' && (*path == '\0' || *path == '/');
}

bool currentProjectUnder(const char* path) {
    return proj.project_loaded && pathStartsWithProjectDir(mcl_cfg.project,
                                                           path);
}

bool isProjectDirNameAtCwd(const char* entry) {
    size_t len = strlen(entry);
    if (len == 0 || len > PRJ_NAME_LEN || strchr(entry, '/') != nullptr)
        return false;

    char projectFile[PRJ_NAME_LEN * 2 + 6];
    if (!MCLSd::join_path(projectFile, sizeof(projectFile), entry, entry))
        return false;
    strcat(projectFile, ".mcl");
    return SD.exists(projectFile);
}

bool projectRelPathIsProject(const char* rel) {
    char base[PRJ_NAME_LEN + 1];
    if (!chdirProjectParent(rel, base, sizeof(base)))
        return false;
    return isProjectDirNameAtCwd(base);
}

bool currentProjectChildForCwd(const char* cwd, char* out,
                                      size_t outLen) {
    if (!proj.project_loaded || !out || outLen == 0)
        return false;
    const char* focus = nullptr;
    if (!cwd || cwd[0] == '\0') {
        focus = mcl_cfg.project;
    } else {
        size_t cwdLen = strlen(cwd);
        if (strncmp(mcl_cfg.project, cwd, cwdLen) != 0 ||
            mcl_cfg.project[cwdLen] != '/')
            return false;
        focus = mcl_cfg.project + cwdLen + 1;
    }
    const char* slash = strchr(focus, '/');
    size_t len = slash ? (size_t)(slash - focus) : strlen(focus);
    if (len == 0 || len >= outLen)
        return false;
    memcpy(out, focus, len);
    out[len] = '\0';
    return true;
}

uint8_t projectEntryFlags(uint8_t type, const char* path) {
    uint8_t flags = 0;
    bool currentExact =
        proj.project_loaded && strcmp(mcl_cfg.project, path) == 0;
    bool currentUnderPath = currentProjectUnder(path);
    if (currentExact || (type == PROJECT_ENTRY_DIR && currentUnderPath))
        flags |= PROJECT_ENTRY_CURRENT;
    if (!currentUnderPath)
        flags |= PROJECT_ENTRY_CAN_DELETE | PROJECT_ENTRY_CAN_MOVE;
    if (!currentUnderPath ||
        (type == PROJECT_ENTRY_PROJECT && currentExact))
        flags |= PROJECT_ENTRY_CAN_RENAME;
    if (type == PROJECT_ENTRY_PROJECT || type == PROJECT_ENTRY_DIR)
        flags |= PROJECT_ENTRY_CAN_COPY;
#ifndef MCL_HAS_FILE_MOVE
    flags &= (uint8_t)~PROJECT_ENTRY_CAN_MOVE;
#endif
#ifdef MCL_HAS_PROJECT_BACKUP
    if (type == PROJECT_ENTRY_PROJECT)
        flags |= PROJECT_ENTRY_HAS_VERSIONS;
#endif
    return flags;
}

void writeProjectEntry(uint8_t* dst, uint8_t type, uint8_t flags,
                              const char* name) {
    dst[0] = type;
    dst[1] = flags;
    memset(dst + 2, 0, spsarr::kProjectEntryNameBytes);
    for (uint8_t i = 0; i < spsarr::kProjectEntryNameBytes &&
                        name && name[i] != '\0';
         ++i) {
        dst[2 + i] = (uint8_t)labelChar(name[i]);
    }
}

bool appendProjectEntry(uint8_t* body, uint16_t logicalIndex,
                               uint16_t offset, uint8_t maxEntries,
                               uint8_t& count, uint8_t type,
                               uint8_t flags, const char* name) {
    if (logicalIndex < offset || count >= maxEntries)
        return false;
    uint16_t recordOff =
        (uint16_t)(spsarr::kProjectListHeaderBytes +
                   count * spsarr::kProjectEntryRecordBytes);
    writeProjectEntry(body + recordOff, type, flags, name);
    count++;
    return true;
}

void fillProjectListHeader(uint8_t* body, uint8_t flags,
                                  uint16_t offset, uint8_t count,
                                  uint16_t total, uint16_t currentIndex,
                                  const char* cwd) {
    body[0] = flags;
    spsArrPutU16(body + 1, offset);
    body[3] = count;
    spsArrPutU16(body + 4, total);
    spsArrPutU16(body + 6, currentIndex);
    uint8_t cwdLen = (uint8_t)strlen(cwd ? cwd : "");
    uint8_t currentLen = (uint8_t)strlen(proj.project_loaded ? mcl_cfg.project
                                                             : "");
    body[8] = cwdLen;
    body[9] = currentLen;
    memset(body + 10, 0, spsarr::kProjectPathBytes * 2);
    memcpy(body + 10, cwd ? cwd : "", cwdLen);
    memcpy(body + 10 + spsarr::kProjectPathBytes,
           proj.project_loaded ? mcl_cfg.project : "", currentLen);
}

uint16_t fillProjectListError(uint8_t* body, const char* cwd) {
    fillProjectListHeader(body, 0, 0, 1, 1, 0xFFFF, cwd ? cwd : "");
    writeProjectEntry(body + spsarr::kProjectListHeaderBytes,
                      PROJECT_ENTRY_ERROR, 0, "ERROR");
    return (uint16_t)(spsarr::kProjectListHeaderBytes +
                      spsarr::kProjectEntryRecordBytes);
}

bool sameProjectParent(const char* a, const char* b) {
    char parentA[PRJ_PATH_LEN];
    char parentB[PRJ_PATH_LEN];
    char base[PRJ_NAME_LEN + 1];
    return splitProjectRelPath(a, parentA, sizeof(parentA), base,
                               sizeof(base)) &&
           splitProjectRelPath(b, parentB, sizeof(parentB), base,
                               sizeof(base)) &&
           strcmp(parentA, parentB) == 0;
}

}  // namespace sps_host_arr_internal

#endif  // MCL_FEATURE_HOST_ARRANGER
