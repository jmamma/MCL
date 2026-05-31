/**
 * SpsHostSeqBridge — implementation. See SpsHostSeqBridge.h / SpsSeqProtocol.h.
 */
#if !defined(__AVR__)

#include "SpsHostSeqBridge.h"

#include "MCLSeq.h"                 // mcl_seq, spsx_tracks[], SPSXSeqTrack
#include "SPSXSeqTrack.h"
#include "StepSeqDefines.h"        // STEPSEQ_MASK_*
#include "MidiUart.h"              // MidiUart (host-facing port)
#include "MidiClock.h"
#include "MidiSysex.h"             // MidiSysex dispatcher

using namespace spsseq;

SpsHostSeqBridge sps_host_seq_bridge;

// ---- helpers ----
static inline void putU16le(uint8_t* p, uint16_t v) { p[0] = (uint8_t)(v & 0xFF); p[1] = (uint8_t)(v >> 8); }
static inline uint16_t getU16le(const uint8_t* p) { return (uint16_t)(p[0] | (p[1] << 8)); }

static inline SPSXSeqTrack* spsxTrack(int t) {
    if (t < 0 || t >= NUM_MD_TRACKS) return nullptr;
    return &mcl_seq.spsx_tracks[t];
}

static uint8_t spsxLongestTrackLength() {
    uint8_t longest = 0;
    for (int t = 0; t < NUM_MD_TRACKS; t++) {
        uint8_t length = mcl_seq.spsx_tracks[t].length;
        if (length > longest) longest = length;
    }
    return longest > 0 ? longest : 16;
}

int SpsHostSeqBridge::wireToMclMask(int w) {
    switch (w) {
        case WMASK_TRIG:          return STEPSEQ_MASK_PATTERN;
        case WMASK_MUTE:          return STEPSEQ_MASK_MUTE;
        case WMASK_SLIDE:         return STEPSEQ_MASK_SLIDE;
        case WMASK_SWING:         return STEPSEQ_MASK_SWING;
        // No WMASK_LOCK_ENABLED: MCL has no lock-enable gate. Lock values use
        // SET_LOCK/CLR_LOCK; WMASK_LOCKS_ON_STEP is read-only (no SET_STEP).
        default:                  return -1;
    }
}

// ============================================================================
// Registration / receive
// ============================================================================
void SpsHostSeqBridge::setup() {
    MidiSysex.addSysexListener(this);
}

void SpsHostSeqBridge::end() {
    SysexView view(sysex, msg_rd);
    uint16_t len = view.get_recordLen();
    if (len < kFrameMinLen || len > 2048) return;
    uint8_t buf[2048];
    for (uint16_t i = 0; i < len; i++) buf[i] = view.getByte(i);
    Parsed p;
    if (!spsSeqParseFrame(buf, len, p)) return;  // foreign 0x7D / bad checksum
    if (spsSeqUnpack7Size(p.body7len) > kMaxBodyRaw) return;  // reject oversized, don't truncate
    uint8_t body[kMaxBodyRaw + 64];
    uint16_t bl = spsSeqDecodeBody(p, body, (uint16_t)sizeof body);
    handle(p, body, bl);
}

