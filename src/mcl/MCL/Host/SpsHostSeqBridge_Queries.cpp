#if !defined(__AVR__)

#include "SpsHostSeqBridge.h"
#include "SpsHostSeqBridge_Internal.h"

using namespace spsseq;
using namespace sps_host_seq_internal;

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
    putU16le(body + 6, mclLiveTempoRaw());           // live tempo slot source
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

#endif  // !defined(__AVR__)
