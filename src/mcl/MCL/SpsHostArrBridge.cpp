/**
 * SpsHostArrBridge - implementation. See SpsHostArrBridge.h.
 */
#if !defined(__AVR__)

#include "SpsHostArrBridge.h"

#include "DeviceTrack.h"
#include "EmptyTrack.h"
#include "GridTask.h"
#include "MCLSeq.h"
#include "MCLSysConfig.h"
#include "MidiClock.h"
#include "MidiSysex.h"
#include "MidiUart.h"
#include "SeqTrack.h"
#include "MCLArrangement.h"
#include "TrackLoadFade.h"
#include "../Drivers/MD/MDParams.h"
#include "../Drivers/MNM/MNMParams.h"

#include <string.h>

using namespace spsarr;

SpsHostArrBridge sps_host_arr_bridge;

namespace {

struct ArrCell {
    bool ok = false;
    bool active = false;
    bool loadSound = true;
    GridLink link;
    uint32_t durationQ12 = 0;
    TrackLoadFadeData fade;
    bool hasFade = false;
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
    if (track >= spsarr::kNumTracks || row >= GRID_LENGTH)
        return false;
    EmptyTrack scratch;
    DeviceTrack* tr = scratch.load_from_grid_512(track, row);
    if (!tr || !tr->is_active())
        return false;
    TrackLoadFadeData* dst = tr->load_fade_data();
    if (!dst)
        return false;
    *dst = fade;
    return tr->write_grid(tr->_this(), tr->get_track_size(), track, row);
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
                                      CAP_ARRANGEMENT_STORE));
    body[3] = (uint8_t)spsarr::kNumTracks;
    sendFrame(CMD_HELLO_ACK, tag, body, (uint16_t)sizeof body);
}

void SpsHostArrBridge::onReqActive(uint8_t tag) {
    uint8_t body[3];
    body[0] = activeRowOrZero();
    body[1] = grid_task.next_active_row < GRID_LENGTH ? grid_task.next_active_row
                                                       : body[0];
    body[2] = grid_task.chain_behaviour ? 1 : 0;
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

    if (startRow >= GRID_LENGTH || rowCount == 0)
        return;
    uint8_t maxRows = sendLabels ? (uint8_t)spsarr::kRowsPerLabelCellPage
                                 : (uint8_t)spsarr::kRowsPerCellPage;
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
        body[off++] = sendLabels ? CELL_FORMAT_LABELS : 0;

        uint16_t recordBytes = (uint16_t)(kCellRecordBaseBytes +
            (sendLabels ? kCellRecordLabelBytes : 0));
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
            if (sendLabels) {
                body[off++] = (uint8_t)cell.label2[0];
                body[off++] = (uint8_t)cell.label2[1];
                body[off++] = (uint8_t)cell.label4[0];
                body[off++] = (uint8_t)cell.label4[1];
                body[off++] = (uint8_t)cell.label4[2];
                body[off++] = (uint8_t)cell.label4[3];
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

void SpsHostArrBridge::onLoadSlots(uint8_t tag, const uint8_t* b, uint16_t n) {
    if (n < 24) {
        sendErr(tag, ERR_RANGE, 0);
        return;
    }

    uint8_t mode = b[0];
    uint8_t flags = b[1];
    uint32_t startStep = spsArrGetU32(b + 2);
    uint16_t trackMask = spsArrGetU16(b + 6);
    if (mode != ARR_LOAD_MANUAL) {
        sendErr(tag, ERR_UNSUPPORTED, mode);
        return;
    }

    GridRow rowSelect[NUM_SLOTS];
    memset(rowSelect, 255, sizeof(rowSelect));
    bool any = false;
    for (uint8_t slot = 0; slot < NUM_SLOTS && slot < spsarr::kNumTracks; slot++) {
        if (((trackMask >> slot) & 1u) == 0)
            continue;
        uint8_t row = b[8 + slot];
        if (row >= GRID_LENGTH)
            continue;
        rowSelect[slot] = row;
        any = true;
    }

    if (!any) {
        sendErr(tag, ERR_RANGE, 1);
        return;
    }

    if ((flags & ARR_LOAD_START_TRANSPORT) != 0) {
        static constexpr uint32_t kHostTicksPer16th = 6u * 16u;
        uint32_t tick96 = startStep > 0xFFFFFFFFu / kHostTicksPer16th
                              ? 0xFFFFFFFFu
                              : startStep * kHostTicksPer16th;
        MidiClock.set_transport_position(tick96);
        mcl_seq.set_transport_position(tick96);
        mcl_arrangement.resetPlayback();
    }

    grid_task.load_queue.put(LOAD_MANUAL, rowSelect);
    uint8_t ack[2] = {CMD_LOAD_SLOTS, 1};
    sendFrame(CMD_ACK, tag, ack, (uint16_t)sizeof ack);
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
    return writeCellFade(b[0], b[1], fade);
}

void SpsHostArrBridge::notifyDirty(int track, uint8_t regions) {
    if (!ready_)
        return;
    uint8_t b[2] = {(uint8_t)track, regions};
    sendFrame(CMD_NOTIFY_DIRTY, 0, b, 2);
}

#endif  // !defined(__AVR__)