void SpsHostSeqBridge::handle(const Parsed& p, const uint8_t* b, uint16_t n) {
    switch (p.cmd) {
        case CMD_HELLO:             onHello(p.tag, b, n);              break;
        case CMD_REQ_ACTIVE:        onReqActive(p.tag);               break;
        case CMD_REQ_TRACK_SUMMARY: onReqTrackSummary(p.tag, b, n);   break;
        case CMD_REQ_TRACK_DETAIL:  onReqTrackDetail(p.tag, b, n);    break;
        case CMD_REQ_TRACK_LOCKS:   onReqTrackLocks(p.tag, b, n);     break;
        case CMD_REQ_PATTERN_META:  onReqPatternMeta(p.tag);          break;

        case CMD_SET_STEP:        if (applySetStep(b, n))        { if (n) notifyDirty(b[0], DIRTY_SUMMARY); } break;
        case CMD_SET_MICROTIMING: if (applySetMicroTiming(b, n)) { if (n) notifyDirty(b[0], DIRTY_DETAIL); }  break;
        case CMD_SET_CONDITION:   if (applySetCondition(b, n))   { if (n) notifyDirty(b[0], DIRTY_DETAIL); }  break;
        case CMD_SET_LOCK:        if (applySetLock(b, n))        { if (n) notifyDirty(b[0], (uint8_t)(DIRTY_LOCKS | DIRTY_SUMMARY)); } break;
        case CMD_CLR_LOCK:        if (applyClrLock(b, n))        { if (n) notifyDirty(b[0], (uint8_t)(DIRTY_LOCKS | DIRTY_SUMMARY)); } break;
        case CMD_SET_TRACK_PROP:  if (applySetTrackProp(b, n))   { if (n) notifyDirty(b[0], DIRTY_SUMMARY); } break;
        case CMD_SET_PATTERN_PROP:if (applySetPatternProp(b, n)) { notifyDirty(0xFF, DIRTY_META); }          break;

        case CMD_BATCH: {
            // sequential best-effort: {cmd,len,bytes}* ; correctness via NOTIFY_DIRTY
            uint16_t off = 1;
            uint8_t count = n ? b[0] : 0;
            for (uint8_t i = 0; i < count && off + 2 <= n; i++) {
                uint8_t sc = b[off++], sl = b[off++];
                if (off + sl > n) break;
                const uint8_t* sb = b + off;
                switch (sc) {
                    case CMD_SET_STEP:        applySetStep(sb, sl);        break;
                    case CMD_SET_MICROTIMING: applySetMicroTiming(sb, sl); break;
                    case CMD_SET_CONDITION:   applySetCondition(sb, sl);   break;
                    case CMD_SET_LOCK:        applySetLock(sb, sl);        break;
                    case CMD_CLR_LOCK:        applyClrLock(sb, sl);        break;
                    default: break;
                }
                off = (uint16_t)(off + sl);
            }
            notifyDirty(0xFF, (uint8_t)(DIRTY_SUMMARY | DIRTY_DETAIL | DIRTY_LOCKS));
            break;
        }
        case CMD_CLR_STEP_LOCKS: {
            SPSXSeqTrack* tr = (n >= 2) ? spsxTrack(b[0]) : nullptr;
            if (tr && b[1] < kNumSteps) {
                tr->clear_step_locks(b[1]);  // clears ALL param locks on the step
                notifyDirty(b[0], (uint8_t)(DIRTY_LOCKS | DIRTY_SUMMARY));
            }
            break;
        }
        default: sendErr(p.tag, ERR_UNKNOWN_CMD, 0); break;
    }
}

// ============================================================================
// Send
// ============================================================================
void SpsHostSeqBridge::sendFrame(uint8_t cmd, uint8_t tag, const uint8_t* body, uint16_t bodyLen) {
    uint8_t frame[1 + 3 + 2 + spsseq::kMaxBodyRaw * 2 + 8];
    uint16_t n = spsSeqBuildFrame(cmd, tag, body, bodyLen, frame, (uint16_t)sizeof frame);
    if (n) MidiUart.sendRaw(frame, n);
}
void SpsHostSeqBridge::sendAck(uint8_t tag, uint8_t status) { uint8_t b = status; sendFrame(CMD_ACK, tag, &b, 1); }
void SpsHostSeqBridge::sendErr(uint8_t tag, uint8_t code, uint8_t detail) { uint8_t b[2] = { code, detail }; sendFrame(CMD_ERR, tag, b, 2); }

void SpsHostSeqBridge::onHello(uint8_t tag, const uint8_t* b, uint16_t n) {
    if (n >= 1 && b[0] == 0) return;  // malformed/incompatible host proto: stay silent
    uint16_t caps = CAP_SPSX | CAP_LOCKS | CAP_DETAIL | CAP_PER_TRACK_LEN | CAP_BATCH;
    uint8_t body[6];
    body[0] = kProtoVersion; putU16le(body + 1, caps);
    body[3] = (uint8_t)NUM_MD_TRACKS; body[4] = (uint8_t)kNumSteps; body[5] = (uint8_t)kNumLockParams;
    sendFrame(CMD_HELLO_ACK, tag, body, 6);
}

