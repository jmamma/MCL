#pragma once

#if !defined(__AVR__)

#include "Host/SpsHostSeqBridge.h"

#include "Sequencer/MCLSeq.h"
#include "SPSXSeqTrack.h"
#include "StepSeqDefines.h"
#include "MidiUart.h"
#include "MidiClock.h"
#include "MidiSysex.h"
#include "MCLSysConfig.h"
#include "SeqTrackUtil.h"

namespace sps_host_seq_internal {

static inline void putU16le(uint8_t* p, uint16_t v) {
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)(v >> 8);
}

static inline uint16_t getU16le(const uint8_t* p) {
    return (uint16_t)(p[0] | (p[1] << 8));
}

static inline void putU32le(uint8_t* p, uint32_t v) {
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
    p[2] = (uint8_t)((v >> 16) & 0xFF);
    p[3] = (uint8_t)((v >> 24) & 0xFF);
}

static inline uint32_t getU32le(const uint8_t* p) {
    return ((uint32_t)p[0]) | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static inline SPSXSeqTrack* spsxTrack(int t) {
    if (t < 0 || t >= NUM_MD_TRACKS)
        return nullptr;
    return &mcl_seq.spsx_tracks[t];
}

static inline uint8_t spsxLongestTrackLength() {
    uint8_t longest = 0;
    for (int t = 0; t < NUM_MD_TRACKS; t++) {
        uint8_t length = mcl_seq.spsx_tracks[t].length;
        if (length > longest)
            longest = length;
    }
    return longest > 0 ? longest : 16;
}

static inline uint16_t mclLiveTempoRaw() {
    float bpm = MidiClock.get_tempo();
    if (bpm < 1.0f)
        bpm = mcl_cfg.tempo;
    int tenths = (int)(bpm * 10.0f + 0.5f);
    if (tenths < 300)
        tenths = 300;
    if (tenths > 3000)
        tenths = 3000;
    return (uint16_t)((tenths * 24 + 5) / 10);
}

static inline uint8_t extStepTrackCount() {
#ifdef EXT_TRACKS
    if (mcl_cfg.grid_y_device == GRID_Y_DEVICE_OFF)
        return 0;
    return SeqTrackUtil::track_count(false);
#else
    return 0;
#endif
}

static inline bool validExtStepTrack(uint8_t device, int track) {
    return device == spsseq::EXT_DEVICE_GRID_Y && track >= 0 &&
           track < extStepTrackCount();
}

static inline void setMclLiveTempoRaw(uint16_t raw) {
    if (raw < 720)
        raw = 720;
    if (raw > 7200)
        raw = 7200;
    int tenths = (raw * 10 + 12) / 24;
    float bpm = (float)tenths / 10.0f;
    mcl_cfg.tempo = bpm;
    MidiClock.setTempo(bpm);
}

}  // namespace sps_host_seq_internal

#endif  // !defined(__AVR__)
