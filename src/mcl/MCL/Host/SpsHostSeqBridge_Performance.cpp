#if !defined(__AVR__)

#include "Host/SpsHostSeqBridge.h"
#include "SpsHostSeqBridge_Internal.h"

#include "GUI/Pages/Sequencer/SeqPages.h"
#include "LFOTrackRef.h"
#include "PtcGroups.h"

#include <string.h>

using namespace spsseq;
using namespace sps_host_seq_internal;

namespace {

DeviceIdx perfDeviceIdx(uint8_t device) {
    return device == EXT_DEVICE_GRID_Y ? DeviceIdx::Secondary
                                       : DeviceIdx::None;
}

uint8_t perfDeviceSlot(DeviceIdx idx) {
    return idx == DeviceIdx::Primary ? 0 : 1;
}

void putMask128(uint8_t* dst, const uint64_t* mask) {
    spsSeqPutU64(dst, mask ? mask[0] : 0);
    spsSeqPutU64(dst + 8, mask ? mask[1] : 0);
}

bool selectPerfTrack(uint8_t device, int track) {
#ifdef EXT_TRACKS
    DeviceIdx deviceIdx = perfDeviceIdx(device);
    if (deviceIdx != DeviceIdx::Secondary || !validExtStepTrack(device, track))
        return false;
    SeqPage::select_device_idx(deviceIdx);
    last_ext_track = (uint8_t)track;
    uint8_t dev = perfDeviceSlot(deviceIdx);
    ptc_param_oct.setValue(seq_ptc_page.octs[dev]);
    ptc_param_fine_tune.setValue(seq_ptc_page.fine_tunes[dev]);
    ptc_param_len.setValue(
        SeqTrackUtil::get_ext_step_track((uint8_t)track).length());
    arp_page.track_update((uint8_t)track, false);
    return true;
#else
    (void)device;
    (void)track;
    return false;
#endif
}

uint8_t clampU8(int value, int lo, int hi) {
    if (value < lo)
        return (uint8_t)lo;
    if (value > hi)
        return (uint8_t)hi;
    return (uint8_t)value;
}

DeviceIdx lfoDeviceIdx(uint8_t device) {
    return device == LFO_DEVICE_GRID_Y ? DeviceIdx::Secondary
                                       : DeviceIdx::Primary;
}

LFOSeqTrack* selectedLfoTrack(uint8_t device, int track) {
#ifdef LFO_TRACKS
    DeviceIdx idx = lfoDeviceIdx(device);
    if (track < 0 || track >= LFOTrackRef::track_count(idx))
        return nullptr;
    return &SeqTrackUtil::lfo_track(idx, (uint8_t)track);
#else
    (void)device;
    (void)track;
    return nullptr;
#endif
}

void putFixedLabel(uint8_t* dst, const char* src) {
    for (uint8_t i = 0; i < kLfoLabelBytes; ++i)
        dst[i] = ' ';
    if (src == nullptr)
        return;
    for (uint8_t i = 0; i < kLfoLabelBytes && src[i] != '\0'; ++i)
        dst[i] = (uint8_t)src[i];
}

void putNumericLabel(char* dst, uint8_t len, uint8_t value) {
    if (dst == nullptr || len < 4)
        return;
    dst[0] = (char)('0' + (value / 100) % 10);
    dst[1] = (char)('0' + (value / 10) % 10);
    dst[2] = (char)('0' + value % 10);
    dst[3] = '\0';
}

uint8_t lfoParamEncoderMax(uint8_t dest) {
    uint8_t count = LFOTrackRef::param_count(dest);
    return dest == 0 ? 2 : (count ? count - 1 : 0);
}

void refreshLfoOffsetFromTarget(LFOSeqTrack& track, uint8_t paramIdx) {
    if (paramIdx >= NUM_LFO_PARAMS)
        return;
    uint8_t value = 0;
    if (LFOTrackRef::get_base_param(track.params[paramIdx].dest,
                                    track.params[paramIdx].param, &value)) {
        track.params[paramIdx].offset = value;
    }
}

void finishLfoTrackEdit(LFOSeqTrack& track, bool resetRuntime) {
    track.rearm_oneshot();
    if (resetRuntime) {
        for (uint8_t i = 0; i < NUM_LFO_PARAMS; ++i)
            track.last_wav_value[i] = 255;
        track.reset_runtime();
    }
    if (track.step_count >= track.length)
        track.step_count = 0;
    LFOTrackRef::sync_panel(track);
}

LFOSeqTrackData lfo_bridge_clipboard;
bool lfo_bridge_clipboard_valid = false;
LFOSeqTrackData lfo_bridge_clear_undo;
LFOSeqTrack* lfo_bridge_clear_undo_owner = nullptr;

} // namespace