void SpsHostSeqBridge::onReqActive(uint8_t tag)          { sendPatternMeta(CMD_NOTIFY_ACTIVE, tag); }
void SpsHostSeqBridge::onReqPatternMeta(uint8_t tag)     { sendPatternMeta(CMD_PATTERN_META, tag); }
void SpsHostSeqBridge::onReqTrackDetail(uint8_t tag, const uint8_t* b, uint16_t n) { (void)tag; if (n >= 1) sendTrackDetail(b[0]); }
void SpsHostSeqBridge::onReqTrackLocks(uint8_t tag, const uint8_t* b, uint16_t n)  { (void)tag; if (n >= 1) sendTrackLocks(b[0]); }

void SpsHostSeqBridge::onReqTrackSummary(uint8_t tag, const uint8_t* b, uint16_t n) {
    (void)tag;
    if (n < 2) return;
    uint16_t mask = getU16le(b);
    for (int t = 0; t < NUM_MD_TRACKS; t++)
        if (mask & (1u << t)) sendTrackSummary(t);
}

void SpsHostSeqBridge::sendTrackSummary(int track) {
    SPSXSeqTrack* tr = spsxTrack(track);
    if (!tr) return;
    uint8_t body[7 + 5 * 8];
    body[0] = (uint8_t)track;
    body[1] = 0x40;                      // ver (SPS-X)
    body[2] = tr->length;                // pattern length proxy (effective)
    body[3] = tr->track_length;          // 0 = follow pattern
    body[4] = tr->track_speed;
    body[5] = 0;                         // scale (TODO: from pattern)
    body[6] = 0;                         // swing amount (TODO)
    uint8_t* m = body + 7;
    spsSeqPutU64(m + 0 * 8, tr->trig_mask);
    spsSeqPutU64(m + 1 * 8, tr->slide_mask);
    spsSeqPutU64(m + 2 * 8, tr->swing_mask);
    spsSeqPutU64(m + 3 * 8, tr->mute_mask);
    uint64_t lockPresent = 0;
    for (int s = 0; s < kNumSteps; s++)
        if (tr->steps[s].locks != 0) lockPresent |= (1ULL << s);
    spsSeqPutU64(m + 4 * 8, lockPresent);
    sendFrame(CMD_TRACK_SUMMARY, 0, body, (uint16_t)sizeof body);
}

void SpsHostSeqBridge::sendTrackDetail(int track) {
    SPSXSeqTrack* tr = spsxTrack(track);
    if (!tr) return;
    uint8_t body[2 + kNumSteps + kNumSteps];
    body[0] = (uint8_t)track; body[1] = 0x40;
    for (int s = 0; s < kNumSteps; s++) body[2 + s] = (uint8_t)(int8_t)tr->microtiming[s];
    for (int s = 0; s < kNumSteps; s++) {
        uint8_t c = (uint8_t)(tr->steps[s].cond_id & 0x3F);
        if (tr->steps[s].cond_plock) c |= 0x80;
        body[2 + kNumSteps + s] = c;
    }
    sendFrame(CMD_TRACK_DETAIL, 0, body, (uint16_t)sizeof body);
}

