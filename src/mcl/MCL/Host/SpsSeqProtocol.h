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

#include "Host/SpsWireProtocol.h"

namespace spsseq {

// ---- header / identity ----
static const uint8_t  kMfrId   = 0x7D;   // MIDI non-commercial / private-use id
static const uint8_t  kSubId0  = 0x53;   // 'S'
static const uint8_t  kSubId1  = 0x51;   // 'Q'
static const uint8_t  kProtoVersion = 1;

static const uint8_t  kSysexStart = spswire::kSysexStart;
static const uint8_t  kSysexEnd   = spswire::kSysexEnd;

// ---- dimensions ----
static const int kNumTracks     = 16;
static const int kNumExtTracks  = 16;   // protocol maximum; HELLO_ACK reports actual
static const int kNumSteps      = 64;
static const int kNumLockParams = 34;   // SPS-X superset
static const int kMaxBodyRaw    = 512;  // cap per frame (TRACK_LOCKS paginates)
static const int kExtNoteWireBytes = 11; // start(4), width(4), note, velocity, condition

// ---- command ids (byte[3]) ----
enum Cmd {
    CMD_HELLO            = 0x01,  // H->M  proto_ver, caps(2)
    CMD_HELLO_ACK        = 0x02,  // M->H  proto_ver, caps(2), num_tracks, max_steps, max_lock_params

    CMD_REQ_ACTIVE       = 0x10,  // H->M
    CMD_REQ_TRACK_SUMMARY= 0x11,  // H->M  track_mask(2)
    CMD_REQ_TRACK_DETAIL = 0x12,  // H->M  track
    CMD_REQ_TRACK_LOCKS  = 0x13,  // H->M  track
    CMD_REQ_PATTERN_META = 0x14,  // H->M
    CMD_REQ_EXT_TRACK_META = 0x15, // H->M  device, track
    CMD_REQ_EXT_NOTES    = 0x16,  // H->M  device, track
    CMD_REQ_PERF_STATE   = 0x17,  // H->M  device, track

    CMD_TRACK_SUMMARY    = 0x30,  // M->H
    CMD_TRACK_DETAIL     = 0x31,  // M->H
    CMD_TRACK_LOCKS      = 0x32,  // M->H  (paginated)
    CMD_PATTERN_META     = 0x33,  // M->H
    CMD_EXT_TRACK_META   = 0x34,  // M->H  device, track, timing/meta
    CMD_EXT_NOTES        = 0x35,  // M->H  device, track, paginated note pairs
    CMD_PERF_STATE       = 0x36,  // M->H  device, track, PTC/ARP/voice state

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
    CMD_EXT_ADD_NOTE     = 0x59,  // H->M  device, track, start(4), width(4), note, velocity, condition
    CMD_EXT_DEL_NOTE     = 0x5A,  // H->M  device, track, start(4), width(4), note
    CMD_EXT_CLEAR_RANGE  = 0x5B,  // H->M  device, track, start(4), width(4), note_min, note_max
    CMD_EXT_SET_TRACK_PROP = 0x5C,// H->M  device, track, prop, value
    CMD_SET_PTC_PROP     = 0x5D,  // H->M  device, track, prop, value
    CMD_SET_ARP_PROP     = 0x5E,  // H->M  device, track, prop, value
    CMD_SET_PTC_GROUP    = 0x5F,  // H->M  track, group
    CMD_PTC_NOTE_EVENT   = 0x60,  // H->M  device, track, note, velocity, pressed

