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
    char label2[3] = {'-', '-', '\0'};
    char label4[5] = {'-', '-', ' ', ' ', '\0'};
};

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

static void copyRowName(GridRow row, uint8_t* dst) {
    if (!dst)
        return;
    for (uint8_t i = 0; i < spsarr::kRowNameBytes; i++)
        dst[i] = 0;

    GridRowHeader header;
    if (!proj.read_grid_row_header(&header, row, 0) || !header.active)
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
                               uint8_t track, GridRow row) {
    setLabel2(cell.label2, '-', '-');
    cell.label4[0] = '-';
    cell.label4[1] = '-';
    cell.label4[2] = ' ';
    cell.label4[3] = ' ';
    cell.label4[4] = '\0';

    if (!tr || !tr->is_active())
        return;

    GridSlotLabelContext ctx = {tr->get_model(), track};
#if defined(PLATFORM_TBD)
    ctx.slot = track;
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
            DeviceTrack* depTrack = scratch.load_from_grid_512(dep, row);
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

static ArrCell readCell(uint8_t track, GridRow row) {
    ArrCell cell;
    if (track >= spsarr::kNumTracks || row >= GRID_LENGTH)
        return cell;

    EmptyTrack scratch;
    DeviceTrack* tr = scratch.load_from_grid_512(track, row);
    if (!tr)
        return cell;

    cell.ok = true;
    cell.active = tr->is_active();
    cell.loadSound = tr->load_sound();
    cell.link = tr->link;
    cell.durationQ12 = linkDurationQ12(cell.link);
    populateCellLabels(cell, tr, track, row);
    cell.dependencyMask = cellDependencyMask(track, row, tr);
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

static bool parseGridRect(const uint8_t* b, uint16_t n, GridSlot& col,
                          GridRow& row, GridSpan& w, GridSpan& h) {
    if (n < 4)
        return false;
    col = b[0];
    row = b[1];
    w = b[2];
    h = b[3];
    if (col >= spsarr::kNumTracks || row >= GRID_LENGTH || w == 0 || h == 0)
        return false;
    if ((uint16_t)col + w > spsarr::kNumTracks)
        w = (GridSpan)(spsarr::kNumTracks - col);
    if ((uint16_t)row + h > GRID_LENGTH)
        h = (GridSpan)(GRID_LENGTH - row);
    return w > 0 && h > 0;
}

static bool clearGridRect(GridSlot col, GridRow row, GridSpan w, GridSpan h) {
    bool ok = true;
    for (GridSpan y = 0; y < h && (uint16_t)row + y < GRID_LENGTH; y++) {
        GridRow dstRow = (GridRow)(row + y);
        for (GridSpan x = 0; x < w && (uint16_t)col + x < spsarr::kNumTracks; x++) {
            if (!proj.clear_slot_grid((GridSlot)(col + x), dstRow))
                ok = false;
        }

        GridRowHeader header;
        if (proj.read_grid_row_header(&header, dstRow, 0) &&
            header.is_empty()) {
            header.active = false;
            header.name[0] = '\0';
            if (!proj.write_grid_row_header(&header, dstRow, 0))
                ok = false;
        }
    }

    proj.sync_grid(0);
    grid_page.slot_undo = 1;
    grid_page.slot_undo_x = (GridColumn)(col & 0x0F);
    grid_page.slot_undo_y = row;
    grid_page.load_slot_models();
    return ok;
}

static bool saveNeedsMdCurrentPattern() {
#ifdef PLATFORM_TBD
    return MD.connected &&
           (device_manager.primary_device() == &MD ||
            device_manager.secondary_device() == &MD);
#else
    return true;
#endif
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
        case CMD_REQ_CELLS: onReqCells(p.tag, b, n); break;
        case CMD_REQ_ARR_META: onReqArrMeta(p.tag); break;
        case CMD_REQ_ARR_CLIPS: onReqArrClips(p.tag, b, n); break;
        case CMD_REQ_ARR_MARKERS: onReqArrMarkers(p.tag, b, n); break;
        case CMD_REQ_ARR_TRACK_LABELS: onReqArrTrackLabels(p.tag); break;
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
        case CMD_GRID_APPLY_SLOT: onGridApplySlotEdit(p.tag, b, n); break;
        case CMD_SET_ROW_NAME: onSetRowName(p.tag, b, n); break;
        case CMD_SET_ARR_MARKER: onSetArrMarker(p.tag, b, n); break;
        case CMD_SET_ARR_TRACK_LABEL: onSetArrTrackLabel(p.tag, b, n); break;
        case CMD_SET_ARR_CLIP_FADE: onSetArrClipFade(p.tag, b, n); break;
        case CMD_ARR_SEEK_LOAD: onArrSeekLoad(p.tag, b, n); break;
        case CMD_ARR_MAKE_LOCAL: onArrMakeLocal(p.tag, b, n); break;
        case CMD_ARR_LOCAL_TO_GRID: onArrLocalToGrid(p.tag, b, n); break;
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
    uint8_t body[4];
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
    sendFrame(CMD_HELLO_ACK, tag, body, (uint16_t)sizeof body);
}

void SpsHostArrBridge::onReqActive(uint8_t tag) {
    uint8_t body[4 + spsarr::kActiveSlotBytes];
    body[0] = activeRowOrZero();
    body[1] = grid_task.next_active_row < GRID_LENGTH ? grid_task.next_active_row
                                                       : body[0];
    body[2] = grid_task.chain_behaviour ? 1 : 0;
    body[3] = MidiClock.isStarted() ? 1 : 0;
    for (uint8_t slot = 0; slot < spsarr::kActiveSlotBytes; slot++)
        body[4 + slot] = slot < NUM_SLOTS ? grid_page.active_slots[slot] : 255;
    sendFrame(CMD_ACTIVE, tag, body, (uint16_t)sizeof body);
}

void SpsHostArrBridge::onReqCells(uint8_t tag, const uint8_t* b, uint16_t n) {
    if (n < 5)
        return;
    uint8_t startRow = b[0];
    uint8_t rowCount = b[1];
    uint16_t trackMask = spsArrGetU16(b + 2);
    uint8_t flags = b[4];
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
                                CELL_FORMAT_DEPENDENCIES);

        uint16_t recordBytes = (uint16_t)(kCellRecordBaseBytes +
            kCellRecordDependencyBytes +
            (sendLabels ? kCellRecordLabelBytes : 0) +
            (sendRowNames ? kCellRecordRowNameBytes : 0));
        for (uint8_t i = 0; i < rowCount && off + recordBytes <= kMaxBodyRaw; i++) {
            ArrCell cell = readCell(track, (GridRow)(startRow + i));
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
            if (sendLabels) {
                body[off++] = (uint8_t)cell.label2[0];
                body[off++] = (uint8_t)cell.label2[1];
                body[off++] = (uint8_t)cell.label4[0];
                body[off++] = (uint8_t)cell.label4[1];
                body[off++] = (uint8_t)cell.label4[2];
                body[off++] = (uint8_t)cell.label4[3];
            }
            if (sendRowNames) {
                copyRowName((GridRow)(startRow + i), body + off);
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
    GridSlot loadOffset =
        (n >= 25 && b[24] < NUM_SLOTS) ? (GridSlot)b[24] : (GridSlot)255;
    if (mode < ARR_LOAD_MANUAL || mode > ARR_LOAD_QUEUE) {
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
        mcl_cfg.load_mode = mode;
        mcl_cfg.track_type_select = trackMask;
        grid_load_page.group_load(row, loadOffset);
        mcl_cfg.track_type_select = oldGroupMask;
        mcl_cfg.load_mode = oldMode;
        uint8_t ack[2] = {CMD_LOAD_SLOTS, 1};
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
        if (row < GRID_LENGTH) {
            rowSelect[slot] = row;
            any = true;
            continue;
        }
        if (allowClear && row == 255) {
            rowSelect[slot] = LOAD_QUEUE_CLEAR_ROW;
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
        mcl_arrangement.resetPlayback();
    }

    uint32_t positionQ12 = startStep > 0xFFFFFFFFu / 12u
                               ? 0xFFFFFFFFu
                               : startStep * 12u;
    bool armedRuntimeFade = mcl_arrangement.armRuntimeForHostLoad(
        positionQ12, rowSelect, trackMask, loadOffset);

    if ((flags & ARR_LOAD_RUNTIME_FADES) != 0) {
        uint16_t off = n >= 25 ? 25 : 24;
        if (off + 2 <= n) {
            uint16_t fadeMask = spsArrGetU16(b + off) & trackMask;
            off += 2;
            GridSlot firstSource = 255;
            for (uint8_t src = 0; src < NUM_SLOTS && src < spsarr::kNumTracks;
                 ++src) {
                if (((trackMask >> src) & 1u) != 0 &&
                    rowSelect[src] < GRID_LENGTH) {
                    firstSource = src;
                    break;
                }
            }
            for (uint8_t src = 0; src < NUM_SLOTS && src < spsarr::kNumTracks;
                 ++src) {
                if (((fadeMask >> src) & 1u) == 0)
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

                GridSlot dst = src;
                if (rowSelect[src] < GRID_LENGTH && loadOffset < NUM_SLOTS) {
                    if (firstSource == 255)
                        continue;
                    int mapped =
                        (int)src - (int)firstSource + (int)loadOffset;
                    if (mapped < 0 || mapped >= (int)NUM_SLOTS)
                        continue;
                    dst = (GridSlot)mapped;
                }
                if (dst >= 16)
                    continue;
                mcl_arrangement.armRuntimeFade(dst, fade);
                armedRuntimeFade = armedRuntimeFade || fade.enabled();
                ARR_FADE_TRACE("payload fade src=%u dst=%u flags=%u dur=%u amount=%u curve=%d enabled=%u",
                               src, dst, fade.flags, fade.duration_q12,
                               fade.amount, (int)fade.curve,
                               fade.enabled() ? 1 : 0);
            }
        }
    }

    uint8_t queueMode = mode;
    if ((flags & ARR_LOAD_IMMEDIATE) != 0) {
        queueMode |= LOAD_QUEUE_FLAG_IMMEDIATE;
    }
    if ((flags & ARR_LOAD_START_TRANSPORT) != 0) {
        queueMode |= LOAD_QUEUE_FLAG_PRESTART_FADE;
    }
    ARR_FADE_TRACE("load-slots mode=%u qmode=%u flags=%u step=%lu immediate=%u",
                   mode, queueMode, flags, (unsigned long)startStep,
                   (flags & ARR_LOAD_IMMEDIATE) ? 1 : 0);
    if (armedRuntimeFade) {
        ARR_FADE_TRACE("armed runtime fade step=%lu q12=%lu mask=%u",
                       (unsigned long)startStep, (unsigned long)positionQ12,
                       trackMask);
    }
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
            if ((mask & (uint16_t)(1u << slot)) != 0)
                trackSelect[slot] = 1;
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
    if (sourceCol >= spsarr::kNumTracks || sourceRow >= GRID_LENGTH ||
        targetCol >= spsarr::kNumTracks || targetRow >= GRID_LENGTH ||
        width == 0 || height == 0 || fields == 0) {
        sendErr(tag, ERR_RANGE, 1);
        return;
    }
    if ((uint16_t)targetCol + width > spsarr::kNumTracks)
        width = (GridSpan)(spsarr::kNumTracks - targetCol);
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
             x < width && (uint16_t)targetCol + x < spsarr::kNumTracks;
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
            if (proj.read_grid_row_header(&header, row, 0)) {
                header.active = true;
                header.name[0] = '\0';
                proj.write_grid_row_header(&header, row, 0);
            }
        }
    }

    if (!anyStored) {
        sendErr(tag, ERR_BUSY, 1);
        return;
    }

    proj.sync_grid(0);
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
        track >= spsarr::kNumTracks) {
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
    if (track >= spsarr::kNumTracks || row >= GRID_LENGTH ||
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
    if (track >= spsarr::kNumTracks || row >= GRID_LENGTH ||
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
        sourceRow >= GRID_LENGTH || targetSlot >= spsarr::kNumTracks ||
        targetRow >= GRID_LENGTH) {
        sendErr(tag, ERR_RANGE, targetSlot);
        return;
    }

    ArrCell targetCell = readCell(targetSlot, targetRow);
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

    bool queued = mcl_arrangement.seekLoad(
        positionQ12, (flags & ARR_LOAD_IMMEDIATE) != 0,
        (flags & ARR_LOAD_START_TRANSPORT) != 0);

    uint8_t ack[2] = {CMD_ARR_SEEK_LOAD, queued ? (uint8_t)1 : (uint8_t)0};
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

void SpsHostArrBridge::notifyDirty(int track, uint8_t regions) {
    if (!ready_)
        return;
    uint8_t b[2] = {(uint8_t)track, regions};
    sendFrame(CMD_NOTIFY_DIRTY, 0, b, 2);
}

#endif  // MCL_FEATURE_HOST_ARRANGER