void SpsHostSeqBridge::sendTrackLocks(int track) {
    SPSXSeqTrack* tr = spsxTrack(track);
    if (!tr) return;
    // active params, ascending
    uint8_t params[kNumLockParams]; int np = 0;
    for (int param = 0; param < kNumLockParams; param++)
        if (tr->find_param((uint8_t)param) != 255) params[np++] = (uint8_t)param;
    int colBytes = (np + 6) / 7;
    if (colBytes < 1) colBytes = 1;

    // Pre-build every step entry that carries locks.
    struct Entry { uint8_t step; uint64_t colMask; uint8_t nv; uint8_t vals[kNumLockParams]; };
    Entry entries[kNumSteps]; int ne = 0;
    for (int s = 0; s < kNumSteps; s++) {
        uint64_t colMask = 0; uint8_t vals[kNumLockParams]; int nv = 0;
        for (int q = 0; q < np; q++) {
            uint8_t slot = tr->find_param(params[q]);
            if (slot != 255 && tr->steps[s].is_lock_bit(slot)) {
                uint16_t idx = tr->get_lockidx((uint8_t)s, slot);
                if (idx < STEPSEQ_NUM_LOCK_SLOTS) { colMask |= (1ULL << q); vals[nv++] = (uint8_t)(tr->locks[idx] & 0x7F); }
            }
        }
        if (!colMask) continue;
        entries[ne].step = (uint8_t)s; entries[ne].colMask = colMask; entries[ne].nv = (uint8_t)nv;
        for (int v = 0; v < nv; v++) entries[ne].vals[v] = vals[v];
        ne++;
    }

    const int hdr = 5 + np + 1;                 // track,ver,page,page_count,np + params[np] + num_entries
    const int budget = (int)kMaxBodyRaw - hdr;  // entry bytes per page
    // Pass 1: greedy page count (always >= 1; >=1 entry per page).
    int pageCount = 1, rem = budget;
    for (int e = 0; e < ne; e++) {
        int esz = 1 + colBytes + entries[e].nv;
        if (esz > rem) { pageCount++; rem = budget; }
        rem -= esz;
    }
    // Pass 2: emit pages with the identical greedy fill so the partition matches.
    int e = 0;
    for (int page = 0; page < pageCount; page++) {
        uint8_t body[kMaxBodyRaw]; uint16_t off = 0;
        body[off++] = (uint8_t)track; body[off++] = 0x40;
        body[off++] = (uint8_t)page; body[off++] = (uint8_t)pageCount;
        body[off++] = (uint8_t)np;
        for (int q = 0; q < np; q++) body[off++] = params[q];
        uint16_t entriesOff = off++; uint8_t entCount = 0; int prem = budget;
        while (e < ne) {
            int esz = 1 + colBytes + entries[e].nv;
            if (entCount > 0 && esz > prem) break;
            body[off++] = entries[e].step;
            for (int cb = 0; cb < colBytes; cb++) body[off++] = (uint8_t)((entries[e].colMask >> (7 * cb)) & 0x7F);
            for (int v = 0; v < entries[e].nv; v++) body[off++] = entries[e].vals[v];
            prem -= esz; entCount++; e++;
        }
        body[entriesOff] = entCount;
        sendFrame(CMD_TRACK_LOCKS, 0, body, off);
    }
}

void SpsHostSeqBridge::sendPatternMeta(uint8_t cmd, uint8_t tag) {
    // PATTERN_META(10) [+ activeTrack + transport for NOTIFY_ACTIVE]
    uint8_t body[12];
    body[0] = 0x40;                                  // version
    body[1] = 0;                                     // currentPattern (TODO: MD globals)
    body[2] = 0;                                     // kit            (TODO)
    body[3] = spsxLongestTrackLength();              // fake master length
    body[4] = 0;                                     // scale          (TODO)
    body[5] = 0;                                     // doubleTempo    (TODO)
    putU16le(body + 6, 0);                           // tempo          (TODO)
    body[8] = 0;                                     // scaleMode      (TODO)
    body[9] = 0;                                     // chainChange    (TODO)
    if (cmd == CMD_NOTIFY_ACTIVE) {
        body[10] = 0;                                // activeTrack    (TODO)
        body[11] = 0;                                // transport      (TODO)
        sendFrame(cmd, tag, body, 12);
    } else {
        sendFrame(cmd, tag, body, 10);
    }
}

