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

#include "SpsWireProtocol.h"

namespace spsarr {

static const uint8_t kMfrId = 0x7D;
static const uint8_t kSubId0 = 0x41;  // 'A'
static const uint8_t kSubId1 = 0x52;  // 'R'
static const uint8_t kProtoVersion = 1;

static const uint8_t kSysexStart = spswire::kSysexStart;
static const uint8_t kSysexEnd = spswire::kSysexEnd;

static const int kNumTracks = 16;
static const int kNumRows = 128;
static const int kRowsPerCellPage = 32;
static const int kRowsPerLabelCellPage = 24;
static const int kMaxBodyRaw = 512;

enum Cmd {
    CMD_HELLO = 0x01,
    CMD_HELLO_ACK = 0x02,

    CMD_REQ_CELLS = 0x10,
    CMD_REQ_ACTIVE = 0x11,
    CMD_REQ_ARR_META = 0x12,
    CMD_REQ_ARR_CLIPS = 0x13,

    CMD_CELLS = 0x30,
    CMD_ACTIVE = 0x31,
    CMD_ARR_META = 0x32,
    CMD_ARR_CLIPS = 0x33,

    CMD_SET_LINK = 0x50,
    CMD_SET_FADE = 0x51,
    CMD_LOAD_SLOTS = 0x52,
    CMD_ARR_CLEAR = 0x53,
    CMD_ARR_IMPORT_GRID = 0x54,
    CMD_ARR_SELECT = 0x55,
    CMD_ARR_NEW = 0x56,
    CMD_BATCH = 0x57,
    CMD_ARR_SAVE = 0x58,

    CMD_NOTIFY_ACTIVE = 0x70,
    CMD_NOTIFY_DIRTY = 0x71,
    CMD_ACK = 0x7E,
    CMD_ERR = 0x7F
};

enum Caps {
    CAP_AUTO = 1 << 0,
    CAP_FADE = 1 << 1,
    CAP_CHAIN = 1 << 2,
    CAP_BATCH = 1 << 3,
    CAP_ARRANGER_LOAD = 1 << 4,
    CAP_ARRANGEMENT_STORE = 1 << 5
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
    REQ_CELL_LABELS = 1 << 0
};

enum CellFormatFlags {
    CELL_FORMAT_LABELS = 1 << 0
};

static const int kCellRecordBaseBytes = 15;
static const int kCellRecordLabelBytes = 6;
static const int kArrNameBytes = 16;
static const int kArrClipRecordBytes = 16;
static const int kMaxArrClipRecordsPerFrame =
    (kMaxBodyRaw - 16) / kArrClipRecordBytes;

enum DirtyRegion {
    DIRTY_CELLS = 1 << 0,
    DIRTY_ACTIVE = 1 << 1,
    DIRTY_ARRANGEMENT = 1 << 2
};

enum ArrangerLoadMode {
    ARR_LOAD_MANUAL = 1
};

enum ArrangerLoadFlags {
    ARR_LOAD_START_TRANSPORT = 1 << 0
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

}  // namespace spsarr

#endif  // SPS_ARR_PROTOCOL_H
