/**
 * SpsHostSeqBridge — MCL side of the SPS<->MCL sequencer control protocol.
 *
 * Listens for F0 7D 53 51 ... frames on the host-facing MIDI port, decodes them
 * (verifying the 53 51 sub-id + checksum), and serves queries / applies edits
 * against the SPS-X sequencer (mcl_seq.spsx_tracks[]). Emits responses and
 * NOTIFY_* via MidiUart.sendRaw (self-framed; NOT sendSysex, which is uint8 cnt).
 *
 * SPS-X only: SPS-X is the wire model. Replies carry CAP_SPSX so the host knows
 * the engine is supported. Spec: plans/PROTOCOL_SPS_MCL_SEQ_GRID.md and the
 * mirrored SpsSeqProtocol.h.
 */
#ifndef SPS_HOST_SEQ_BRIDGE_H
#define SPS_HOST_SEQ_BRIDGE_H

#if !defined(__AVR__)

#include "MidiSysex.h"
#include "Host/SpsSeqProtocol.h"

class SpsHostSeqBridge : public MidiSysexListenerClass {
public:
    SpsHostSeqBridge()
        : MidiSysexListenerClass(nullptr, spsseq::kMfrId, spsseq::kSubId0, spsseq::kSubId1) {}

    // Register on the host-facing MidiSysex dispatcher (call from MCL init).
    void setup();

    // MidiSysexListenerClass
    void end() override;

    // Emit hooks — call from MCL when state changes (see .cpp for guidance):
    void notifyDirty(int track, uint8_t regions);   // track 0xFF = all
    void notifyTracksDirty(uint16_t track_mask, uint8_t regions);  // per-bit; coalesces broad masks
    void notifyExtDirty(uint8_t device, int track, uint8_t regions);
    void notifyPerfDirty(uint8_t device, int track, uint8_t regions);
    void notifyMixerDirty(uint8_t device, uint8_t regions);
    void notifyTransport(bool running, uint8_t masterStep);
    void notifyActive();

private:
    bool ready_ = false;

    void handle(const spsseq::Parsed& p, const uint8_t* b, uint16_t n);

    void sendFrame(uint8_t cmd, uint8_t tag, const uint8_t* body, uint16_t bodyLen);
    void sendAck(uint8_t tag, uint8_t status);
    void sendErr(uint8_t tag, uint8_t code, uint8_t detail);

    void onHello(uint8_t tag, const uint8_t* b, uint16_t n);
    void onReqActive(uint8_t tag);
    void onReqTrackSummary(uint8_t tag, const uint8_t* b, uint16_t n);
    void onReqTrackDetail(uint8_t tag, const uint8_t* b, uint16_t n);
    void onReqTrackLocks(uint8_t tag, const uint8_t* b, uint16_t n);
    void onReqPatternMeta(uint8_t tag);
    void onReqExtTrackMeta(uint8_t tag, const uint8_t* b, uint16_t n);
    void onReqExtNotes(uint8_t tag, const uint8_t* b, uint16_t n);
    void onReqPerfState(uint8_t tag, const uint8_t* b, uint16_t n);
    void onReqExtLocks(uint8_t tag, const uint8_t* b, uint16_t n);
    void onReqMixerState(uint8_t tag, const uint8_t* b, uint16_t n);
    void sendTrackSummary(int track);
    void sendTrackDetail(int track);
    void sendTrackLocks(int track);
    void sendPatternMeta(uint8_t cmd, uint8_t tag);  // PATTERN_META or NOTIFY_ACTIVE
    void sendExtTrackMeta(uint8_t tag, uint8_t device, int track);
    void sendExtNotes(uint8_t tag, uint8_t device, int track);
    void sendExtLocks(uint8_t tag, uint8_t device, int track,
                      uint8_t lock_idx);
    void sendPerfState(uint8_t tag, uint8_t device, int track);
    void sendMixerState(uint8_t tag, uint8_t device);

    bool applySetStep(const uint8_t* b, uint16_t n);
    bool applySetMicroTiming(const uint8_t* b, uint16_t n);
    bool applySetCondition(const uint8_t* b, uint16_t n);
    bool applySetLock(const uint8_t* b, uint16_t n);
    bool applyClrLock(const uint8_t* b, uint16_t n);
    bool applySetTrackProp(const uint8_t* b, uint16_t n);
    bool applySetPatternProp(const uint8_t* b, uint16_t n);
    bool applyExtAddNote(const uint8_t* b, uint16_t n);
    bool applyExtDeleteNote(const uint8_t* b, uint16_t n);
    bool applyExtToggleNote(const uint8_t* b, uint16_t n);
    bool applyExtClearRange(const uint8_t* b, uint16_t n);
    bool applyExtSetTrackProp(const uint8_t* b, uint16_t n);
    bool applyExtSetLock(const uint8_t* b, uint16_t n);
    bool applyExtClearLock(const uint8_t* b, uint16_t n);
    bool applyExtClearLocks(const uint8_t* b, uint16_t n);
    bool applySetPtcProp(const uint8_t* b, uint16_t n);
    bool applySetArpProp(const uint8_t* b, uint16_t n);
    bool applySetPtcGroup(const uint8_t* b, uint16_t n);
    bool applyPtcNoteEvent(const uint8_t* b, uint16_t n);
    bool applyMixerSetParam(const uint8_t* b, uint16_t n);
    bool applyMixerAdjustParam(const uint8_t* b, uint16_t n);
    bool applyMixerSetMask(const uint8_t* b, uint16_t n);
    bool applyMixerLoadPerf(const uint8_t* b, uint16_t n);
    bool applyMixerSetDisplay(const uint8_t* b, uint16_t n);
    bool applyMixerSetPerfLock(const uint8_t* b, uint16_t n);

    // wire mask -> MCL StepSeq native (STEPSEQ_MASK_*); -1 if unknown
    static int wireToMclMask(int wmask);
};

extern SpsHostSeqBridge sps_host_seq_bridge;

#endif // !defined(__AVR__)
#endif // SPS_HOST_SEQ_BRIDGE_H