// ============================================================================
// Edits (apply to spsx_tracks)
// ============================================================================
bool SpsHostSeqBridge::applySetStep(const uint8_t* b, uint16_t n) {
    if (n < 4) return false;
    SPSXSeqTrack* tr = spsxTrack(b[0]);
    int mclMask = wireToMclMask(b[2]);
    if (!tr || mclMask < 0 || b[1] >= kNumSteps) return false;
    tr->set_step(b[1], (uint8_t)mclMask, b[3] != 0);
    return true;
}
bool SpsHostSeqBridge::applySetMicroTiming(const uint8_t* b, uint16_t n) {
    if (n < 3) return false;
    SPSXSeqTrack* tr = spsxTrack(b[0]);
    if (!tr || b[1] >= kNumSteps) return false;
    tr->microtiming[b[1]] = (int8_t)b[2];
    return true;
}
bool SpsHostSeqBridge::applySetCondition(const uint8_t* b, uint16_t n) {
    if (n < 3) return false;
    SPSXSeqTrack* tr = spsxTrack(b[0]);
    if (!tr || b[1] >= kNumSteps) return false;
    tr->steps[b[1]].cond_id = (uint8_t)(b[2] & 0x3F);
    tr->steps[b[1]].cond_plock = (b[2] & 0x80) != 0;
    return true;
}
bool SpsHostSeqBridge::applySetLock(const uint8_t* b, uint16_t n) {
    if (n < 4) return false;
    SPSXSeqTrack* tr = spsxTrack(b[0]);
    if (!tr || b[1] >= kNumSteps || b[2] >= kNumLockParams) return false;  // param range
    tr->set_track_locks(b[1], b[2], (uint8_t)(b[3] & 0x7F));
    return true;
}
bool SpsHostSeqBridge::applyClrLock(const uint8_t* b, uint16_t n) {
    if (n < 3) return false;
    SPSXSeqTrack* tr = spsxTrack(b[0]);
    if (!tr || b[1] >= kNumSteps || b[2] >= kNumLockParams) return false;  // param range
    tr->clear_step_lock(b[1], b[2]);
    return true;
}
bool SpsHostSeqBridge::applySetTrackProp(const uint8_t* b, uint16_t n) {
    if (n < 3) return false;
    SPSXSeqTrack* tr = spsxTrack(b[0]);
    if (!tr) return false;
    switch (b[1]) {
        // Set the stored override AND apply via the canonical setters so the
        // effective length/speed and runtime timing update live.
        case TPROP_LENGTH: tr->track_length = b[2]; tr->set_length(b[2]); break;
        case TPROP_SPEED:  tr->track_speed  = b[2]; tr->set_speed(b[2]);  break;
        default: return false;  // scale/swing/mute: TODO
    }
    return true;
}
bool SpsHostSeqBridge::applySetPatternProp(const uint8_t* b, uint16_t n) {
    (void)b; (void)n;
    return false;  // TODO: wire to MD pattern globals
}

// ============================================================================
// Notifications (call from MCL state-change sites; SET_* handlers self-emit)
// ============================================================================
void SpsHostSeqBridge::notifyDirty(int track, uint8_t regions) {
    uint8_t b[2] = { (uint8_t)track, regions };
    sendFrame(CMD_NOTIFY_DIRTY, 0, b, 2);
}
void SpsHostSeqBridge::notifyTracksDirty(uint16_t mask, uint8_t regions) {
    if (!mask) return;
    int count = 0;
    for (int t = 0; t < kNumTracks; t++) if (mask & (1u << t)) count++;
    if (count >= 8) { notifyDirty(0xFF, regions); return; }  // broad change: one all-tracks notify
    for (int t = 0; t < kNumTracks; t++)
        if (mask & (1u << t)) notifyDirty(t, regions);
}
void SpsHostSeqBridge::notifyTransport(bool running, uint8_t masterStep) {
    uint8_t b[4] = { (uint8_t)(running ? 1 : 0), masterStep, 0, 0 };
    sendFrame(CMD_NOTIFY_TRANSPORT, 0, b, 4);
}
void SpsHostSeqBridge::notifyActive() { sendPatternMeta(CMD_NOTIFY_ACTIVE, 0); }

#endif // !defined(__AVR__)