void SpsHostSeqBridge::sendPerfState(uint8_t tag, uint8_t device,
                                     int track) {
#ifdef EXT_TRACKS
    DeviceIdx deviceIdx = perfDeviceIdx(device);
    if (deviceIdx != DeviceIdx::Secondary || !validExtStepTrack(device, track))
        return;

    uint8_t dev = perfDeviceSlot(deviceIdx);
    ArpSeqTrack& arp = SeqTrackUtil::arp_track(deviceIdx, (uint8_t)track);
    uint8_t body[kPerfStateWireBytes];
    uint16_t off = 0;
    body[off++] = device;
    body[off++] = (uint8_t)track;
    body[off++] = seq_ptc_page.octs[dev];
    body[off++] = seq_ptc_page.fine_tunes[dev];
    body[off++] = (uint8_t)ptc_param_scale.getValue();
    body[off++] =
        SeqTrackUtil::get_ext_step_track((uint8_t)track).length();
    body[off++] = arp.enabled;
    body[off++] = arp.mode;
    body[off++] = arp.length;
    body[off++] = arp.range;
    putMask128(body + off, seq_ptc_page.dev_note_masks[dev]);
    off = (uint16_t)(off + kPtcNoteMaskBytes);
    putMask128(body + off, arp.note_mask);
    off = (uint16_t)(off + kPtcNoteMaskBytes);
    for (uint8_t i = 0; i < kPtcGroupTracks; i++)
        body[off++] = ptc_groups.group_for_track(i);
    body[off++] = SeqTrackUtil::track_count(DeviceIdx::Primary);
    body[off++] = extStepTrackCount();
    sendFrame(CMD_PERF_STATE, tag, body, off);
#else
    (void)tag;
    (void)device;
    (void)track;
#endif
}

bool SpsHostSeqBridge::applySetPtcProp(const uint8_t* b, uint16_t n) {
    if (n < 4 || !selectPerfTrack(b[0], b[1]))
        return false;
    uint8_t dev = perfDeviceSlot(DeviceIdx::Secondary);
    uint8_t value = b[3];
    switch (b[2]) {
    case PTCPROP_OCTAVE:
        value = clampU8(value, 0, 8);
        seq_ptc_page.octs[dev] = value;
        ptc_param_oct.setValue(value);
        break;
    case PTCPROP_DETUNE:
        value = clampU8(value, 0, 64);
        seq_ptc_page.fine_tunes[dev] = value;
        ptc_param_fine_tune.setValue(value);
        break;
    case PTCPROP_SCALE:
        value = clampU8(value, 0, 23);
        ptc_param_scale.setValue(value);
        break;
    case PTCPROP_LENGTH:
        value = clampU8(value, 1, 128);
        SeqTrackUtil::get_ext_step_track(b[1]).set_length(value);
        ptc_param_len.setValue(value);
        break;
    default:
        return false;
    }
    ArpSeqTrack& arp = SeqTrackUtil::arp_track(DeviceIdx::Secondary, b[1]);
    arp.oct = ptc_param_oct.cur;
    arp.fine_tune = ptc_param_fine_tune.cur;
    seq_ptc_page.render_arp(b[2] == PTCPROP_SCALE, DeviceIdx::Secondary,
                            b[1], true);
    return true;
}

bool SpsHostSeqBridge::applySetArpProp(const uint8_t* b, uint16_t n) {
    if (n < 4 || !selectPerfTrack(b[0], b[1]))
        return false;
    ArpSeqTrack& arp = SeqTrackUtil::arp_track(DeviceIdx::Secondary, b[1]);
    uint8_t value = b[3];
    switch (b[2]) {
    case ARPPROP_ENABLED:
        arp.enabled = clampU8(value, 0, 3);
        seq_ptc_page.render_arp(value != ARP_ON, DeviceIdx::Secondary, b[1],
                                true);
        break;
    case ARPPROP_MODE:
        arp.mode = clampU8(value, 0, 18);
        seq_ptc_page.render_arp(!arp.preserves_note_set(),
                                DeviceIdx::Secondary, b[1], true);
        break;
    case ARPPROP_RATE:
        arp.set_length(clampU8(value, 0, 16));
        break;
    case ARPPROP_RANGE:
        arp.range = clampU8(value, 0, 4);
        seq_ptc_page.render_arp(!arp.preserves_note_set(),
                                DeviceIdx::Secondary, b[1], true);
        break;
    default:
        return false;
    }
    arp_page.track_update(b[1], false);
    return true;
}

bool SpsHostSeqBridge::applySetPtcGroup(const uint8_t* b, uint16_t n) {
    if (n < 2 || b[0] >= kPtcGroupTracks)
        return false;
    ptc_groups.set_track_group(b[0], b[1]);
    return true;
}

