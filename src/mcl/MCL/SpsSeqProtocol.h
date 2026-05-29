/**
 * SpsSeqProtocol.h — SPS host <-> MCL sequencer control protocol (wire format)
 *
 * CANONICAL copy. A byte-identical mirror lives at
 *   MCL/src/mcl/MCL/SpsSeqProtocol.h
 * Keep the two in sync — they define the on-wire contract for both sides.
 *
 * Spec: plans/PROTOCOL_SPS_MCL_SEQ_GRID.md
 *
 * Dependency-free (only <stdint.h>/<stddef.h>) so it compiles unchanged in the
 * SPS host build and the MCL/WASM build. No std::, no allocation, no I/O.
 *
 * Frame (one MIDI SysEx):
 *   F0 7D 53 51 <cmd> <tag> <body7...> <cks_hi> <cks_lo> F7
 * where body7 = 8->7 bit packing of the logical body (spsSeqPack7), and
 * cks = 14-bit sum over (cmd, tag, body7), transmitted as two 7-bit bytes.
 * SysEx (F0..F7) self-delimits; there is no length field.
 *
 * NOTE on listener matching: both host MidiSysex and MCL MidiSysexClass treat a
 * non-zero first id byte (0x7D) as a 1-byte manufacturer match (catch-all for
 * 0x7D). Handlers MUST verify byte[1],byte[2] == 0x53,0x51 (spsSeqParseFrame
 * does this).
 */
#ifndef SPS_SEQ_PROTOCOL_H
#define SPS_SEQ_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>

namespace spsseq {

// ---- header / identity ----
static const uint8_t  kMfrId   = 0x7D;   // MIDI non-commercial / private-use id
static const uint8_t  kSubId0  = 0x53;   // 'S'
static const uint8_t  kSubId1  = 0x51;   // 'Q'
static const uint8_t  kProtoVersion = 1;

static const uint8_t  kSysexStart = 0xF0;
static const uint8_t  kSysexEnd   = 0xF7;

// ---- dimensions ----
static const int kNumTracks     = 16;
static const int kNumSteps      = 64;
static const int kNumLockParams = 34;   // SPS-X superset
static const int kMaxBodyRaw    = 512;  // cap per frame (TRACK_LOCKS paginates)

// ---- command ids (byte[3]) ----
enum Cmd {
    CMD_HELLO            = 0x01,  // H->M  proto_ver, caps(2)
    CMD_HELLO_ACK        = 0x02,  // M->H  proto_ver, caps(2), num_tracks, max_steps, max_lock_params

    CMD_REQ_ACTIVE       = 0x10,  // H->M
    CMD_REQ_TRACK_SUMMARY= 0x11,  // H->M  track_mask(2)
    CMD_REQ_TRACK_DETAIL = 0x12,  // H->M  track
    CMD_REQ_TRACK_LOCKS  = 0x13,  // H->M  track
    CMD_REQ_PATTERN_META = 0x14,  // H->M

    CMD_TRACK_SUMMARY    = 0x30,  // M->H
    CMD_TRACK_DETAIL     = 0x31,  // M->H
    CMD_TRACK_LOCKS      = 0x32,  // M->H  (paginated)
    CMD_PATTERN_META     = 0x33,  // M->H

    CMD_SET_STEP         = 0x50,  // H->M  track, step, wmask, value
    CMD_SET_LOCK         = 0x51,  // H->M  track, step, param, value
    CMD_SET_MICROTIMING  = 0x52,  // H->M  track, step, int8
    CMD_SET_CONDITION    = 0x53,  // H->M  track, step, cond
    CMD_SET_TRACK_PROP   = 0x54,  // H->M  track, prop, value
    CMD_SET_PATTERN_PROP = 0x55,  // H->M  prop, value(2)
    CMD_CLR_LOCK         = 0x56,  // H->M  track, step, param
    CMD_BATCH            = 0x57,  // H->M  count, {cmd,len,bytes}*
    CMD_CLR_STEP_LOCKS   = 0x58,  // H->M  track, step — clear ALL locks on a step (authoritative;
                                  //       no cached lock detail needed on the host)

