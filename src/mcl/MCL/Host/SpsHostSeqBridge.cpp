#if !defined(__AVR__)

#include "Host/SpsHostSeqBridge.h"
#include "SpsHostSeqBridge_Internal.h"

using namespace spsseq;
using namespace sps_host_seq_internal;

SpsHostSeqBridge sps_host_seq_bridge;

void SpsHostSeqBridge::setup() {
    ready_ = false;
    negotiated_proto_version_ = kProtoVersionV1;
    negotiated_lock_params_ = kNumLockParamsV1;
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
        case CMD_REQ_EXT_TRACK_META:onReqExtTrackMeta(p.tag, b, n);   break;
        case CMD_REQ_EXT_NOTES:     onReqExtNotes(p.tag, b, n);       break;
        case CMD_REQ_PERF_STATE:    onReqPerfState(p.tag, b, n);      break;
        case CMD_REQ_EXT_LOCKS:     onReqExtLocks(p.tag, b, n);       break;
        case CMD_REQ_MIXER_STATE:   onReqMixerState(p.tag, b, n);     break;
        case CMD_REQ_PERF_PAGE_STATE:onReqPerfPageState(p.tag);        break;
        case CMD_REQ_LFO_STATE:     onReqLfoState(p.tag, b, n);       break;

        case CMD_SET_STEP:        if (applySetStep(b, n))        { if (n) notifyDirty(b[0], DIRTY_SUMMARY); } break;
        case CMD_SET_MICROTIMING: if (applySetMicroTiming(b, n)) { if (n) notifyDirty(b[0], DIRTY_DETAIL); }  break;
        case CMD_SET_CONDITION:   if (applySetCondition(b, n))   { if (n) notifyDirty(b[0], DIRTY_DETAIL); }  break;
        case CMD_SET_LOCK:        if (applySetLock(b, n))        { if (n) notifyDirty(b[0], (uint8_t)(DIRTY_LOCKS | DIRTY_SUMMARY)); } break;
        case CMD_CLR_LOCK:        if (applyClrLock(b, n))        { if (n) notifyDirty(b[0], (uint8_t)(DIRTY_LOCKS | DIRTY_SUMMARY)); } break;
        case CMD_CLEAR_STEP_RANGE:
            if (applyClearStepRange(b, n))
                notifyDirty(0xFF, (uint8_t)(DIRTY_SUMMARY | DIRTY_DETAIL | DIRTY_LOCKS));
            break;
        case CMD_STEP_CLIPBOARD:
            if (applyStepClipboard(b, n) && n >= 1 &&
                b[0] != STEP_CLIP_COPY)
                notifyDirty(0xFF, (uint8_t)(DIRTY_SUMMARY | DIRTY_DETAIL |
                                            DIRTY_LOCKS | DIRTY_META));
            break;
        case CMD_SET_TRACK_PROP:  if (applySetTrackProp(b, n))   { if (n) notifyDirty(b[0], DIRTY_SUMMARY); } break;
        case CMD_SET_PATTERN_PROP:if (applySetPatternProp(b, n)) { notifyDirty(0xFF, DIRTY_META); }          break;
        case CMD_EXT_ADD_NOTE:
            if (applyExtAddNote(b, n) && n >= 2)
                notifyExtDirty(b[0], b[1], EXT_DIRTY_NOTES);
            break;
        case CMD_EXT_DEL_NOTE:
            if (applyExtDeleteNote(b, n) && n >= 2)
                notifyExtDirty(b[0], b[1], EXT_DIRTY_NOTES);
            break;
        case CMD_EXT_TOGGLE_NOTE:
            if (applyExtToggleNote(b, n) && n >= 2)
                notifyExtDirty(b[0], b[1], EXT_DIRTY_NOTES);
            break;
        case CMD_EXT_CLEAR_RANGE:
            if (applyExtClearRange(b, n) && n >= 2)
                notifyExtDirty(b[0], b[1], EXT_DIRTY_NOTES);
            break;
        case CMD_EXT_SET_TRACK_PROP:
            if (applyExtSetTrackProp(b, n) && n >= 2)
                notifyExtDirty(b[0], b[1],
                               (uint8_t)(EXT_DIRTY_META | EXT_DIRTY_NOTES |
                                         EXT_DIRTY_LOCKS));
            break;
        case CMD_SET_PTC_PROP:
            if (applySetPtcProp(b, n) && n >= 2) {
                notifyPerfDirty(b[0], b[1], PERF_DIRTY_PTC);
                if (n >= 3 && b[2] == PTCPROP_LENGTH) {
                    notifyExtDirty(
                        b[0], b[1],
                        (uint8_t)(EXT_DIRTY_META | EXT_DIRTY_NOTES |
                                  EXT_DIRTY_LOCKS));
                }
            }
            break;
        case CMD_SET_ARP_PROP:
            if (applySetArpProp(b, n) && n >= 2)
                notifyPerfDirty(b[0], b[1], PERF_DIRTY_ARP);
            break;
        case CMD_SET_PTC_GROUP:
            if (applySetPtcGroup(b, n))
                notifyPerfDirty(EXT_DEVICE_GRID_Y, 0xFF, PERF_DIRTY_GROUPS);
            break;
        case CMD_PTC_NOTE_EVENT:
            if (applyPtcNoteEvent(b, n) && n >= 2)
                notifyPerfDirty(b[0], b[1],
                                (uint8_t)(PERF_DIRTY_PTC | PERF_DIRTY_ARP));
            break;
        case CMD_EXT_SET_LOCK:
            if (applyExtSetLock(b, n) && n >= 2)
                notifyExtDirty(b[0], b[1], EXT_DIRTY_LOCKS);
            break;
        case CMD_EXT_CLR_LOCK:
            if (applyExtClearLock(b, n) && n >= 2)
                notifyExtDirty(b[0], b[1], EXT_DIRTY_LOCKS);
            break;
        case CMD_EXT_CLEAR_LOCKS:
            if (applyExtClearLocks(b, n) && n >= 2)
                notifyExtDirty(b[0], b[1], EXT_DIRTY_LOCKS);
            break;
        case CMD_MIXER_SET_PARAM:
            if (applyMixerSetParam(b, n) && n >= 1)
                notifyMixerDirty(b[0], MIXER_DIRTY_STATE);
            break;
        case CMD_MIXER_ADJUST_PARAM:
            if (applyMixerAdjustParam(b, n) && n >= 1)
                notifyMixerDirty(b[0], MIXER_DIRTY_STATE);
            break;
        case CMD_MIXER_SET_MASK:
            if (applyMixerSetMask(b, n) && n >= 1)
                notifyMixerDirty(b[0], MIXER_DIRTY_STATE);
            break;
        case CMD_MIXER_LOAD_PERF:
            if (applyMixerLoadPerf(b, n))
                notifyMixerDirty(0xFF, MIXER_DIRTY_STATE);
            break;
        case CMD_MIXER_SET_DISPLAY:
            if (applyMixerSetDisplay(b, n) && n >= 1)
                notifyMixerDirty(b[0], MIXER_DIRTY_STATE);
            break;
        case CMD_MIXER_SET_PERF_LOCK:
            if (applyMixerSetPerfLock(b, n))
                notifyMixerDirty(0xFF, MIXER_DIRTY_STATE);
            break;
        case CMD_PERF_PAGE_SET_CONTROL:
            if (applyPerfPageSetControl(b, n))
                notifyPerfPageDirty(PERF_PAGE_DIRTY_STATE);
            break;
        case CMD_PERF_PAGE_SET_ACTIVE_SCENE:
            if (applyPerfPageSetActiveScene(b, n))
                notifyPerfPageDirty(PERF_PAGE_DIRTY_STATE);
            break;
        case CMD_PERF_PAGE_SET_SCENE_PARAM:
            if (applyPerfPageSetSceneParam(b, n))
                notifyPerfPageDirty(PERF_PAGE_DIRTY_STATE);
            break;
        case CMD_PERF_PAGE_SCENE_ACTION:
            if (applyPerfPageSceneAction(b, n))
                notifyPerfPageDirty(PERF_PAGE_DIRTY_STATE);
            break;
        case CMD_PERF_PAGE_SET_VIEW:
            if (applyPerfPageSetView(b, n))
                notifyPerfPageDirty(PERF_PAGE_DIRTY_STATE);
            break;
        case CMD_LFO_SET_PROP:
            if (applyLfoSetProp(b, n) && n >= 2)
                notifyLfoDirty(b[0], b[1], LFO_DIRTY_STATE);
            break;
        case CMD_LFO_SET_MASK:
            if (applyLfoSetMask(b, n) && n >= 2)
                notifyLfoDirty(b[0], b[1], LFO_DIRTY_STATE);
            break;
        case CMD_LFO_ACTION:
            if (applyLfoAction(b, n) && n >= 2)
                notifyLfoDirty(b[0], b[1], LFO_DIRTY_STATE);
            break;

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
                    case CMD_CLR_STEP_LOCKS:  applyClearStepLocks(sb, sl); break;
                    case CMD_CLEAR_STEP_RANGE: applyClearStepRange(sb, sl); break;
                    default: break;
                }
                off = (uint16_t)(off + sl);
            }
            notifyDirty(0xFF, (uint8_t)(DIRTY_SUMMARY | DIRTY_DETAIL | DIRTY_LOCKS));
            break;
        }
        case CMD_CLR_STEP_LOCKS: {
            if (applyClearStepLocks(b, n) && n >= 2) {
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
    if (n < 1 || b[0] == 0) return;
    negotiated_proto_version_ = spsSeqNegotiateVersion(b[0]);
    negotiated_lock_params_ =
        spsSeqLockParamsForVersion(negotiated_proto_version_);
    if (negotiated_lock_params_ == 0) return;
    ready_ = true;
    uint16_t caps = CAP_SPSX | CAP_LOCKS | CAP_DETAIL | CAP_PER_TRACK_LEN |
                    CAP_BATCH | CAP_STEP_CLIPBOARD | CAP_ACCENT |
                    CAP_EXT_NOTES | CAP_PTC_ARP | CAP_EXT_LOCKS |
                    CAP_EXT_NOTE_TOGGLE | CAP_MIXER | CAP_PERF_PAGE |
                    CAP_LFO_PAGE;
    uint8_t body[7];
    body[0] = negotiated_proto_version_; putU16le(body + 1, caps);
    body[3] = (uint8_t)NUM_MD_TRACKS; body[4] = (uint8_t)kNumSteps;
    body[5] = negotiated_lock_params_;
    body[6] = extStepTrackCount();
    sendFrame(CMD_HELLO_ACK, tag, body, (uint16_t)sizeof body);
}

void SpsHostSeqBridge::onReqActive(uint8_t tag)          { sendPatternMeta(CMD_NOTIFY_ACTIVE, tag); }

void SpsHostSeqBridge::onReqPatternMeta(uint8_t tag)     { sendPatternMeta(CMD_PATTERN_META, tag); }

void SpsHostSeqBridge::onReqTrackDetail(uint8_t tag, const uint8_t* b, uint16_t n) { (void)tag; if (n >= 1) sendTrackDetail(b[0]); }

void SpsHostSeqBridge::onReqTrackLocks(uint8_t tag, const uint8_t* b, uint16_t n)  { (void)tag; if (n >= 1) sendTrackLocks(b[0]); }

void SpsHostSeqBridge::onReqExtTrackMeta(uint8_t tag, const uint8_t* b,
                                         uint16_t n) {
    if (n >= 2)
        sendExtTrackMeta(tag, b[0], b[1]);
}

void SpsHostSeqBridge::onReqExtNotes(uint8_t tag, const uint8_t* b,
                                     uint16_t n) {
    if (n >= 2)
        sendExtNotes(tag, b[0], b[1]);
}

void SpsHostSeqBridge::onReqPerfState(uint8_t tag, const uint8_t* b,
                                      uint16_t n) {
    if (n >= 2)
        sendPerfState(tag, b[0], b[1]);
}

void SpsHostSeqBridge::onReqExtLocks(uint8_t tag, const uint8_t* b,
                                     uint16_t n) {
    if (n >= 3)
        sendExtLocks(tag, b[0], b[1], b[2]);
}

void SpsHostSeqBridge::onReqMixerState(uint8_t tag, const uint8_t* b,
                                       uint16_t n) {
    sendMixerState(tag, n >= 1 ? b[0] : MIXER_DEVICE_PRIMARY);
}

void SpsHostSeqBridge::onReqPerfPageState(uint8_t tag) {
    sendPerfPageState(tag);
}

void SpsHostSeqBridge::onReqLfoState(uint8_t tag, const uint8_t* b,
                                     uint16_t n) {
    if (n >= 2)
        sendLfoState(tag, b[0], b[1]);
}

#endif  // !defined(__AVR__)