bool SpsHostSeqBridge::applyPtcNoteEvent(const uint8_t* b, uint16_t n) {
#ifdef EXT_TRACKS
    if (n < 5 || !selectPerfTrack(b[0], b[1]))
        return false;
    uint8_t track = b[1];
    uint8_t note = b[2] & 0x7F;
    uint8_t velocity = b[3] & 0x7F;
    bool pressed = b[4] != 0;
    bool oldScalePadding = seq_ptc_page.scale_padding;
    seq_ptc_page.scale_padding = true;
    uint8_t pitch = seq_ptc_page.process_ext_event(
        note, pressed, (uint8_t)(track & 0x0F), false);
    seq_ptc_page.scale_padding = oldScalePadding;
    seq_ptc_page.config_encoders();
    arp_page.track_update(track, false);
    seq_ptc_page.render_arp(false, DeviceIdx::Secondary, track);
    if (pitch == 255)
        return true;

    ArpSeqTrack& arp = SeqTrackUtil::arp_track(DeviceIdx::Secondary, track);
    if (pressed) {
        if (!arp.enabled || (MidiClock.state != 2))
            seq_ptc_page.note_on_ext(pitch, velocity, track);
    } else if (!arp.enabled) {
        seq_ptc_page.note_off_ext(pitch, velocity, track);
    }
    return true;
#else
    (void)b;
    (void)n;
    return false;
#endif
}

void SpsHostSeqBridge::sendLfoState(uint8_t tag, uint8_t device, int track) {
#ifdef LFO_TRACKS
    LFOSeqTrack* lfo = selectedLfoTrack(device, track);
    if (lfo == nullptr)
        return;

    uint8_t body[kLfoStateWireBytes];
    uint16_t off = 0;
    DeviceIdx idx = lfoDeviceIdx(device);
    body[off++] = device == LFO_DEVICE_GRID_Y ? LFO_DEVICE_GRID_Y
                                              : LFO_DEVICE_GRID_X;
    body[off++] = (uint8_t)track;
    body[off++] = LFOTrackRef::track_count(idx);
    body[off++] = lfo->step_count;
    body[off++] = lfo->enable ? 1 : 0;
    body[off++] = lfo->length ? lfo->length : 16;
    body[off++] = lfo->wav_type;
    body[off++] = lfo->speed;
    body[off++] = lfo->mode;
    body[off++] = LFOTrackRef::target_count();
    spsSeqPutU64(body + off, lfo->pattern_mask);
    off = (uint16_t)(off + 8);

    for (uint8_t i = 0; i < NUM_LFO_PARAMS; ++i) {
        LFOSeqParam& param = lfo->params[i];
        body[off++] = param.dest;
        body[off++] = param.param;
        body[off++] = param.depth;
        body[off++] = param.offset;
        body[off++] = LFOTrackRef::param_count(param.dest);
        body[off++] = lfo->last_wav_value[i];

        char label[5] = {};
        if (param.dest == 0) {
            strcpy(label, "---");
        } else if (!LFOTrackRef::target_label(param.dest, label,
                                              sizeof(label))) {
            putNumericLabel(label, sizeof(label), param.dest);
        }
        putFixedLabel(body + off, label);
        off = (uint16_t)(off + kLfoLabelBytes);

        label[0] = '\0';
        if (param.dest == 0) {
            strcpy(label, param.param > 1 ? "LER" : "---");
        } else if (!LFOTrackRef::param_label(param.dest, param.param, label,
                                             sizeof(label))) {
            putNumericLabel(label, sizeof(label), param.param);
        }
        putFixedLabel(body + off, label);
        off = (uint16_t)(off + kLfoLabelBytes);
    }

    sendFrame(CMD_LFO_STATE, tag, body, off);
#else
    (void)tag;
    (void)device;
    (void)track;
#endif
}