    CMD_NOTIFY_TRANSPORT = 0x70,  // M->H  running, master_step, sub_tick(2)
    CMD_NOTIFY_DIRTY     = 0x71,  // M->H  track, regions
    CMD_NOTIFY_ACTIVE    = 0x72,  // M->H  pattern_meta + active_track + transport
    CMD_NOTIFY_EXT_DIRTY = 0x73,  // M->H  device, track, regions
    CMD_NOTIFY_PERF_DIRTY= 0x74,  // M->H  device, track, regions
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
    CAP_AUTOMATION    = 1 << 6,
    CAP_EXT_NOTES     = 1 << 7,
    CAP_PTC_ARP       = 1 << 8
};

enum ExtDevice {
    EXT_DEVICE_GRID_Y = 1
};

enum ExtDirtyRegion {
    EXT_DIRTY_META  = 1 << 0,
    EXT_DIRTY_NOTES = 1 << 1
};

static const int kPtcGroupTracks = 16;
static const int kPtcNoteMaskBytes = 16; // 128-bit note masks, little endian
static const int kPerfStateWireBytes =
    2 + 4 + 4 + kPtcNoteMaskBytes + kPtcNoteMaskBytes + kPtcGroupTracks + 2;

enum PerfDirtyRegion {
    PERF_DIRTY_PTC    = 1 << 0,
    PERF_DIRTY_ARP    = 1 << 1,
    PERF_DIRTY_GROUPS = 1 << 2
};

enum PtcProp {
    PTCPROP_OCTAVE    = 0,
    PTCPROP_DETUNE    = 1,
    PTCPROP_SCALE     = 2,
    PTCPROP_TRANSPOSE = 3
};

enum ArpProp {
    ARPPROP_ENABLED = 0,
    ARPPROP_MODE    = 1,
    ARPPROP_RATE    = 2,
    ARPPROP_RANGE   = 3
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
enum ExtTrackProp {
    EXTPROP_LENGTH = 0, EXTPROP_SPEED = 1, EXTPROP_CHANNEL = 2
};
enum PatternProp {
    PPROP_LENGTH = 0, PPROP_SCALE = 1, PPROP_TEMPO = 2,
    PPROP_KIT = 3, PPROP_SCALE_MODE = 4
};

// Shared SPS wire helpers with step-protocol names kept for compatibility.
static const uint16_t kFrameMinLen = spswire::kFrameMinLen;
using Parsed = spswire::Parsed;

inline uint16_t spsSeqPack7(const uint8_t* in, uint16_t n, uint8_t* out) {
    return spswire::pack7(in, n, out);
}
inline uint16_t spsSeqUnpack7(const uint8_t* in, uint16_t encLen,
                              uint8_t* out, uint16_t outCap) {
    return spswire::unpack7(in, encLen, out, outCap);
}
inline uint16_t spsSeqPack7Size(uint16_t n) {
    return spswire::pack7Size(n);
}
inline uint16_t spsSeqUnpack7Size(uint16_t encLen) {
    return spswire::unpack7Size(encLen);
}
inline uint16_t spsSeqChecksum(const uint8_t* p, uint16_t n) {
    return spswire::checksum(p, n);
}
inline uint16_t spsSeqBuildFrame(uint8_t cmd, uint8_t tag,
                                 const uint8_t* body8, uint16_t body8len,
                                 uint8_t* out, uint16_t outcap) {
    return spswire::buildFrame(kMfrId, kSubId0, kSubId1, cmd, tag, body8,
                               body8len, out, outcap);
}
inline bool spsSeqParseFrame(const uint8_t* msg, uint16_t len, Parsed& out) {
    return spswire::parseFrame(kMfrId, kSubId0, kSubId1, msg, len, out);
}
inline uint16_t spsSeqDecodeBody(const Parsed& p, uint8_t* body8,
                                 uint16_t cap) {
    return spswire::decodeBody(p, body8, cap);
}
inline void spsSeqPutU64(uint8_t* p, uint64_t v) { spswire::putU64(p, v); }
inline uint64_t spsSeqGetU64(const uint8_t* p) { return spswire::getU64(p); }
inline void spsSeqPutU32(uint8_t* p, uint32_t v) { spswire::putU32(p, v); }
inline uint32_t spsSeqGetU32(const uint8_t* p) { return spswire::getU32(p); }

} // namespace spsseq

#endif // SPS_SEQ_PROTOCOL_H
