#include "MCLPlatformFeatures.h"

#if MCL_FEATURE_HOST_ARRANGER

#include "Host/SpsHostArrBridge.h"
#include "SpsHostArrBridge_Internal.h"

using namespace spsarr;
using namespace sps_host_arr_internal;

SpsHostArrBridge sps_host_arr_bridge;

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