bool SpsHostSeqBridge::applyLfoSetProp(const uint8_t* b, uint16_t n) {
#ifdef LFO_TRACKS
    if (n < 4)
        return false;
    LFOSeqTrack* lfo = selectedLfoTrack(b[0], b[1]);
    if (lfo == nullptr)
        return false;

    uint8_t value = b[3];
    bool resetRuntime = false;
    switch (b[2]) {
    case LFOPROP_ENABLE:
        value = value ? 1 : 0;
        if (!value && lfo->enable)
            lfo->reset_params();
        lfo->enable = value != 0;
        break;
    case LFOPROP_LENGTH:
        lfo->length = clampU8(value, 1, 64);
        break;
    case LFOPROP_MODE:
        lfo->set_mode(clampU8(value, LFO_MODE_FREE, LFO_MODE_TRACK_TRIG));
        break;
    case LFOPROP_WAVE:
        lfo->set_wav_type(clampU8(value, 0, LFO_WAV_COUNT - 1));
        break;
    case LFOPROP_SPEED:
        lfo->set_speed(value & 0x7F);
        break;
    case LFOPROP_MULT:
        lfo->set_speed_multiplier(clampU8(value, 0, LFO_SPEED_MULT_COUNT - 1));
        break;
    case LFOPROP_DEST1:
    case LFOPROP_DEST2: {
        uint8_t paramIdx = b[2] == LFOPROP_DEST1 ? 0 : 1;
        uint8_t maxDest = LFOTrackRef::target_count();
        lfo->params[paramIdx].dest = clampU8(value, 0, maxDest);
        uint8_t maxParam = lfoParamEncoderMax(lfo->params[paramIdx].dest);
        if (lfo->params[paramIdx].param > maxParam)
            lfo->params[paramIdx].param = maxParam;
        refreshLfoOffsetFromTarget(*lfo, paramIdx);
        break;
    }
    case LFOPROP_PARAM1:
    case LFOPROP_PARAM2: {
        uint8_t paramIdx = b[2] == LFOPROP_PARAM1 ? 0 : 1;
        uint8_t maxParam = lfoParamEncoderMax(lfo->params[paramIdx].dest);
        lfo->params[paramIdx].param = clampU8(value, 0, maxParam);
        refreshLfoOffsetFromTarget(*lfo, paramIdx);
        break;
    }
    case LFOPROP_DEPTH1:
        lfo->set_depth(0, value & 0x7F);
        break;
    case LFOPROP_DEPTH2:
        lfo->set_depth(1, value & 0x7F);
        break;
    case LFOPROP_OFFSET1:
    case LFOPROP_OFFSET2: {
        uint8_t paramIdx = b[2] == LFOPROP_OFFSET1 ? 0 : 1;
        value &= 0x7F;
        lfo->params[paramIdx].offset = value;
        if (lfo->params[paramIdx].dest != 0 &&
            LFOTrackRef::set_base_param(lfo->params[paramIdx].dest,
                                        lfo->params[paramIdx].param, value)) {
            lfo->last_wav_value[paramIdx] = value;
        }
        break;
    }
    default:
        return false;
    }
    finishLfoTrackEdit(*lfo, resetRuntime);
    return true;
#else
    (void)b;
    (void)n;
    return false;
#endif
}

bool SpsHostSeqBridge::applyLfoSetMask(const uint8_t* b, uint16_t n) {
#ifdef LFO_TRACKS
    if (n < 4)
        return false;
    LFOSeqTrack* lfo = selectedLfoTrack(b[0], b[1]);
    if (lfo == nullptr)
        return false;
    uint8_t length = lfo->length ? lfo->length : 16;
    uint8_t step = b[2];
    if (step >= length || step >= 64)
        return false;
    uint64_t bit = 1ULL << step;
    if (b[3])
        lfo->pattern_mask |= bit;
    else
        lfo->pattern_mask &= ~bit;
    finishLfoTrackEdit(*lfo, false);
    return true;
#else
    (void)b;
    (void)n;
    return false;
#endif
}

bool SpsHostSeqBridge::applyLfoAction(const uint8_t* b, uint16_t n) {
#ifdef LFO_TRACKS
    if (n < 3)
        return false;
    LFOSeqTrack* lfo = selectedLfoTrack(b[0], b[1]);
    if (lfo == nullptr)
        return false;

    switch (b[2]) {
    case LFO_ACTION_CLEAR:
        if (lfo_bridge_clear_undo_owner == lfo) {
            if (lfo->enable)
                lfo->reset_params();
            lfo->LFOSeqTrackData::operator=(lfo_bridge_clear_undo);
            lfo_bridge_clear_undo_owner = nullptr;
        } else {
            lfo_bridge_clear_undo = *lfo;
            lfo_bridge_clear_undo_owner = lfo;
            if (lfo->enable)
                lfo->reset_params();
            lfo->LFOSeqTrackData::init();
        }
        finishLfoTrackEdit(*lfo, true);
        return true;
    case LFO_ACTION_COPY:
        lfo_bridge_clipboard = *lfo;
        lfo_bridge_clipboard_valid = true;
        return true;
    case LFO_ACTION_PASTE:
        if (!lfo_bridge_clipboard_valid)
            return false;
        if (lfo->enable)
            lfo->reset_params();
        lfo->LFOSeqTrackData::operator=(lfo_bridge_clipboard);
        finishLfoTrackEdit(*lfo, true);
        return true;
    default:
        return false;
    }
#else
    (void)b;
    (void)n;
    return false;
#endif
}

#endif  // !defined(__AVR__)