    CMD_NOTIFY_TRANSPORT = 0x70,  // M->H  running, master_step, sub_tick(2)
    CMD_NOTIFY_DIRTY     = 0x71,  // M->H  track, regions
    CMD_NOTIFY_ACTIVE    = 0x72,  // M->H  pattern_meta + active_track + transport
    CMD_ACK              = 0x7E,  // M->H  (echo tag) status
    CMD_ERR              = 0x7F   // M->H  (echo tag) err_code, detail
};

// ---- capability bits (HELLO/HELLO_ACK caps, 2x 7-bit -> 14 bits) ----
enum Caps {
    CAP_SPSX          = 1 << 0,  // required: MCL running spsx_tracks
    CAP_LOCKS         = 1 << 1,
    CAP_DETAIL        = 1 << 2,
    CAP_PER_TRACK_LEN = 1 << 3,
    CAP_BATCH         = 1 << 4,
    CAP_PER_TRACK_PH  = 1 << 5,
    CAP_AUTOMATION    = 1 << 6
};

// ---- canonical wire mask enum (translate to/from native on each side) ----
enum WMask {
    WMASK_TRIG          = 0,
    WMASK_MUTE          = 1,
    WMASK_SLIDE         = 2,
    WMASK_SWING         = 3,
    WMASK_LOCKS_ON_STEP = 4,  // read-only "step has >=1 lock"; not a SET_STEP target
    WMASK_COUNT         = 5
    // No accent (MCL has no accent) and NO lock-enable gate: host MASK_LOCK toggles
    // steps[].locks_enabled, but MCL has no such bit (descriptor reserved:1) and its
    // STEPSEQ_MASK_LOCK setter is a no-op. Lock VALUES are edited via SET_LOCK/CLR_LOCK.
};

// ---- NOTIFY_DIRTY region bits ----
enum DirtyRegion {
    DIRTY_SUMMARY = 1 << 0,
    DIRTY_DETAIL  = 1 << 1,
    DIRTY_LOCKS   = 1 << 2,
    DIRTY_META    = 1 << 3
};

// ---- ERR codes ----
enum ErrCode {
    ERR_UNKNOWN_CMD = 0,
    ERR_BAD_TRACK   = 1,
    ERR_RANGE       = 2,
    ERR_BUSY        = 3,
    ERR_UNSUPPORTED = 4,
    ERR_BAD_CKSUM   = 5,
    ERR_BATCH_SUBOP = 6
};

// ---- SET_TRACK_PROP / SET_PATTERN_PROP prop ids ----
enum TrackProp {
    TPROP_LENGTH = 0, TPROP_SPEED = 1, TPROP_SCALE = 2,
    TPROP_SWING_AMOUNT = 3, TPROP_MUTE = 4
};
enum PatternProp {
    PPROP_LENGTH = 0, PPROP_SCALE = 1, PPROP_TEMPO = 2,
    PPROP_KIT = 3, PPROP_SCALE_MODE = 4
};

// ============================================================================
// 8->7 bit packing (byte-identical to ElektronHelper::ElektronDataToSysex /
// ElektronSysexToData). Self-contained so the header has no link dependency.
// ============================================================================

// Pack `n` 8-bit bytes from `in` into 7-bit `out`. Returns encoded byte count.
inline uint16_t spsSeqPack7(const uint8_t* in, uint16_t n, uint8_t* out) {
    uint16_t retlen = 0;
    uint16_t cnt7 = 0;
    out[0] = 0;
    for (uint16_t cnt = 0; cnt < n; cnt++) {
        uint8_t c   = in[cnt] & 0x7F;
        uint8_t msb = (uint8_t)(in[cnt] >> 7);
        out[0] |= (uint8_t)(msb << (6 - cnt7));
        out[1 + cnt7] = c;
        if (cnt7++ == 6) {
            out += 8;
            retlen += 8;
            out[0] = 0;
            cnt7 = 0;
        }
    }
    return (uint16_t)(retlen + cnt7 + (cnt7 != 0 ? 1 : 0));
}

// Unpack `encLen` 7-bit bytes from `in` into 8-bit `out` (cap `outCap` bytes —
// never writes past it). Returns bytes actually written (<= outCap).
inline uint16_t spsSeqUnpack7(const uint8_t* in, uint16_t encLen, uint8_t* out, uint16_t outCap) {
    uint16_t cnt2 = 0;
    uint16_t bits = 0;
    for (uint16_t cnt = 0; cnt < encLen; cnt++) {
        if ((cnt % 8) == 0) {
            bits = in[cnt];
        } else {
            bits <<= 1;
            if (cnt2 < outCap) out[cnt2] = (uint8_t)(in[cnt] | (bits & 0x80));
            cnt2++;
        }
    }
    return cnt2 < outCap ? cnt2 : outCap;
}

// Worst-case encoded size for `n` raw bytes (ceil(n/7)*8).
inline uint16_t spsSeqPack7Size(uint16_t n) {
    return (uint16_t)(((n + 6) / 7) * 8);
}

// Exact decoded size for `encLen` 7-bit bytes (each group of 8 -> 7; a partial
// group of r bytes -> r-1). Lets callers reject oversized frames before decoding.
inline uint16_t spsSeqUnpack7Size(uint16_t encLen) {
    uint16_t full = (uint16_t)((encLen / 8) * 7);
    uint16_t rem  = (uint16_t)(encLen % 8);
    return (uint16_t)(full + (rem ? (rem - 1) : 0));
}

// ---- checksum: 14-bit sum, masked ----
inline uint16_t spsSeqChecksum(const uint8_t* p, uint16_t n) {
    uint16_t s = 0;
    for (uint16_t i = 0; i < n; i++) s = (uint16_t)(s + p[i]);
    return (uint16_t)(s & 0x3FFF);
}

// ============================================================================
// Frame build / parse
// ============================================================================

// Minimum recorded length (excl F0/F7): hdr(3)+cmd(1)+tag(1)+cks(2) = 7.
static const uint16_t kFrameMinLen = 7;

// Build a full SysEx frame (incl F0..F7) into `out` (cap `outcap`).
// `body8`/`body8len` is the logical body (may be null/0). Returns total bytes,
// or 0 on overflow.
inline uint16_t spsSeqBuildFrame(uint8_t cmd, uint8_t tag,
                                 const uint8_t* body8, uint16_t body8len,
                                 uint8_t* out, uint16_t outcap) {
    uint16_t need = (uint16_t)(1 + 3 + 2 + spsSeqPack7Size(body8len) + 2 + 1);
    if (need > outcap) return 0;
    uint16_t i = 0;
    out[i++] = kSysexStart;
    out[i++] = kMfrId;
    out[i++] = kSubId0;
    out[i++] = kSubId1;
    out[i++] = cmd;
    out[i++] = tag;
    uint16_t enc = (body8 && body8len) ? spsSeqPack7(body8, body8len, out + i) : 0;
    i = (uint16_t)(i + enc);
    // checksum over cmd,tag,body7 == out[4 .. 4+2+enc)
    uint16_t cks = spsSeqChecksum(out + 4, (uint16_t)(2 + enc));
    out[i++] = (uint8_t)((cks >> 7) & 0x7F);
    out[i++] = (uint8_t)(cks & 0x7F);
    out[i++] = kSysexEnd;
    return i;
}

struct Parsed {
    uint8_t        cmd;
    uint8_t        tag;
    const uint8_t* body7;     // points into msg
    uint16_t       body7len;
};

// Parse a recorded message that EXCLUDES F0 and F7 (host getByte / MCL view
// semantics). Verifies the 53 51 sub-id and the checksum. Returns false on any
// mismatch (including foreign 0x7D traffic). `msg[0]` must be 0x7D.
inline bool spsSeqParseFrame(const uint8_t* msg, uint16_t len, Parsed& out) {
    if (len < kFrameMinLen) return false;
    if (msg[0] != kMfrId || msg[1] != kSubId0 || msg[2] != kSubId1) return false;
    uint16_t cks = (uint16_t)(((uint16_t)msg[len - 2] << 7) | msg[len - 1]);
    // checksum covers cmd,tag,body7 == msg[3 .. len-2)
    if (spsSeqChecksum(msg + 3, (uint16_t)(len - 2 - 3)) != cks) return false;
    out.cmd      = msg[3];
    out.tag      = msg[4];
    out.body7    = msg + 5;
    out.body7len = (uint16_t)(len - 7);  // len - hdr(3) - cmd - tag - cks(2)
    return true;
}

// Decode body7 into an 8-bit buffer of `cap` bytes. Cap-safe (no overflow).
// Returns decoded byte count (<= cap). A frame whose body would exceed cap is
// malformed for this protocol (bodies are bounded by kMaxBodyRaw); callers
// further validate the decoded length per command.
inline uint16_t spsSeqDecodeBody(const Parsed& p, uint8_t* body8, uint16_t cap) {
    return spsSeqUnpack7(p.body7, p.body7len, body8, cap);
}

// ---- little-endian 64-bit mask helpers (step 0 = bit 0) ----
inline void spsSeqPutU64(uint8_t* p, uint64_t v) {
    for (int i = 0; i < 8; i++) p[i] = (uint8_t)((v >> (8 * i)) & 0xFF);
}
inline uint64_t spsSeqGetU64(const uint8_t* p) {
    uint64_t v = 0;
    for (int i = 0; i < 8; i++) v |= ((uint64_t)p[i]) << (8 * i);
    return v;
}

} // namespace spsseq

#endif // SPS_SEQ_PROTOCOL_H
