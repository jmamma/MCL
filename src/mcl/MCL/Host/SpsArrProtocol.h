/**
 * SpsArrProtocol.h - SPS host <-> MCL arranger cell protocol.
 *
 * CANONICAL copy. A byte-identical mirror lives at
 *   MCL/src/mcl/MCL/SpsArrProtocol.h
 *
 * Frame:
 *   F0 7D 41 52 <cmd> <tag> <body7...> <cks_hi> <cks_lo> F7
 *
 * Bodies are logical 8-bit bytes packed with the same 8->7 scheme used by the
 * step-grid protocol. MCL returns grid-cell/link summaries; the host caches
 * them and derives arranger viewport blocks locally. Durations are Q12
 * 16th-note ticks: 12 units per 16th, matching MCL's GridLink scheduling.
 */
#ifndef SPS_ARR_PROTOCOL_H
#define SPS_ARR_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>

#include "Host/SpsWireProtocol.h"

namespace spsarr {

static const uint8_t kMfrId = 0x7D;
static const uint8_t kSubId0 = 0x41;  // 'A'
static const uint8_t kSubId1 = 0x52;  // 'R'
static const uint8_t kProtoVersion = 1;

static const uint8_t kSysexStart = spswire::kSysexStart;
static const uint8_t kSysexEnd = spswire::kSysexEnd;

static const int kNumTracks = 16;
static const int kNumGridBanks = 2;
static const int kNumGridSlots = kNumTracks * kNumGridBanks;
static const int kNumRows = 128;
static const int kRowsPerCellPage = 29;
static const int kRowsPerLabelCellPage = 22;
static const int kRowsPerNamedCellPage = 13;
static const int kMaxBodyRaw = 512;

enum Cmd {
    CMD_HELLO = 0x01,
    CMD_HELLO_ACK = 0x02,

    CMD_REQ_CELLS = 0x10,
    CMD_REQ_ACTIVE = 0x11,
    CMD_REQ_ARR_META = 0x12,
    CMD_REQ_ARR_CLIPS = 0x13,
    CMD_REQ_ARR_MARKERS = 0x14,
    CMD_REQ_ARR_TRACK_LABELS = 0x15,
    CMD_REQ_ARR_LOOP_REGIONS = 0x16,
    CMD_REQ_PROJECT_LIST = 0x17,
    CMD_REQ_PROJECT_VERSIONS = 0x18,
    CMD_REQ_GRID_CHAIN = 0x19,
    CMD_REQ_ARR_AUTOMATION_LANES = 0x1A,
    CMD_REQ_ARR_AUTOMATION_POINTS = 0x1B,
    CMD_REQ_ARR_LOCAL_PREVIEW = 0x1C,

    CMD_CELLS = 0x30,
    CMD_ACTIVE = 0x31,
    CMD_ARR_META = 0x32,
    CMD_ARR_CLIPS = 0x33,
    CMD_ARR_MARKERS = 0x34,
    CMD_ARR_TRACK_LABELS = 0x35,
    CMD_ARR_LOOP_REGIONS = 0x36,
    CMD_PROJECT_LIST = 0x37,
    CMD_PROJECT_VERSIONS = 0x38,
    CMD_GRID_CHAIN = 0x39,
    CMD_ARR_AUTOMATION_LANES = 0x3A,
    CMD_ARR_AUTOMATION_POINTS = 0x3B,
    CMD_ARR_LOCAL_PREVIEW = 0x3C,

    CMD_SET_LINK = 0x50,
    CMD_SET_FADE = 0x51,
    CMD_LOAD_SLOTS = 0x52,
    CMD_ARR_CLEAR = 0x53,
    CMD_ARR_IMPORT_GRID = 0x54,
    CMD_ARR_SELECT = 0x55,
    CMD_ARR_NEW = 0x56,
    CMD_BATCH = 0x57,
    CMD_ARR_SAVE = 0x58,
    CMD_GRID_COPY = 0x59,
    CMD_GRID_CLEAR = 0x5A,
    CMD_GRID_PASTE = 0x5B,
    CMD_SET_ROW_NAME = 0x5C,
    CMD_SET_ARR_MARKER = 0x5D,
    CMD_SET_ARR_TRACK_LABEL = 0x5E,
    CMD_SAVE_SLOTS = 0x5F,
    CMD_GRID_APPLY_SLOT = 0x60,
    CMD_SET_ARR_CLIP_FADE = 0x61,
    CMD_ARR_SEEK_LOAD = 0x62,
    CMD_ARR_MAKE_LOCAL = 0x63,
    CMD_ARR_LOCAL_TO_GRID = 0x64,
    CMD_ARR_SET_LOOP = 0x65,
    CMD_SET_LOAD_SETTINGS = 0x66,
    CMD_SET_ARR_LOOP_REGION = 0x67,
    CMD_PROJECT_OP = 0x68,
    CMD_PROJECT_VERSION_OP = 0x69,
    CMD_GRID_MOVE = 0x6A,
    CMD_GRID_UNDO = 0x6B,
    CMD_SET_ARR_AUTOMATION_LANE = 0x6C,
    CMD_SET_ARR_AUTOMATION_LANE_CHUNK = 0x6D,

