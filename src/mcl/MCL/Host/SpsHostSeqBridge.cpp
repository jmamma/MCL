#if !defined(__AVR__)

#include "SpsHostSeqBridge.h"
#include "SpsHostSeqBridge_Internal.h"

using namespace spsseq;
using namespace sps_host_seq_internal;

SpsHostSeqBridge sps_host_seq_bridge;

void SpsHostSeqBridge::setup() {
    ready_ = true;
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

#endif  // !defined(__AVR__)
