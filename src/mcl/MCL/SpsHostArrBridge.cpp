/**
 * SpsHostArrBridge - implementation. See SpsHostArrBridge.h.
 */
#include "MCLPlatformFeatures.h"

#if MCL_FEATURE_HOST_ARRANGER

#include "SpsHostArrBridge.h"

#include "DeviceTrack.h"
#include "EmptyTrack.h"
#include "GridPages.h"
#include "GridTask.h"
#include "MCLActions.h"
#include "MCLClipBoard.h"
#include "MCLSeq.h"
#include "MCLSd.h"
#include "MCLSysConfig.h"
#include "MidiClock.h"
#include "MidiSysex.h"
#include "MidiUart.h"
#include "SeqTrack.h"
#include "MCLArrangement.h"
#include "Project.h"
#include "TrackLoadFade.h"
#include "DeviceManager.h"
#include "platform.h"
#include "../Drivers/MD/MD.h"
#include "MDTrack.h"
#include "SPSXTrack.h"
#include "../Drivers/MD/MDParams.h"
#include "../Drivers/MNM/MNMParams.h"

#include <string.h>

using namespace spsarr;

SpsHostArrBridge sps_host_arr_bridge;

namespace {

static constexpr uint16_t kCurrentPatternTimeoutMs = 500;
#if MCL_FEATURE_HOST_GRID_MOVE_UNDO
#define FILENAME_SPS_GRID_JOURNAL "sps_grid_journal_"
static constexpr uint8_t kSpsGridJournalDepth = 16;
#endif

#if defined(PLATFORM_WASM) && defined(DEBUGMODE)
#define ARR_FADE_TRACE(fmt, ...) DEBUG_PRINT_FN("[arr-fade] " fmt, ##__VA_ARGS__)
#else
#define ARR_FADE_TRACE(fmt, ...)
#endif

struct ArrCell {
    bool ok = false;
    bool active = false;
    bool loadSound = true;
    GridLink link;
    uint32_t durationQ12 = 0;
    TrackLoadFadeData fade;
    bool hasFade = false;
    uint16_t dependencyMask = 0;
    uint8_t groupIndex = 0xFF;
    char label2[3] = {'-', '-', '\0'};
    char label4[5] = {'-', '-', ' ', ' ', '\0'};
};

#if MCL_FEATURE_HOST_GRID_MOVE_UNDO
struct GridMoveRequest {
    GridSlot sourceCol = 0;
    GridRow sourceRow = 0;
    GridSpan width = 0;
    GridSpan height = 0;
    GridSlot targetCol = 0;
    GridRow targetRow = 0;
    bool sparse = false;
    uint16_t rowMasks[GRID_LENGTH] = {};
};

static Grid sps_grid_journal[kSpsGridJournalDepth][NUM_GRIDS];
static uint16_t
    sps_grid_journal_masks[kSpsGridJournalDepth][NUM_GRIDS][GRID_LENGTH];
static bool sps_grid_journal_valid[kSpsGridJournalDepth];
static uint8_t sps_grid_journal_head = 0;
static uint8_t sps_grid_journal_count = 0;
#endif

static uint32_t linkDurationQ12(const GridLink& link) {
    if (link.loops == 0)
        return 0;
    uint32_t q12 = (uint32_t)link.loops * link.length *
                   SeqTrack::get_speed_multiplier_int(link.speed_value());
    if (q12 < 12)
        q12 = 48;
    return q12;
}

static char labelChar(char c) {
    unsigned char u = (unsigned char)c;
    return u >= 32 && u <= 126 ? c : ' ';
}

static void setLabel2(char label[3], char a, char b) {
    label[0] = labelChar(a);
    label[1] = labelChar(b);
    label[2] = '\0';
}

static void copyLabelPair(const char* src, char* dst) {
    dst[0] = src && src[0] ? labelChar(src[0]) : ' ';
    dst[1] = src && src[1] ? labelChar(src[1]) : ' ';
}

static GridIndex sanitizeGridBank(uint8_t gridBank) {
    return gridBank < NUM_GRIDS ? (GridIndex)gridBank : (GridIndex)0;
}

static GridSlot visibleSlotToGridSlot(uint8_t visibleSlot, GridIndex gridBank) {
    return (GridSlot)(visibleSlot + gridBank * GRID_WIDTH);
}

static void copyRowName(GridRow row, GridIndex gridBank, uint8_t* dst) {
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

static const char* shortNamePart(uint8_t trackType, uint8_t model,
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

static void populateCellLabels(ArrCell& cell, DeviceTrack* tr,
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

static void addDependencyTrack(uint16_t& mask, uint8_t track) {
    if (track < spsarr::kNumTracks)
        mask |= (uint16_t)(1u << track);
}

static uint16_t directCellDependencyMask(DeviceTrack* tr) {
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

static uint16_t cellDependencyMask(uint8_t track, GridRow row,
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

static uint8_t groupSelectIndexForSlot(GridSlot slot) {
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

static uint8_t hostLoadQueueMode(uint8_t mode, uint8_t flags) {
    uint8_t queueMode = mode;
    if ((flags & ARR_LOAD_IMMEDIATE) != 0) {
        queueMode |= LOAD_QUEUE_FLAG_IMMEDIATE;
    }
    if ((flags & ARR_LOAD_START_TRANSPORT) != 0) {
        queueMode |= LOAD_QUEUE_FLAG_PRESTART_FADE;
    }
    return queueMode;
}

static uint32_t hostLoadDestinationMask(const GridRow rowSelect[NUM_SLOTS],
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

static void releaseHostLoadedArrangementTracks(
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

static ArrCell readCell(uint8_t track, GridRow row, GridIndex gridBank) {
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

static bool writeCellLink(uint8_t track, GridRow row, const GridLink& link,
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

static bool writeCellFade(uint8_t track, GridRow row,
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

static GridRow activeRowOrZero() {
    return grid_task.last_active_row < GRID_LENGTH ? grid_task.last_active_row : 0;
}

static void putArrClip(uint8_t* dst, const mclarrfile::Clip& clip) {
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

static void putArrMarker(uint8_t* dst, const mclarrfile::Marker& marker) {
    spsArrPutU32(dst + 0, marker.startQ12);
    dst[4] = marker.track;
    dst[5] = marker.flags;
    for (uint8_t i = 0; i < spsarr::kArrMarkerLabelBytes; ++i)
        dst[6 + i] = (uint8_t)marker.label[i];
    dst[22] = 0;
    dst[23] = 0;
}

static void putArrLoopRegion(uint8_t* dst,
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

static bool parseGridRect(const uint8_t* b, uint16_t n, GridSlot& col,
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
static uint16_t gridMoveWidthMask(GridSpan width) {
    if (width >= GRID_WIDTH)
        return 0xFFFFu;
    return (uint16_t)((1u << width) - 1u);
}

static bool parseGridMove(const uint8_t* b, uint16_t n,
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

static bool clearGridRect(GridSlot col, GridRow row, GridSpan w, GridSpan h) {
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
static uint8_t spsGridJournalPrev(uint8_t index) {
    return index == 0 ? (uint8_t)(kSpsGridJournalDepth - 1)
                      : (uint8_t)(index - 1);
}

static void resetSpsGridJournalTransaction(uint8_t tx) {
    if (tx >= kSpsGridJournalDepth)
        return;
    memset(sps_grid_journal_masks[tx], 0, sizeof(sps_grid_journal_masks[tx]));
    sps_grid_journal_valid[tx] = false;
}

static bool closeSpsGridJournalFiles(uint8_t tx);

static bool openSpsGridJournalFiles(uint8_t tx) {
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

static bool closeSpsGridJournalFiles(uint8_t tx) {
    if (tx >= kSpsGridJournalDepth)
        return false;
    bool ok = true;
    for (uint8_t i = 0; i < NUM_GRIDS; i++)
        ok = sps_grid_journal[tx][i].close_file() && ok;
    return ok;
}

static bool beginSpsGridJournalTransaction(uint8_t& tx) {
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

static bool commitSpsGridJournalTransaction(uint8_t tx) {
    if (tx >= kSpsGridJournalDepth || tx != sps_grid_journal_head)
        return false;
    sps_grid_journal_valid[tx] = true;
    sps_grid_journal_head = (uint8_t)((tx + 1) % kSpsGridJournalDepth);
    if (sps_grid_journal_count < kSpsGridJournalDepth)
        sps_grid_journal_count++;
    return true;
}

static void discardSpsGridJournalTransaction(uint8_t tx) {
    resetSpsGridJournalTransaction(tx);
}

static bool snapshotSpsGridJournalCell(uint8_t tx, GridSlot slot,
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

static bool prepareSpsGridMoveJournal(const GridMoveRequest& req,
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

static bool restoreSpsGridJournalTransaction(uint8_t tx) {
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

static uint16_t spsGridJournalLocalTrackMask(uint8_t tx) {
    if (tx >= kSpsGridJournalDepth)
        return 0;
    uint16_t trackMask = 0;
    for (uint8_t grid = 0; grid < NUM_GRIDS; grid++) {
        for (GridRow row = 0; row < GRID_LENGTH; row++)
            trackMask |= sps_grid_journal_masks[tx][grid][row];
    }
    return trackMask;
}

static bool restoreLatestSpsGridJournalTransaction(uint16_t& trackMask) {
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

static bool cleanupGridRowIfEmpty(GridIndex grid, GridRow row) {
    GridRowHeader header;
    if (!proj.read_grid_row_header(&header, row, grid))
        return false;
    if (!header.is_empty())
        return true;
    header.active = false;
    header.name[0] = '\0';
    return proj.write_grid_row_header(&header, row, grid);
}

static bool copyGridMoveCell(GridSlot sourceSlot, GridRow sourceRow,
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

static bool moveSourceCellIsDestination(const GridMoveRequest& req,
                                        GridSlot sourceSlot,
                                        GridRow sourceRow) {
    int x = (int)sourceSlot - (int)req.targetCol;
    int y = (int)sourceRow - (int)req.targetRow;
    if (x < 0 || y < 0 || x >= req.width || y >= req.height)
        return false;
    return (req.rowMasks[y] & (uint16_t)(1u << x)) != 0;
}

static bool clearGridMoveSources(const GridMoveRequest& req) {
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

static bool applySparseGridMove(const GridMoveRequest& req) {
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

static bool applyGridMove(const GridMoveRequest& req) {
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

static bool saveNeedsMdCurrentPattern() {
#ifdef PLATFORM_TBD
    return MD.connected &&
           (device_manager.primary_device() == &MD ||
            device_manager.secondary_device() == &MD);
#else
    return true;
#endif
}

static void copyBounded(char* dst, size_t dstLen, const char* src) {
    if (!dst || dstLen == 0)
        return;
    if (!src)
        src = "";
    strncpy(dst, src, dstLen - 1);
    dst[dstLen - 1] = '\0';
}

static bool validProjectRelPath(const char* path, bool allowEmpty) {
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

static bool readProjectBodyString(const uint8_t* b, uint16_t n,
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

static bool simpleProjectName(const char* name) {
    return validProjectRelPath(name, false) && strchr(name, '/') == nullptr;
}

static bool splitProjectRelPath(const char* path, char* parent,
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

static bool joinProjectRelPath(const char* parent, const char* entry,
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

static bool buildProjectRootedPath(const char* rel, char* out, size_t outLen) {
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

static bool chdirProjectParent(const char* rel, char* base, size_t baseLen) {
    char parent[PRJ_PATH_LEN];
    if (!splitProjectRelPath(rel, parent, sizeof(parent), base, baseLen))
        return false;
    char parentRoot[128];
    if (!buildProjectRootedPath(parent, parentRoot, sizeof(parentRoot)))
        return false;
    return SD.chdir(parentRoot);
}

static bool pathStartsWithProjectDir(const char* path, const char* dir) {
    if (!path || !dir || dir[0] == '\0')
        return false;
    while (*dir != '\0' && *path == *dir) {
        path++;
        dir++;
    }
    return *dir == '\0' && (*path == '\0' || *path == '/');
}

static bool currentProjectUnder(const char* path) {
    return proj.project_loaded && pathStartsWithProjectDir(mcl_cfg.project,
                                                           path);
}

static bool isProjectDirNameAtCwd(const char* entry) {
    size_t len = strlen(entry);
    if (len == 0 || len > PRJ_NAME_LEN || strchr(entry, '/') != nullptr)
        return false;

    char projectFile[PRJ_NAME_LEN * 2 + 6];
    if (!MCLSd::join_path(projectFile, sizeof(projectFile), entry, entry))
        return false;
    strcat(projectFile, ".mcl");
    return SD.exists(projectFile);
}

static bool projectRelPathIsProject(const char* rel) {
    char base[PRJ_NAME_LEN + 1];
    if (!chdirProjectParent(rel, base, sizeof(base)))
        return false;
    return isProjectDirNameAtCwd(base);
}

static bool currentProjectChildForCwd(const char* cwd, char* out,
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

static uint8_t projectEntryFlags(uint8_t type, const char* path) {
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

static void writeProjectEntry(uint8_t* dst, uint8_t type, uint8_t flags,
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

static bool appendProjectEntry(uint8_t* body, uint16_t logicalIndex,
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

static void fillProjectListHeader(uint8_t* body, uint8_t flags,
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

static uint16_t fillProjectListError(uint8_t* body, const char* cwd) {
    fillProjectListHeader(body, 0, 0, 1, 1, 0xFFFF, cwd ? cwd : "");
    writeProjectEntry(body + spsarr::kProjectListHeaderBytes,
                      PROJECT_ENTRY_ERROR, 0, "ERROR");
    return (uint16_t)(spsarr::kProjectListHeaderBytes +
                      spsarr::kProjectEntryRecordBytes);
}

static bool sameProjectParent(const char* a, const char* b) {
    char parentA[PRJ_PATH_LEN];
    char parentB[PRJ_PATH_LEN];
    char base[PRJ_NAME_LEN + 1];
    return splitProjectRelPath(a, parentA, sizeof(parentA), base,
                               sizeof(base)) &&
           splitProjectRelPath(b, parentB, sizeof(parentB), base,
                               sizeof(base)) &&
           strcmp(parentA, parentB) == 0;
}

}  // namespace

void SpsHostArrBridge::setup() {
    ready_ = true;
    MidiSysex.addSysexListener(this);
}

void SpsHostArrBridge::end() {
    SysexView view(sysex, msg_rd);
    uint16_t len = view.get_recordLen();
    if (len < kFrameMinLen || len > 2048)
        return;
    uint8_t buf[2048];
    for (uint16_t i = 0; i < len; i++)
        buf[i] = view.getByte(i);
    Parsed p;
    if (!spsArrParseFrame(buf, len, p))
        return;
    if (spsArrUnpack7Size(p.body7len) > kMaxBodyRaw)
        return;
    uint8_t body[kMaxBodyRaw + 64];
    uint16_t bl = spsArrDecodeBody(p, body, (uint16_t)sizeof body);
    handle(p, body, bl);
}

void SpsHostArrBridge::handle(const Parsed& p, const uint8_t* b, uint16_t n) {
    switch (p.cmd) {
        case CMD_HELLO: onHello(p.tag, b, n); break;
        case CMD_REQ_ACTIVE: onReqActive(p.tag); break;
        case CMD_REQ_GRID_CHAIN: onReqGridChain(p.tag); break;
        case CMD_REQ_CELLS: onReqCells(p.tag, b, n); break;
        case CMD_REQ_ARR_META: onReqArrMeta(p.tag); break;
        case CMD_REQ_ARR_CLIPS: onReqArrClips(p.tag, b, n); break;
        case CMD_REQ_ARR_MARKERS: onReqArrMarkers(p.tag, b, n); break;
        case CMD_REQ_ARR_LOOP_REGIONS: onReqArrLoopRegions(p.tag, b, n); break;
        case CMD_REQ_ARR_TRACK_LABELS: onReqArrTrackLabels(p.tag); break;
        case CMD_REQ_PROJECT_LIST: onReqProjectList(p.tag, b, n); break;
        case CMD_REQ_PROJECT_VERSIONS: onReqProjectVersions(p.tag, b, n); break;
        case CMD_SET_LINK:
            if (applySetLink(b, n) && n >= 2)
                notifyDirty(b[0], DIRTY_CELLS);
            break;
        case CMD_SET_FADE:
            if (applySetFade(b, n) && n >= 2)
                notifyDirty(b[0], DIRTY_CELLS);
            break;
        case CMD_LOAD_SLOTS: onLoadSlots(p.tag, b, n); break;
        case CMD_ARR_CLEAR: onArrClear(p.tag); break;
        case CMD_ARR_IMPORT_GRID: onArrImportGrid(p.tag, b, n); break;
        case CMD_ARR_SELECT: onArrSelect(p.tag, b, n); break;
        case CMD_ARR_NEW: onArrNew(p.tag); break;
        case CMD_ARR_SAVE: onArrSave(p.tag); break;
        case CMD_SAVE_SLOTS: onSaveSlots(p.tag, b, n); break;
        case CMD_GRID_COPY: onGridCopy(p.tag, b, n); break;
        case CMD_GRID_CLEAR: onGridClear(p.tag, b, n); break;
        case CMD_GRID_PASTE: onGridPaste(p.tag, b, n); break;
#if MCL_FEATURE_HOST_GRID_MOVE_UNDO
        case CMD_GRID_MOVE: onGridMove(p.tag, b, n); break;
        case CMD_GRID_UNDO: onGridUndo(p.tag); break;
#endif
        case CMD_GRID_APPLY_SLOT: onGridApplySlotEdit(p.tag, b, n); break;
        case CMD_SET_ROW_NAME: onSetRowName(p.tag, b, n); break;
        case CMD_SET_ARR_MARKER: onSetArrMarker(p.tag, b, n); break;
        case CMD_SET_ARR_LOOP_REGION: onSetArrLoopRegion(p.tag, b, n); break;
        case CMD_SET_ARR_TRACK_LABEL: onSetArrTrackLabel(p.tag, b, n); break;
        case CMD_SET_ARR_CLIP_FADE: onSetArrClipFade(p.tag, b, n); break;
        case CMD_ARR_SEEK_LOAD: onArrSeekLoad(p.tag, b, n); break;
        case CMD_ARR_MAKE_LOCAL: onArrMakeLocal(p.tag, b, n); break;
        case CMD_ARR_LOCAL_TO_GRID: onArrLocalToGrid(p.tag, b, n); break;
        case CMD_ARR_SET_LOOP: onArrSetLoop(p.tag, b, n); break;
        case CMD_SET_LOAD_SETTINGS: onSetLoadSettings(p.tag, b, n); break;
        case CMD_PROJECT_OP: onProjectOp(p.tag, b, n); break;
        case CMD_PROJECT_VERSION_OP: onProjectVersionOp(p.tag, b, n); break;
        default: sendErr(p.tag, ERR_UNKNOWN_CMD, 0); break;
    }
}

void SpsHostArrBridge::sendFrame(uint8_t cmd, uint8_t tag,
                                 const uint8_t* body, uint16_t bodyLen) {
    uint8_t frame[1 + 3 + 2 + spsarr::kMaxBodyRaw * 2 + 8];
    uint16_t n = spsArrBuildFrame(cmd, tag, body, bodyLen, frame,
                                  (uint16_t)sizeof frame);
    if (n)
        MidiUart.sendRaw(frame, n);
}

void SpsHostArrBridge::sendErr(uint8_t tag, uint8_t code, uint8_t detail) {
    uint8_t b[2] = {code, detail};
    sendFrame(CMD_ERR, tag, b, 2);
}

void SpsHostArrBridge::onHello(uint8_t tag, const uint8_t* b, uint16_t n) {
    if (n >= 1 && b[0] == 0)
        return;
    uint8_t body[6];
    body[0] = kProtoVersion;
    spsArrPutU16(body + 1, (uint16_t)(CAP_AUTO | CAP_FADE | CAP_BATCH |
                                      CAP_ARRANGER_LOAD |
                                      CAP_ARRANGEMENT_STORE |
                                      CAP_ARRANGER_CLEAR |
                                      CAP_GRID_CLIPBOARD |
                                      CAP_GRID_ROW_NAMES |
                                      CAP_ARRANGEMENT_MARKERS |
                                      CAP_ACTIVE_SLOTS |
                                      CAP_ARRANGEMENT_TRACK_LABELS |
                                      CAP_GRID_SAVE |
                                      CAP_GRID_SLOT_EDIT |
                                      CAP_ARRANGER_LOAD_SEEK |
                                      CAP_ARRANGER_CLIP_FADES));
    body[3] = (uint8_t)spsarr::kNumTracks;
    uint16_t caps2 = (uint16_t)(CAP2_GRID_BANKS | CAP2_SLOT_OWNERSHIP |
                                CAP2_ARRANGEMENT_LOOP_REGIONS |
                                CAP2_PROJECT_BROWSER | CAP2_GRID_CHAIN);
#if MCL_FEATURE_HOST_GRID_MOVE_UNDO
    caps2 |= CAP2_GRID_MOVE_UNDO;
#endif
#ifdef MCL_HAS_PROJECT_BACKUP
    caps2 |= CAP2_PROJECT_BACKUP;
#endif
#ifdef MCL_HAS_FILE_MOVE
    caps2 |= CAP2_PROJECT_MOVE;
#endif
    spsArrPutU16(body + 4, caps2);
    sendFrame(CMD_HELLO_ACK, tag, body, (uint16_t)sizeof body);
}

void SpsHostArrBridge::onReqActive(uint8_t tag) {
    uint8_t body[4 + spsarr::kActiveSlotBytes +
                 spsarr::kActiveReleasedMaskBytes +
                 spsarr::kActiveExtraGridSlotBytes +
                 spsarr::kActiveLoadSettingsBytes +
                 spsarr::kActiveSlotOwnershipBytes +
                 spsarr::kActiveSlotGroupIndexBytes +
                 spsarr::kActivePendingTransitionBytes +
                 spsarr::kActiveSlotSourceRowBytes +
                 spsarr::kActiveLoadQueueLengthBytes];
    body[0] = activeRowOrZero();
    body[1] = grid_task.next_active_row < GRID_LENGTH ? grid_task.next_active_row
                                                       : body[0];
    body[2] = grid_task.chain_behaviour ? 1 : 0;
    body[3] = MidiClock.isStarted() ? 1 : 0;
    for (uint8_t slot = 0; slot < spsarr::kActiveSlotBytes; slot++)
        body[4 + slot] = slot < NUM_SLOTS ? grid_page.active_slots[slot] : 255;
    spsArrPutU32(body + 4 + spsarr::kActiveSlotBytes,
                 mcl_arrangement.playbackReleasedMask());
    uint16_t extraOff =
        4 + spsarr::kActiveSlotBytes + spsarr::kActiveReleasedMaskBytes;
    for (uint8_t slot = 0; slot < spsarr::kActiveExtraGridSlotBytes; slot++) {
        uint8_t absoluteSlot = (uint8_t)(spsarr::kNumTracks + slot);
        body[extraOff + slot] =
            absoluteSlot < NUM_SLOTS ? grid_page.active_slots[absoluteSlot]
                                     : 255;
    }
    uint16_t settingsOff = extraOff + spsarr::kActiveExtraGridSlotBytes;
    body[settingsOff] = mcl_cfg.load_mode;
    body[settingsOff + 1] = mcl_cfg.chain_load_quant;
    uint32_t ownershipMask = 0;
    for (uint8_t slot = 0; slot < NUM_SLOTS && slot < 32; ++slot) {
        if (mcl_actions.get_grid_dev_track(slot) != nullptr)
            ownershipMask |= (uint32_t)(1ul << slot);
    }
    spsArrPutU32(body + settingsOff + spsarr::kActiveLoadSettingsBytes,
                 ownershipMask);
    uint16_t groupIndexOff = settingsOff + spsarr::kActiveLoadSettingsBytes +
                             spsarr::kActiveSlotOwnershipBytes;
    for (uint8_t slot = 0; slot < spsarr::kActiveSlotGroupIndexBytes; ++slot) {
        body[groupIndexOff + slot] =
            slot < NUM_SLOTS ? groupSelectIndexForSlot((GridSlot)slot) : 0xFF;
    }
    uint16_t pendingTransitionOff =
        groupIndexOff + spsarr::kActiveSlotGroupIndexBytes;
    spsArrPutU16(body + pendingTransitionOff, mcl_actions.next_transition);
    uint16_t sourceRowsOff =
        pendingTransitionOff + spsarr::kActivePendingTransitionBytes;
    for (uint8_t slot = 0; slot < spsarr::kActiveSlotSourceRowBytes; ++slot) {
        uint8_t row = 255;
        if (slot < NUM_SLOTS) {
            GridRow active = grid_page.active_slots[slot];
            if (active < GRID_LENGTH) {
                row = active;
            } else if ((active == SLOT_PENDING ||
                        active == SLOT_OFFSET_LOAD) &&
                       mcl_actions.links[slot].row < GRID_LENGTH) {
                row = mcl_actions.links[slot].row;
            }
        }
        body[sourceRowsOff + slot] = row;
    }
    uint16_t queueLengthOff =
        sourceRowsOff + spsarr::kActiveSlotSourceRowBytes;
    body[queueLengthOff] = mcl_cfg.chain_queue_length;
    sendFrame(CMD_ACTIVE, tag, body, (uint16_t)sizeof body);
}

void SpsHostArrBridge::onReqGridChain(uint8_t tag) {
    uint8_t body[spsarr::kGridChainRowBytes];
    for (uint8_t slot = 0; slot < spsarr::kGridChainRowBytes; ++slot) {
        uint8_t row = 255;
        if (slot < NUM_SLOTS && mcl_actions.chains[slot].is_mode_queue()) {
            GridRow activeRow = grid_page.active_slots[slot];
            GridChain& chain = mcl_actions.chains[slot];
            uint8_t count = chain.num_of_links;
            if (count > NUM_LINKS)
                count = NUM_LINKS;
            for (uint8_t i = 0; i < count; ++i) {
                GridRow chainRow = chain.rows[i];
                if (chainRow >= GRID_LENGTH)
                    continue;
                if (activeRow < GRID_LENGTH && activeRow == chainRow)
                    continue;
                row = chainRow;
                break;
            }
        }
        body[slot] = row;
    }
    sendFrame(CMD_GRID_CHAIN, tag, body, (uint16_t)sizeof body);
}

void SpsHostArrBridge::onReqCells(uint8_t tag, const uint8_t* b, uint16_t n) {
    if (n < 5)
        return;
    uint8_t startRow = b[0];
    uint8_t rowCount = b[1];
    uint16_t trackMask = spsArrGetU16(b + 2);
    uint8_t flags = b[4];
    GridIndex gridBank = n >= 6 ? sanitizeGridBank(b[5]) : (GridIndex)0;
    bool sendLabels = (flags & REQ_CELL_LABELS) != 0;
    bool sendRowNames = (flags & REQ_ROW_NAMES) != 0;

    if (startRow >= GRID_LENGTH || rowCount == 0)
        return;
    uint8_t maxRows = sendRowNames ? (uint8_t)spsarr::kRowsPerNamedCellPage
                                   : (sendLabels
                                          ? (uint8_t)spsarr::kRowsPerLabelCellPage
                                          : (uint8_t)spsarr::kRowsPerCellPage);
    if (rowCount > maxRows)
        rowCount = maxRows;
    if ((int)startRow + (int)rowCount > GRID_LENGTH)
        rowCount = (uint8_t)(GRID_LENGTH - startRow);

    for (uint8_t track = 0; track < spsarr::kNumTracks; track++) {
        if (((trackMask >> track) & 1u) == 0)
            continue;

        uint8_t body[kMaxBodyRaw];
        uint16_t off = 0;
        body[off++] = track;
        body[off++] = startRow;
        body[off++] = rowCount;
        body[off++] = (uint8_t)((sendLabels ? CELL_FORMAT_LABELS : 0) |
                                (sendRowNames ? CELL_FORMAT_ROW_NAMES : 0) |
                                CELL_FORMAT_DEPENDENCIES |
                                CELL_FORMAT_GROUP_INDEX |
                                (n >= 6 ? CELL_FORMAT_GRID_BANK : 0));
        if (n >= 6)
            body[off++] = gridBank;

        uint16_t recordBytes = (uint16_t)(kCellRecordBaseBytes +
            kCellRecordDependencyBytes +
            kCellRecordGroupIndexBytes +
            (sendLabels ? kCellRecordLabelBytes : 0) +
            (sendRowNames ? kCellRecordRowNameBytes : 0));
        for (uint8_t i = 0; i < rowCount && off + recordBytes <= kMaxBodyRaw; i++) {
            ArrCell cell = readCell(track, (GridRow)(startRow + i), gridBank);
            uint8_t cellFlags = 0;
            if (cell.ok && cell.active)
                cellFlags |= CELL_ACTIVE;
            if (cell.ok && cell.loadSound)
                cellFlags |= CELL_LOAD_SOUND;
            if (cell.hasFade)
                cellFlags |= CELL_FADE;

            body[off++] = cellFlags;
            body[off++] = cell.ok ? cell.link.row : 0;
            body[off++] = cell.ok ? cell.link.loops : 0;
            body[off++] = cell.ok ? cell.link.length : 16;
            body[off++] = cell.ok ? cell.link.speed_value() : 0;
            spsArrPutU32(body + off, cell.ok ? cell.durationQ12 : 0);
            off = (uint16_t)(off + 4);
            body[off++] = cell.hasFade ? cell.fade.flags : 0;
            body[off++] = cell.hasFade ? cell.fade.target
                                       : TRACK_LOAD_FADE_TARGET_DEFAULT;
            spsArrPutU16(body + off, cell.hasFade ? cell.fade.duration_q12 : 0);
            off = (uint16_t)(off + 2);
            body[off++] = cell.hasFade ? cell.fade.amount : 0;
            body[off++] = cell.hasFade ? (uint8_t)cell.fade.curve : 0;
            spsArrPutU16(body + off, cell.ok ? cell.dependencyMask : 0);
            off = (uint16_t)(off + kCellRecordDependencyBytes);
            body[off++] = cell.ok ? cell.groupIndex : 0xFF;
            if (sendLabels) {
                body[off++] = (uint8_t)cell.label2[0];
                body[off++] = (uint8_t)cell.label2[1];
                body[off++] = (uint8_t)cell.label4[0];
                body[off++] = (uint8_t)cell.label4[1];
                body[off++] = (uint8_t)cell.label4[2];
                body[off++] = (uint8_t)cell.label4[3];
            }
            if (sendRowNames) {
                copyRowName((GridRow)(startRow + i), gridBank, body + off);
                off = (uint16_t)(off + kCellRecordRowNameBytes);
            }
        }

        sendFrame(CMD_CELLS, tag, body, off);
    }
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

void SpsHostArrBridge::onReqProjectList(uint8_t tag, const uint8_t* b,
                                        uint16_t n) {
    if (n < 4) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }

    uint16_t offset = spsArrGetU16(b + 0);
    uint8_t maxEntries = b[2];
    if (maxEntries == 0 ||
        maxEntries > (uint8_t)spsarr::kMaxProjectEntriesPerFrame) {
        maxEntries = (uint8_t)spsarr::kMaxProjectEntriesPerFrame;
    }

    uint16_t off = 3;
    char cwd[PRJ_PATH_LEN];
    if (!readProjectBodyString(b, n, off, cwd, sizeof(cwd), true)) {
        sendErr(tag, ERR_RANGE, 1);
        return;
    }

    uint8_t body[spsarr::kProjectListHeaderBytes +
                 spsarr::kMaxProjectEntriesPerFrame *
                     spsarr::kProjectEntryRecordBytes] = {};
    char rooted[128];
    if (!buildProjectRootedPath(cwd, rooted, sizeof(rooted)) ||
        !SD.chdir(rooted)) {
        uint16_t len = fillProjectListError(body, cwd);
        sendFrame(CMD_PROJECT_LIST, tag, body, len);
        return;
    }

    File dir;
    if (!dir.open(rooted, O_READ) || !dir.isDirectory()) {
        dir.close();
        uint16_t len = fillProjectListError(body, cwd);
        sendFrame(CMD_PROJECT_LIST, tag, body, len);
        return;
    }
    dir.rewind();

    char currentChild[PRJ_NAME_LEN + 1];
    bool haveCurrentChild =
        currentProjectChildForCwd(cwd, currentChild, sizeof(currentChild));
    uint16_t currentIndex = 0xFFFF;
    uint16_t total = 0;
    uint8_t count = 0;

    appendProjectEntry(body, total, offset, maxEntries, count,
                       PROJECT_ENTRY_NEW, 0, "[ NEW PROJECT ]");
    total++;
    if (cwd[0] != '\0') {
        appendProjectEntry(body, total, offset, maxEntries, count,
                           PROJECT_ENTRY_PARENT, 0, "..");
        total++;
    }

    File entryFile;
    char entry[64];
    char entryPath[PRJ_PATH_LEN];
    while (entryFile.openNext(&dir, O_READ)) {
        entryFile.getName(entry, sizeof(entry));
        bool isDir = entryFile.isDirectory();
        entryFile.close();

        if (!isDir || entry[0] == '\0' || entry[0] == '.')
            continue;
        if (!simpleProjectName(entry))
            continue;
        uint8_t type = isProjectDirNameAtCwd(entry)
                           ? PROJECT_ENTRY_PROJECT
                           : PROJECT_ENTRY_DIR;
        if (!joinProjectRelPath(cwd, entry, entryPath, sizeof(entryPath)))
            continue;
        uint8_t flags = projectEntryFlags(type, entryPath);
        if (haveCurrentChild && strcmp(entry, currentChild) == 0) {
            currentIndex = total;
            flags |= PROJECT_ENTRY_CURRENT;
        }
        appendProjectEntry(body, total, offset, maxEntries, count, type,
                           flags, entry);
        total++;
    }
    entryFile.close();
    dir.close();

    uint8_t flags = 0;
    if ((uint16_t)(offset + count) < total)
        flags |= PROJECT_LIST_MORE;
#ifdef MCL_HAS_PROJECT_BACKUP
    flags |= PROJECT_LIST_BACKUP;
#endif
#ifdef MCL_HAS_FILE_MOVE
    flags |= PROJECT_LIST_MOVE;
#endif
    if (proj.project_loaded)
        flags |= PROJECT_LIST_PROJECT_LOADED;

    fillProjectListHeader(body, flags, offset, count, total, currentIndex,
                          cwd);
    uint16_t bodyLen =
        (uint16_t)(spsarr::kProjectListHeaderBytes +
                   count * spsarr::kProjectEntryRecordBytes);
    sendFrame(CMD_PROJECT_LIST, tag, body, bodyLen);
}

void SpsHostArrBridge::onReqProjectVersions(uint8_t tag, const uint8_t* b,
                                            uint16_t n) {
#ifndef MCL_HAS_PROJECT_BACKUP
    (void)b;
    (void)n;
    sendErr(tag, ERR_UNSUPPORTED, 0);
#else
    uint16_t off = 0;
    char project[PRJ_PATH_LEN];
    if (!readProjectBodyString(b, n, off, project, sizeof(project), false) ||
        !projectRelPathIsProject(project)) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }

    uint8_t activePair = 0;
    bool haveActive = proj.read_active_grid_pair(project, &activePair);
    char base[PRJ_NAME_LEN + 1];
    if (!chdirProjectParent(project, base, sizeof(base)) ||
        !SD.chdir(base)) {
        sendErr(tag, ERR_RANGE, 1);
        return;
    }

    uint8_t body[spsarr::kProjectVersionHeaderBytes +
                 128 * spsarr::kProjectVersionRecordBytes] = {};
    uint16_t total = 0;
    uint8_t count = 0;
    for (uint8_t pair = 0; pair < 128; ++pair) {
        if (!proj.project_pair_exists(pair, base))
            continue;
        uint16_t recordOff =
            (uint16_t)(spsarr::kProjectVersionHeaderBytes +
                       count * spsarr::kProjectVersionRecordBytes);
        body[recordOff] = pair;
        body[recordOff + 1] =
            (uint8_t)((haveActive && pair == activePair
                           ? PROJECT_VERSION_ACTIVE
                           : 0) |
                      (pair > 0 && (!haveActive || pair != activePair)
                           ? PROJECT_VERSION_CAN_DELETE
                           : 0));
        count++;
        total++;
    }

    body[0] = 0;
    body[1] = haveActive ? activePair : 0;
    spsArrPutU16(body + 2, total);
    body[4] = count;
    body[5] = (uint8_t)strlen(project);
    body[6] = 0;
    body[7] = 0;
    memset(body + 8, 0, spsarr::kProjectPathBytes);
    memcpy(body + 8, project, strlen(project));
    sendFrame(CMD_PROJECT_VERSIONS, tag, body,
              (uint16_t)(spsarr::kProjectVersionHeaderBytes +
                         count * spsarr::kProjectVersionRecordBytes));
#endif
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

void SpsHostArrBridge::onGridCopy(uint8_t tag, const uint8_t* b,
                                  uint16_t n) {
    GridSlot col = 0;
    GridRow row = 0;
    GridSpan w = 0;
    GridSpan h = 0;
    if (!parseGridRect(b, n, col, row, w, h)) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }
    if (!mcl_clipboard.copy(col, row, w, h)) {
        sendErr(tag, ERR_BUSY, 0);
        return;
    }
    grid_page.slot_undo = 0;
    uint8_t ack[2] = {CMD_GRID_COPY, 1};
    sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
}

void SpsHostArrBridge::onGridClear(uint8_t tag, const uint8_t* b,
                                   uint16_t n) {
    GridSlot col = 0;
    GridRow row = 0;
    GridSpan w = 0;
    GridSpan h = 0;
    if (!parseGridRect(b, n, col, row, w, h)) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }
    if (!mcl_clipboard.copy(col, row, w, h) || !clearGridRect(col, row, w, h)) {
        sendErr(tag, ERR_BUSY, 0);
        return;
    }
    uint8_t ack[2] = {CMD_GRID_CLEAR, 1};
    sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
    notifyDirty(0xFF, DIRTY_CELLS);
}

void SpsHostArrBridge::onGridPaste(uint8_t tag, const uint8_t* b,
                                   uint16_t n) {
    GridSlot col = 0;
    GridRow row = 0;
    GridSpan w = 0;
    GridSpan h = 0;
    if (!parseGridRect(b, n, col, row, w, h)) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }
    if (!mcl_clipboard.paste(col, row)) {
        sendErr(tag, ERR_BUSY, 0);
        return;
    }
    grid_page.slot_undo = 0;
    proj.sync_grid();
    grid_page.load_slot_models();
    uint8_t ack[2] = {CMD_GRID_PASTE, 1};
    sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
    notifyDirty(0xFF, DIRTY_CELLS);
}

#if MCL_FEATURE_HOST_GRID_MOVE_UNDO
void SpsHostArrBridge::onGridMove(uint8_t tag, const uint8_t* b,
                                  uint16_t n) {
    GridMoveRequest req;
    if (!parseGridMove(b, n, req)) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }
    if (!applyGridMove(req)) {
        sendErr(tag, ERR_BUSY, 0);
        return;
    }
    uint8_t ack[2] = {CMD_GRID_MOVE, 1};
    sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
}

void SpsHostArrBridge::onGridUndo(uint8_t tag) {
    uint16_t trackMask = 0;
    if (!restoreLatestSpsGridJournalTransaction(trackMask)) {
        sendErr(tag, ERR_BUSY, 0);
        return;
    }
    uint8_t ack[2] = {CMD_GRID_UNDO, 1};
    sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
    for (uint8_t track = 0; track < GRID_WIDTH; track++) {
        if ((trackMask & (uint16_t)(1u << track)) != 0)
            notifyDirty(track, DIRTY_CELLS);
    }
}
#endif

void SpsHostArrBridge::onGridApplySlotEdit(uint8_t tag, const uint8_t* b,
                                           uint16_t n) {
    if (n < 11) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }

    GridSlot sourceCol = b[0];
    GridRow sourceRow = b[1];
    GridSlot targetCol = b[2];
    GridRow targetRow = b[3];
    GridSpan width = b[4];
    GridSpan height = b[5];
    uint8_t fields = b[6] & (GRID_SLOT_APPLY_ROW |
                             GRID_SLOT_APPLY_LOOPS |
                             GRID_SLOT_APPLY_LENGTH |
                             GRID_SLOT_APPLY_LOAD_SOUND);
    GridIndex sourceGridBank =
        n >= 13 ? sanitizeGridBank(b[11]) : (GridIndex)0;
    GridIndex targetGridBank =
        n >= 13 ? sanitizeGridBank(b[12]) : (GridIndex)0;
    sourceCol = visibleSlotToGridSlot(sourceCol, sourceGridBank);
    targetCol = visibleSlotToGridSlot(targetCol, targetGridBank);
    if (sourceCol >= NUM_SLOTS || sourceRow >= GRID_LENGTH ||
        targetCol >= NUM_SLOTS || targetRow >= GRID_LENGTH ||
        width == 0 || height == 0 || fields == 0) {
        sendErr(tag, ERR_RANGE, 1);
        return;
    }
    GridSpan targetBankRemaining =
        (GridSpan)(GRID_WIDTH - (targetCol % GRID_WIDTH));
    if (width > targetBankRemaining)
        width = targetBankRemaining;
    if ((uint16_t)targetRow + height > GRID_LENGTH)
        height = (GridSpan)(GRID_LENGTH - targetRow);

    EmptyTrack sourceScratch;
    DeviceTrack* source = sourceScratch.load_from_grid_512(sourceCol,
                                                           sourceRow);
    if (!source || !source->is_active()) {
        sendErr(tag, ERR_BUSY, 0);
        return;
    }

    GridLink edited = source->link;
    bool editedLoadSound = source->load_sound();
    if ((fields & GRID_SLOT_APPLY_ROW) != 0)
        edited.row = b[7] < GRID_LENGTH ? b[7] : 0;
    if ((fields & GRID_SLOT_APPLY_LOOPS) != 0)
        edited.loops = b[8] & 0x7F;
    if ((fields & GRID_SLOT_APPLY_LENGTH) != 0) {
        edited.length = b[9] & 0x7F;
        if (edited.length == 0)
            edited.length = 1;
    }
    if ((fields & GRID_SLOT_APPLY_LOAD_SOUND) != 0)
        editedLoadSound = b[10] != 0;

    bool changedRow = (fields & GRID_SLOT_APPLY_ROW) != 0;
    bool changedLoops = (fields & GRID_SLOT_APPLY_LOOPS) != 0;
    bool changedLength = (fields & GRID_SLOT_APPLY_LENGTH) != 0;
    bool changedLoadSound = (fields & GRID_SLOT_APPLY_LOAD_SOUND) != 0;
    bool anyStored = false;

    for (GridSpan y = 0; y < height && (uint16_t)targetRow + y < GRID_LENGTH;
         y++) {
        GridRow row = (GridRow)(targetRow + y);
        bool rowStored = false;
        for (GridSpan x = 0;
             x < width && (uint16_t)targetCol + x < NUM_SLOTS;
             x++) {
            GridSlot col = (GridSlot)(targetCol + x);
            EmptyTrack scratch;
            DeviceTrack* track = scratch.load_from_grid_512(col, row);
            if (!track || !track->is_active())
                continue;

            bool storeSlot = false;
            if (col == sourceCol && row == sourceRow) {
                track->link = edited;
                track->set_load_sound(editedLoadSound);
                storeSlot = true;
            } else {
                GridLink link = track->link;
                if (changedLoadSound) {
                    track->set_load_sound(editedLoadSound);
                    storeSlot = true;
                }
                if (changedLoops && edited.loops == 0) {
                    link.loops = 0;
                    storeSlot = true;
                } else if (changedLoops || changedLength) {
                    if (changedLoops && changedLength) {
                        link.loops = edited.loops;
                        link.length = edited.length;
                        storeSlot = true;
                    } else if (changedLoops) {
                        uint16_t slotLength =
                            (uint16_t)link.length *
                            SeqTrack::get_speed_multiplier_int(
                                link.speed_value()) /
                            12;
                        if (slotLength) {
                            uint16_t targetLength =
                                (uint32_t)edited.length *
                                SeqTrack::get_speed_multiplier_int(
                                    edited.speed_value()) *
                                edited.loops /
                                12;
                            if (!(targetLength % slotLength) &&
                                slotLength <= targetLength) {
                                link.loops = targetLength / slotLength;
                            } else {
                                link.loops = edited.loops;
                            }
                            storeSlot = true;
                        }
                    } else if (changedLength &&
                               link.speed_value() == edited.speed_value()) {
                        link.length = edited.length;
                        storeSlot = true;
                    }
                }
                if (changedRow) {
                    link.row = edited.row;
                    storeSlot = true;
                }
                if (storeSlot)
                    track->link = link;
            }

            if (storeSlot &&
                track->write_grid(track->_this(), track->get_track_size(),
                                  col, row)) {
                rowStored = true;
                anyStored = true;
            }
        }

        if (rowStored) {
            GridRowHeader header;
            GridIndex grid = (GridIndex)(targetCol / GRID_WIDTH);
            if (proj.read_grid_row_header(&header, row, grid)) {
                header.active = true;
                header.name[0] = '\0';
                proj.write_grid_row_header(&header, row, grid);
            }
        }
    }

    if (!anyStored) {
        sendErr(tag, ERR_BUSY, 1);
        return;
    }

    proj.sync_grid((GridIndex)(targetCol / GRID_WIDTH));
    grid_page.load_slot_models();
    uint8_t ack[2] = {CMD_GRID_APPLY_SLOT, 1};
    sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
    notifyDirty(0xFF, DIRTY_CELLS);
}

void SpsHostArrBridge::onSetRowName(uint8_t tag, const uint8_t* b,
                                    uint16_t n) {
    if (n < 1 + spsarr::kRowNameBytes) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }

    GridRow row = b[0];
    if (row >= GRID_LENGTH) {
        sendErr(tag, ERR_RANGE, row);
        return;
    }

    char name[17] = {};
    uint8_t copyLen = (uint8_t)sizeof(name) - 1;
    if (copyLen > spsarr::kRowNameBytes)
        copyLen = spsarr::kRowNameBytes;
    for (uint8_t i = 0; i < copyLen; i++) {
        uint8_t c = b[1 + i];
        if (c == 0)
            break;
        name[i] = labelChar((char)c);
    }
    for (int i = copyLen - 1; i >= 0 && name[i] == ' '; i--)
        name[i] = '\0';

    GridRowHeader rowHeaders[NUM_GRIDS];
    for (uint8_t grid = 0; grid < NUM_GRIDS; grid++) {
        if (!proj.read_grid_row_header(&rowHeaders[grid], row, grid)) {
            sendErr(tag, ERR_RANGE, grid);
            return;
        }
    }
    if (!rowHeaders[0].active) {
        sendErr(tag, ERR_RANGE, row);
        return;
    }

    bool ok = true;
    for (uint8_t grid = 0; grid < NUM_GRIDS; grid++) {
        strncpy(rowHeaders[grid].name, name, sizeof(rowHeaders[grid].name) - 1);
        rowHeaders[grid].name[sizeof(rowHeaders[grid].name) - 1] = '\0';
        if (!proj.write_grid_row_header(&rowHeaders[grid], row, grid))
            ok = false;
        if (!proj.sync_grid(grid))
            ok = false;
    }
    if (!ok) {
        sendErr(tag, ERR_BUSY, row);
        return;
    }

    grid_page.load_slot_models();
    uint8_t ack[2] = {CMD_SET_ROW_NAME, 1};
    sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
    notifyDirty(0xFF, DIRTY_CELLS);
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

void SpsHostArrBridge::onProjectOp(uint8_t tag, const uint8_t* b,
                                   uint16_t n) {
    if (n < 3) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }

    uint8_t op = b[0];
    uint16_t off = 1;
    char path[PRJ_PATH_LEN];
    char arg[PRJ_PATH_LEN];
    if (!readProjectBodyString(b, n, off, path, sizeof(path), true) ||
        !readProjectBodyString(b, n, off, arg, sizeof(arg), true)) {
        sendErr(tag, ERR_RANGE, op);
        return;
    }

    bool ok = false;
    bool loadedProject = false;

    switch (op) {
        case PROJECT_OP_LOAD:
            ok = path[0] != '\0' && projectRelPathIsProject(path) &&
                 proj.load_project(path);
            if (ok) {
                grid_page.reload_slot_models = false;
                loadedProject = true;
            }
            break;

        case PROJECT_OP_NEW_PROJECT: {
            char newPath[PRJ_PATH_LEN];
            ok = simpleProjectName(arg) &&
                 joinProjectRelPath(path, arg, newPath, sizeof(newPath)) &&
                 proj.new_project(newPath) && proj.load_project(newPath);
            if (ok) {
                grid_page.reload_slot_models = false;
                loadedProject = true;
            }
            break;
        }

        case PROJECT_OP_NEW_FOLDER: {
            char newPath[PRJ_PATH_LEN];
            ok = simpleProjectName(arg) &&
                 joinProjectRelPath(path, arg, newPath, sizeof(newPath));
            if (ok) {
                proj.chdir_projects();
                ok = !SD.exists(newPath) && SD.mkdir(newPath, true);
            }
            break;
        }

        case PROJECT_OP_DELETE:
            ok = path[0] != '\0' && !currentProjectUnder(path);
            if (ok) {
                proj.chdir_projects();
                ok = mcl_sd.remove_dir(path);
            }
            break;

        case PROJECT_OP_RENAME: {
            ok = path[0] != '\0' && arg[0] != '\0' &&
                 sameProjectParent(path, arg);
            if (!ok)
                break;

            char baseFrom[PRJ_NAME_LEN + 1];
            char baseTo[PRJ_NAME_LEN + 1];
            char parent[PRJ_PATH_LEN];
            if (!splitProjectRelPath(path, parent, sizeof(parent), baseFrom,
                                     sizeof(baseFrom)) ||
                !splitProjectRelPath(arg, parent, sizeof(parent), baseTo,
                                     sizeof(baseTo)) ||
                !simpleProjectName(baseTo)) {
                ok = false;
                break;
            }

            char parentRoot[128];
            ok = buildProjectRootedPath(parent, parentRoot,
                                        sizeof(parentRoot)) &&
                 SD.chdir(parentRoot);
            if (!ok)
                break;

            bool isProject = isProjectDirNameAtCwd(baseFrom);
            bool reloadCurrent =
                isProject && proj.project_loaded &&
                strcmp(mcl_cfg.project, path) == 0;
            if (!isProject && currentProjectUnder(path)) {
                ok = false;
                break;
            }
            if (isProject) {
                ok = SD.chdir(baseFrom) &&
                     proj.rename_project_files(baseFrom, baseTo);
                SD.chdir(parentRoot);
                ok = ok && SD.rename(baseFrom, baseTo);
                if (ok && reloadCurrent) {
                    ok = proj.load_project(arg);
                    loadedProject = ok;
                }
            } else {
                ok = SD.rename(baseFrom, baseTo);
            }
            break;
        }

        case PROJECT_OP_COPY:
            ok = path[0] != '\0' && arg[0] != '\0';
            if (ok) {
                bool isProject = projectRelPathIsProject(path);
                if (isProject) {
                    ok = proj.copy_project(path, arg);
                } else {
                    proj.chdir_projects();
                    ok = mcl_sd.copy_dir(path, arg, 0, 64, 64);
                }
            }
            break;

        case PROJECT_OP_MOVE:
#ifndef MCL_HAS_FILE_MOVE
            sendErr(tag, ERR_UNSUPPORTED, op);
            return;
#else
            ok = path[0] != '\0' && arg[0] != '\0' &&
                 strcmp(path, arg) != 0 &&
                 !pathStartsWithProjectDir(arg, path) &&
                 !currentProjectUnder(path);
            if (ok) {
                bool isProject = projectRelPathIsProject(path);
                if (isProject) {
                    ok = proj.move_project(path, arg);
                } else {
                    proj.chdir_projects();
                    ok = SD.rename(path, arg);
                }
            }
            break;
#endif

        default:
            sendErr(tag, ERR_UNSUPPORTED, op);
            return;
    }

    if (!ok) {
        sendErr(tag, ERR_RANGE, op);
        return;
    }

    uint8_t ack[2] = {CMD_PROJECT_OP, op};
    sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
    uint8_t dirty = DIRTY_PROJECTS;
    if (loadedProject) {
        dirty |= (uint8_t)(DIRTY_CELLS | DIRTY_ACTIVE |
                           DIRTY_ARRANGEMENT);
    }
    notifyDirty(0xFF, dirty);
}

void SpsHostArrBridge::onProjectVersionOp(uint8_t tag, const uint8_t* b,
                                          uint16_t n) {
#ifndef MCL_HAS_PROJECT_BACKUP
    (void)b;
    (void)n;
    sendErr(tag, ERR_UNSUPPORTED, 0);
#else
    if (n < 3) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }

    uint8_t op = b[0];
    uint8_t pair = b[1];
    uint16_t off = 2;
    char project[PRJ_PATH_LEN];
    if (!readProjectBodyString(b, n, off, project, sizeof(project), false) ||
        !projectRelPathIsProject(project)) {
        sendErr(tag, ERR_RANGE, op);
        return;
    }

    bool ok = false;
    bool loadedProject = false;
    switch (op) {
        case PROJECT_VERSION_CREATE_BACKUP: {
            uint8_t createdPair = 0;
            ok = proj.create_backup(project, &createdPair) &&
                 proj.load_project_version(project, createdPair);
            loadedProject = ok;
            break;
        }
        case PROJECT_VERSION_LOAD:
            ok = pair < 128 && proj.load_project_version(project, pair);
            loadedProject = ok;
            break;
        case PROJECT_VERSION_DELETE:
            ok = pair > 0 && proj.delete_backup(project, pair);
            break;
        default:
            sendErr(tag, ERR_UNSUPPORTED, op);
            return;
    }

    if (!ok) {
        sendErr(tag, ERR_RANGE, op);
        return;
    }

    uint8_t ack[2] = {CMD_PROJECT_VERSION_OP, op};
    sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
    uint8_t dirty = DIRTY_PROJECTS;
    if (loadedProject) {
        dirty |= (uint8_t)(DIRTY_CELLS | DIRTY_ACTIVE |
                           DIRTY_ARRANGEMENT);
    }
    notifyDirty(0xFF, dirty);
#endif
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

void SpsHostArrBridge::notifyDirty(int track, uint8_t regions) {
    if (!ready_)
        return;
    uint8_t b[2] = {(uint8_t)track, regions};
    sendFrame(CMD_NOTIFY_DIRTY, 0, b, 2);
}

void SpsHostArrBridge::notifyArrangementPosition(uint32_t positionQ12,
                                                 uint8_t flags) {
    if (!ready_)
        return;
    uint8_t b[5] = {};
    spsArrPutU32(b + 0, positionQ12);
    b[4] = flags;
    sendFrame(CMD_NOTIFY_ARR_POSITION, 0, b, (uint16_t)sizeof b);
}

#endif  // MCL_FEATURE_HOST_ARRANGER