    CMD_NOTIFY_ACTIVE = 0x70,
    CMD_NOTIFY_DIRTY = 0x71,
    CMD_NOTIFY_ARR_POSITION = 0x72,
    CMD_ACK = 0x7E,
    CMD_ERR = 0x7F
};

enum Caps {
    CAP_AUTO = 1 << 0,
    CAP_FADE = 1 << 1,
    CAP_BATCH = 1 << 2,
    CAP_ARRANGER_LOAD = 1 << 3,
    CAP_ARRANGEMENT_STORE = 1 << 4,
    CAP_ARRANGER_CLEAR = 1 << 5,
    CAP_GRID_CLIPBOARD = 1 << 6,
    CAP_GRID_ROW_NAMES = 1 << 7,
    CAP_ARRANGEMENT_MARKERS = 1 << 8,
    CAP_ACTIVE_SLOTS = 1 << 9,
    CAP_ARRANGEMENT_TRACK_LABELS = 1 << 10,
    CAP_GRID_SAVE = 1 << 11,
    CAP_GRID_SLOT_EDIT = 1 << 12,
    CAP_ARRANGER_LOAD_SEEK = 1 << 13,
    CAP_ARRANGER_CLIP_FADES = 1 << 14
};

enum Caps2 {
    CAP2_GRID_BANKS = 1 << 0,
    CAP2_SLOT_OWNERSHIP = 1 << 1,
    CAP2_ARRANGEMENT_LOOP_REGIONS = 1 << 2,
    CAP2_PROJECT_BROWSER = 1 << 3,
    CAP2_PROJECT_BACKUP = 1 << 4,
    CAP2_PROJECT_MOVE = 1 << 5,
    CAP2_GRID_MOVE_UNDO = 1 << 6,
    CAP2_GRID_CHAIN = 1 << 7,
    CAP2_ARRANGEMENT_AUTOMATION = 1 << 8,
    CAP2_ARRANGER_PRIVATE_SOURCES = 1 << 9,
    CAP2_ARRANGER_LOCAL_PREVIEW = 1 << 10
};

enum ArrangementAutomationWriteOp {
    ARR_AUTOMATION_WRITE_BEGIN = 0,
    ARR_AUTOMATION_WRITE_POINTS = 1,
    ARR_AUTOMATION_WRITE_COMMIT = 2,
    ARR_AUTOMATION_WRITE_ABORT = 3
};

enum Mode {
    MODE_AUTO = 0,
    MODE_CHAIN = 1
};

enum PartFlags {
    PART_ACTIVE = 1 << 0,
    PART_HOLD = 1 << 1,
    PART_LOOP = 1 << 2,
    PART_EMPTY = 1 << 3,
    PART_FADE = 1 << 4,
    PART_LOAD_SOUND = 1 << 5
};

enum CellFlags {
    CELL_ACTIVE = 1 << 0,
    CELL_LOAD_SOUND = 1 << 1,
    CELL_FADE = 1 << 2
};

enum CellRequestFlags {
    REQ_CELL_LABELS = 1 << 0,
    REQ_ROW_NAMES = 1 << 1
};

enum CellFormatFlags {
    CELL_FORMAT_LABELS = 1 << 0,
    CELL_FORMAT_ROW_NAMES = 1 << 1,
    CELL_FORMAT_DEPENDENCIES = 1 << 2,
    CELL_FORMAT_GRID_BANK = 1 << 3,
    CELL_FORMAT_GROUP_INDEX = 1 << 4
};

static const int kCellRecordBaseBytes = 15;
static const int kCellRecordLabelBytes = 6;
static const int kCellRecordRowNameBytes = 16;
static const int kCellRecordDependencyBytes = 2;
static const int kCellRecordGroupIndexBytes = 1;
static const int kActiveSlotBytes = kNumTracks;
static const int kActiveExtraGridSlotBytes = kNumTracks;
static const int kActiveReleasedMaskBytes = 4;
static const int kActiveLoadSettingsBytes = 2;
static const int kActiveSlotOwnershipBytes = 4;
static const int kActiveSlotGroupIndexBytes = kNumGridSlots;
static const int kActivePendingTransitionBytes = 2;
static const int kActiveSlotSourceRowBytes = kNumGridSlots;
static const int kActiveLoadQueueLengthBytes = 1;
static const int kActivePrivateSourceMaskBytes = 4;
static const int kGridChainRowBytes = kNumGridSlots;
static const uint8_t kActiveSlotOffsetLoad = 253;
static const uint8_t kActiveSlotPending = 254;
static const uint8_t kActiveSlotDisabled = 255;
static const uint8_t kArrPrivateSourceRow = 253;
static const int kRowNameBytes = 16;
static const int kArrNameBytes = 16;
static const int kArrClipFadeBytes = 8;
static const int kArrClipRecordBaseBytes = 16;
static const int kArrClipRecordLegacyFadeBytes =
    kArrClipRecordBaseBytes + kArrClipFadeBytes;
static const int kArrClipRecordV4Bytes =
    kArrClipRecordBaseBytes + kArrClipFadeBytes * 2;
static const int kArrClipRecordSourceBytes = 8;
static const int kArrClipRecordBytes =
    kArrClipRecordV4Bytes + kArrClipRecordSourceBytes;
static const int kArrMarkerLabelBytes = 16;
static const int kArrMarkerRecordBytes = 24;
static const int kArrMarkerGlobalTrack = 255;
static const int kArrLoopRegionLabelBytes = 16;
static const int kArrLoopRegionRecordBytes = 32;
static const int kArrAutomationLaneRecordBytes = 16;
static const int kArrAutomationPointRecordBytes = 8;
static const int kArrLocalPreviewBytes = 16;
static const int kArrAutomationChunkBeginBytes =
    2 + kArrAutomationLaneRecordBytes;
static const int kArrAutomationChunkPointHeaderBytes = 6;
static const int kArrTrackLabelBytes = 16;
static const int kArrTrackLabelCount = 16;
static const int kProjectPathBytes = 64;
static const int kProjectEntryNameBytes = 32;
static const int kProjectEntryRecordBytes = 2 + kProjectEntryNameBytes;
static const int kProjectListHeaderBytes =
    10 + kProjectPathBytes + kProjectPathBytes;
static const int kProjectVersionHeaderBytes = 8 + kProjectPathBytes;
static const int kProjectVersionRecordBytes = 2;
static const int kMaxArrClipRecordsPerFrame =
    (kMaxBodyRaw - 16) / kArrClipRecordBytes;
static const int kMaxArrMarkerRecordsPerFrame =
    (kMaxBodyRaw - 16) / kArrMarkerRecordBytes;
static const int kMaxArrLoopRegionRecordsPerFrame =
    (kMaxBodyRaw - 16) / kArrLoopRegionRecordBytes;
static const int kMaxArrAutomationLaneRecordsPerFrame =
    (kMaxBodyRaw - 8) / kArrAutomationLaneRecordBytes;
static const int kMaxArrAutomationPointRecordsPerFrame =
    (kMaxBodyRaw - 16) / kArrAutomationPointRecordBytes;
static const int kMaxArrAutomationChunkPointRecordsPerFrame =
    (kMaxBodyRaw - kArrAutomationChunkPointHeaderBytes) /
    kArrAutomationPointRecordBytes;
static const int kMaxProjectEntriesPerFrame =
    (kMaxBodyRaw - kProjectListHeaderBytes) / kProjectEntryRecordBytes;
static const int kMaxProjectVersionRecordsPerFrame =
    (kMaxBodyRaw - kProjectVersionHeaderBytes) / kProjectVersionRecordBytes;
static const uint32_t kMinArrLoopQ12 = 2u * 12u;

enum DirtyRegion {
    DIRTY_CELLS = 1 << 0,
    DIRTY_ACTIVE = 1 << 1,
    DIRTY_ARRANGEMENT = 1 << 2,
    DIRTY_PROJECTS = 1 << 3
};

enum ProjectEntryType {
    PROJECT_ENTRY_NEW = 0,
    PROJECT_ENTRY_PARENT = 1,
    PROJECT_ENTRY_DIR = 2,
    PROJECT_ENTRY_PROJECT = 3,
    PROJECT_ENTRY_ERROR = 4
};

enum ProjectEntryFlags {
    PROJECT_ENTRY_CURRENT = 1 << 0,
    PROJECT_ENTRY_CAN_DELETE = 1 << 1,
    PROJECT_ENTRY_CAN_RENAME = 1 << 2,
    PROJECT_ENTRY_CAN_COPY = 1 << 3,
    PROJECT_ENTRY_CAN_MOVE = 1 << 4,
    PROJECT_ENTRY_HAS_VERSIONS = 1 << 5
};

enum ProjectListFlags {
    PROJECT_LIST_MORE = 1 << 0,
    PROJECT_LIST_BACKUP = 1 << 1,
    PROJECT_LIST_MOVE = 1 << 2,
    PROJECT_LIST_PROJECT_LOADED = 1 << 3
};

enum ProjectOp {
    PROJECT_OP_LOAD = 1,
    PROJECT_OP_NEW_PROJECT = 2,
    PROJECT_OP_NEW_FOLDER = 3,
    PROJECT_OP_DELETE = 4,
    PROJECT_OP_RENAME = 5,
    PROJECT_OP_COPY = 6,
    PROJECT_OP_MOVE = 7
};

enum ProjectVersionOp {
    PROJECT_VERSION_CREATE_BACKUP = 1,
    PROJECT_VERSION_LOAD = 2,
    PROJECT_VERSION_DELETE = 3
};

enum ProjectVersionFlags {
    PROJECT_VERSION_ACTIVE = 1 << 0,
    PROJECT_VERSION_CAN_DELETE = 1 << 1
};

enum PositionNotifyFlags {
    POSITION_NOTIFY_LOOP = 1 << 0
};

enum ArrangerLoadMode {
    ARR_LOAD_MANUAL = 1,
    ARR_LOAD_AUTO = 2,
    ARR_LOAD_QUEUE = 3,
    ARR_LOAD_ARRANG = 4
};

enum ArrangerLoadFlags {
    ARR_LOAD_START_TRANSPORT = 1 << 0,
    ARR_LOAD_CLEAR_EMPTY = 1 << 1,
    ARR_LOAD_GROUP_SELECT = 1 << 2,
    ARR_LOAD_SEEK_POSITION = 1 << 3,
    ARR_LOAD_IMMEDIATE = 1 << 4,
    ARR_LOAD_RUNTIME_FADES = 1 << 5,
    ARR_LOAD_GRID_BANK = 1 << 6,
    ARR_LOAD_PRIVATE_SOURCES = 1 << 7
};

enum GridSaveFlags {
    GRID_SAVE_GROUP_SELECT = 1 << 0
};

enum LoadSettingsFlags {
    LOAD_SETTINGS_MODE = 1 << 0,
    LOAD_SETTINGS_QUANT = 1 << 1,
    LOAD_SETTINGS_QUEUE_LENGTH = 1 << 2
};

enum GridSlotApplyFields {
    GRID_SLOT_APPLY_ROW = 1 << 0,
    GRID_SLOT_APPLY_LOOPS = 1 << 1,
    GRID_SLOT_APPLY_LENGTH = 1 << 2,
    GRID_SLOT_APPLY_LOAD_SOUND = 1 << 3
};

enum ErrCode {
    ERR_UNKNOWN_CMD = 0,
    ERR_BAD_TRACK = 1,
    ERR_RANGE = 2,
    ERR_BUSY = 3,
    ERR_UNSUPPORTED = 4,
    ERR_BAD_CKSUM = 5,
    ERR_BATCH_SUBOP = 6
};

static const uint16_t kFrameMinLen = spswire::kFrameMinLen;

using Parsed = spswire::Parsed;

inline uint16_t spsArrPack7(const uint8_t* in, uint16_t n, uint8_t* out) {
    return spswire::pack7(in, n, out);
}
inline uint16_t spsArrUnpack7(const uint8_t* in, uint16_t encLen,
                              uint8_t* out, uint16_t outCap) {
    return spswire::unpack7(in, encLen, out, outCap);
}
inline uint16_t spsArrPack7Size(uint16_t n) {
    return spswire::pack7Size(n);
}
inline uint16_t spsArrUnpack7Size(uint16_t encLen) {
    return spswire::unpack7Size(encLen);
}
inline uint16_t spsArrChecksum(const uint8_t* p, uint16_t n) {
    return spswire::checksum(p, n);
}
inline uint16_t spsArrBuildFrame(uint8_t cmd, uint8_t tag,
                                 const uint8_t* body8, uint16_t body8len,
                                 uint8_t* out, uint16_t outcap) {
    return spswire::buildFrame(kMfrId, kSubId0, kSubId1, cmd, tag, body8,
                               body8len, out, outcap);
}
inline bool spsArrParseFrame(const uint8_t* msg, uint16_t len, Parsed& out) {
    return spswire::parseFrame(kMfrId, kSubId0, kSubId1, msg, len, out);
}
inline uint16_t spsArrDecodeBody(const Parsed& p, uint8_t* body8,
                                 uint16_t cap) {
    return spswire::decodeBody(p, body8, cap);
}
inline void spsArrPutU16(uint8_t* p, uint16_t v) { spswire::putU16(p, v); }
inline uint16_t spsArrGetU16(const uint8_t* p) { return spswire::getU16(p); }
inline void spsArrPutU32(uint8_t* p, uint32_t v) { spswire::putU32(p, v); }
inline uint32_t spsArrGetU32(const uint8_t* p) { return spswire::getU32(p); }
inline void spsArrPutU64(uint8_t* p, uint64_t v) { spswire::putU64(p, v); }
inline uint64_t spsArrGetU64(const uint8_t* p) { return spswire::getU64(p); }

}  // namespace spsarr

#endif  // SPS_ARR_PROTOCOL_H
