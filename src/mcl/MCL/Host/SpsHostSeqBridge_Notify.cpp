#if !defined(__AVR__)

#include "Host/SpsHostSeqBridge.h"
#include "SpsHostSeqBridge_Internal.h"

using namespace spsseq;
using namespace sps_host_seq_internal;

void SpsHostSeqBridge::notifyDirty(int track, uint8_t regions) {
    if (!ready_) return;
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

void SpsHostSeqBridge::notifyExtDirty(uint8_t device, int track,
                                      uint8_t regions) {
    if (!ready_)
        return;
    uint8_t b[3] = {device, (uint8_t)track, regions};
    sendFrame(CMD_NOTIFY_EXT_DIRTY, 0, b, (uint16_t)sizeof b);
}

void SpsHostSeqBridge::notifyPerfDirty(uint8_t device, int track,
                                       uint8_t regions) {
    if (!ready_)
        return;
    uint8_t b[3] = {device, (uint8_t)track, regions};
    sendFrame(CMD_NOTIFY_PERF_DIRTY, 0, b, (uint16_t)sizeof b);
}

void SpsHostSeqBridge::notifyMixerDirty(uint8_t device, uint8_t regions) {
    if (!ready_)
        return;
    uint8_t b[2] = {device, regions};
    sendFrame(CMD_NOTIFY_MIXER_DIRTY, 0, b, (uint16_t)sizeof b);
}

void SpsHostSeqBridge::notifyPerfPageDirty(uint8_t regions) {
    if (!ready_)
        return;
    uint8_t b = regions;
    sendFrame(CMD_NOTIFY_PERF_PAGE_DIRTY, 0, &b, 1);
}

void SpsHostSeqBridge::notifyLfoDirty(uint8_t device, int track,
                                      uint8_t regions) {
    if (!ready_)
        return;
    uint8_t b[3] = {device, (uint8_t)track, regions};
    sendFrame(CMD_NOTIFY_LFO_DIRTY, 0, b, (uint16_t)sizeof b);
}

void SpsHostSeqBridge::notifyTransport(bool running, uint8_t masterStep) {
    if (!ready_) return;
    uint8_t b[4] = { (uint8_t)(running ? 1 : 0), masterStep, 0, 0 };
    sendFrame(CMD_NOTIFY_TRANSPORT, 0, b, 4);
}

void SpsHostSeqBridge::notifyActive() {
    if (!ready_) return;
    sendPatternMeta(CMD_NOTIFY_ACTIVE, 0);
}

#endif  // !defined(__AVR__)
