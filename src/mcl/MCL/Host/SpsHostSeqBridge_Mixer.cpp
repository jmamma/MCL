#if !defined(__AVR__)

#include "Host/SpsHostSeqBridge.h"
#include "SpsHostSeqBridge_Internal.h"

#include "Drivers/MD/MDParams.h"
#include "GUI/Pages/CommonPages.h"
#include "GUI/Pages/Performance/MixerPage.h"

using namespace spsseq;
using namespace sps_host_seq_internal;

namespace {

DeviceIdx mixerDeviceIdx(uint8_t device) {
    return device == MIXER_DEVICE_SECONDARY ? DeviceIdx::Secondary
                                            : DeviceIdx::Primary;
}

uint8_t mixerDeviceByte(DeviceIdx idx) {
    return idx == DeviceIdx::Secondary ? MIXER_DEVICE_SECONDARY
                                       : MIXER_DEVICE_PRIMARY;
}

uint16_t trackMaskForLen(uint8_t len) {
    if (len >= kNumTracks)
        return 0xFFFF;
    return (uint16_t)((1u << len) - 1u);
}

uint8_t mixerDeviceSlot(DeviceIdx idx) {
    return idx == DeviceIdx::Secondary ? 1 : 0;
}

uint16_t activeMuteMask(const MixerTarget& target, uint8_t len) {
    uint16_t mask = 0;
    for (uint8_t i = 0; i < len; i++) {
        SeqTrack* seq = target.seq_track(i);
        if (seq != nullptr && seq->mute_state != SEQ_MUTE_OFF)
            mask |= (uint16_t)(1u << i);
    }
    return mask;
}

uint8_t resolveMixerParam(const MixerTarget& target, uint8_t selector) {
    if (selector == kMixerParamSelectorDefault)
        return target.default_param();
    if (selector == kMixerParamVol)
        return target.is_md_device() ? MODEL_VOL : target.default_param();
    if (selector == kMixerParamPan)
        return target.is_md_device() ? MODEL_PAN : 1;
    if (selector < kMixerPerfEncoders)
        return target.param_for_encoder(selector, mixer_page.display_mode,
                                        target.perf_available());
    return selector;
}

uint8_t mixerParamValue(const MixerTarget& target, uint8_t track,
                        uint8_t param) {
    MidiDeviceMixerParam info;
    if (target.param(track, param, &info))
        return target.param_value_7bit(info);
    uint8_t fallback = target.default_param();
    if (param != fallback && target.param(track, fallback, &info))
        return target.param_value_7bit(info);
    return 127;
}

bool mixerParamValueExact(const MixerTarget& target, uint8_t track,
                          uint8_t selector, uint8_t& value) {
    uint8_t param = resolveMixerParam(target, selector);
    MidiDeviceMixerParam info;
    if (!target.param(track, param, &info))
        return false;
    value = target.param_value_7bit(info);
    return true;
}

void markMixerPageDirty(uint8_t device) {
    DeviceIdx idx = mixerDeviceIdx(device);
    if (mixer_page.mixer_device_idx == idx) {
        mixer_page.redraw_mask = 0xFFFF;
        mixer_page.redraw_mutes = true;
    }
}

} // namespace

void SpsHostSeqBridge::sendMixerState(uint8_t tag, uint8_t device) {
    if (device > MIXER_DEVICE_SECONDARY)
        device = mixerDeviceByte(mixer_page.mixer_device_idx);

    DeviceIdx idx = mixerDeviceIdx(device);
    MixerTarget target;
    target.bind(idx);

    uint8_t len = target.track_count();
    if (len > kNumTracks)
        len = kNumTracks;

    uint8_t defaultParam = target.default_param();
    uint8_t displayParam = defaultParam;
    if (mixer_page.mixer_device_idx == idx)
        displayParam = mixer_page.display_mode;

    uint8_t body[kMixerStateWireBytes];
    uint16_t off = 0;
    body[off++] = device;
    body[off++] = len;
    body[off++] = defaultParam;
    body[off++] = displayParam;
    body[off++] = target.perf_available() ? MIXER_FLAG_PERF_AVAILABLE : 0;
    body[off++] = mixer_page.first_track < kNumTracks ? mixer_page.first_track
                                                      : (uint8_t)0xFF;
    putU16le(body + off, activeMuteMask(target, len));
    off = (uint16_t)(off + 2);
    putU16le(body + off, mcl_seq.fill_mask_for(idx));
    off = (uint16_t)(off + 2);

    for (uint8_t i = 0; i < kNumTracks; i++)
        body[off++] = i < len ? mixerParamValue(target, i, defaultParam) : 0;

    uint8_t* levels = idx == DeviceIdx::Secondary ? mixer_page.ext_disp_levels
                                                  : mixer_page.disp_levels;
    for (uint8_t i = 0; i < kNumTracks; i++)
        body[off++] = i < len ? levels[i] : 0;

    uint16_t volMask = 0;
    uint8_t volValues[kNumTracks] = {};
    for (uint8_t i = 0; i < len; i++) {
        uint8_t vol = 127;
        if (mixerParamValueExact(target, i, kMixerParamVol, vol)) {
            volMask |= (uint16_t)(1u << i);
            volValues[i] = vol;
        }
    }
    putU16le(body + off, volMask);
    off = (uint16_t)(off + 2);
    for (uint8_t i = 0; i < kNumTracks; i++)
        body[off++] = volValues[i];

    uint16_t panMask = 0;
    uint8_t panValues[kNumTracks] = {};
    for (uint8_t i = 0; i < len; i++) {
        uint8_t pan = 64;
        if (mixerParamValueExact(target, i, kMixerParamPan, pan)) {
            panMask |= (uint16_t)(1u << i);
            panValues[i] = pan;
        }
    }
    putU16le(body + off, panMask);
    off = (uint16_t)(off + 2);
    for (uint8_t i = 0; i < kNumTracks; i++)
        body[off++] = panValues[i];

    for (uint8_t state = 0; state < kMixerPerfStates; state++) {
        uint16_t primaryMute =
            (uint16_t)~mixer_page.perf_states[state].mute_mask[0];
        uint16_t secondaryMute =
            (uint16_t)~mixer_page.perf_states[state].mute_mask[1];
        putU16le(body + off, primaryMute);
        off = (uint16_t)(off + 2);
        putU16le(body + off, secondaryMute);
        off = (uint16_t)(off + 2);
        putU16le(body + off, mixer_page.perf_states[state].fill_mask[0]);
        off = (uint16_t)(off + 2);
        putU16le(body + off, mixer_page.perf_states[state].fill_mask[1]);
        off = (uint16_t)(off + 2);
        uint8_t loadBits = 0;
        if (mixer_page.load_types[state][0])
            loadBits |= 1;
        if (mixer_page.load_types[state][1])
            loadBits |= 2;
        body[off++] = loadBits;
        for (uint8_t enc = 0; enc < kMixerPerfEncoders; enc++)
            body[off++] = mixer_page.perf_locks[state][enc];
    }

    sendFrame(CMD_MIXER_STATE, tag, body, off);
}

bool SpsHostSeqBridge::applyMixerSetParam(const uint8_t* b, uint16_t n) {
    if (n < 5)
        return false;
    MixerTarget target;
    target.bind(mixerDeviceIdx(b[0]));
    uint8_t len = target.track_count();
    if (len > kNumTracks)
        len = kNumTracks;
    uint8_t param = resolveMixerParam(target, b[1]);
    uint16_t mask = getU16le(b + 2) & trackMaskForLen(len);
    uint8_t value = b[4] & 0x7F;
    bool changed = false;
    for (uint8_t i = 0; i < len; i++) {
        if ((mask & (uint16_t)(1u << i)) == 0)
            continue;
        changed = target.set_param(i, param, value, true) || changed;
    }
    if (changed)
        markMixerPageDirty(b[0]);
    return changed;
}

bool SpsHostSeqBridge::applyMixerAdjustParam(const uint8_t* b, uint16_t n) {
    if (n < 5)
        return false;
    MixerTarget target;
    target.bind(mixerDeviceIdx(b[0]));
    uint8_t len = target.track_count();
    if (len > kNumTracks)
        len = kNumTracks;
    uint8_t param = resolveMixerParam(target, b[1]);
    uint16_t mask = getU16le(b + 2) & trackMaskForLen(len);
    int8_t delta = (int8_t)b[4];
    bool changed = false;
    for (uint8_t i = 0; i < len; i++) {
        if ((mask & (uint16_t)(1u << i)) == 0)
            continue;
        MidiDeviceMixerParam info;
        if (!target.param(i, param, &info))
            continue;
        MidiDeviceMixerValue value =
            target.clamp_param_value(info, (int16_t)info.value + delta);
        changed = target.set_param(i, param, value, true) || changed;
    }
    if (changed)
        markMixerPageDirty(b[0]);
    return changed;
}

bool SpsHostSeqBridge::applyMixerSetMask(const uint8_t* b, uint16_t n) {
    if (n < 6)
        return false;
    uint8_t device = b[0];
    uint8_t kind = b[1];
    uint8_t set = b[2];
    uint8_t slot = b[3];
    uint16_t mask = getU16le(b + 4);
    DeviceIdx idx = mixerDeviceIdx(device);
    MixerTarget target;
    target.bind(idx);
    uint8_t len = target.track_count();
    if (len > kNumTracks)
        len = kNumTracks;
    mask &= trackMaskForLen(len);

    if (kind == MIXER_MASK_ACTIVE_MUTE) {
        for (uint8_t i = 0; i < len; i++) {
            bool mute = (mask & (uint16_t)(1u << i)) != 0;
            if (target.set_seq_mute_state(i, mute))
                target.mute_track(i, mute);
        }
        markMixerPageDirty(device);
        return true;
    }

    if (kind == MIXER_MASK_ACTIVE_FILL) {
        mcl_seq.set_fill_mask(idx, mask);
        markMixerPageDirty(device);
        return true;
    }

    if (set >= kMixerPerfStates || slot > 1)
        return false;

    if (kind == MIXER_MASK_PERF_MUTE) {
        mixer_page.perf_states[set].mute_mask[slot] = (uint16_t)~mask;
        mixer_page.redraw_mutes = true;
        return true;
    }
    if (kind == MIXER_MASK_PERF_FILL) {
        mixer_page.perf_states[set].fill_mask[slot] = mask;
        mixer_page.redraw_mutes = true;
        return true;
    }
    return false;
}

bool SpsHostSeqBridge::applyMixerLoadPerf(const uint8_t* b, uint16_t n) {
    if (n < 3 || b[0] >= kMixerPerfStates)
        return false;
    uint8_t set = b[0];
    mixer_page.load_types[set][0] = (b[2] & 1) != 0;
    mixer_page.load_types[set][1] = (b[2] & 2) != 0;
    mixer_page.load_perf_state = b[1] ? set : 255;
    mixer_page.switch_perf_state(set, b[1] != 0, mixer_page.load_types[set]);
    mixer_page.redraw_mutes = true;
    return true;
}

bool SpsHostSeqBridge::applyMixerSetDisplay(const uint8_t* b, uint16_t n) {
    if (n < 2)
        return false;
    DeviceIdx idx = mixerDeviceIdx(b[0]);
    MixerTarget target;
    target.bind(idx);
    mixer_page.select_mixer_device(idx);
    mixer_page.set_display_mode(resolveMixerParam(target, b[1]));
    return true;
}

bool SpsHostSeqBridge::applyMixerSetPerfLock(const uint8_t* b, uint16_t n) {
    if (n < 3 || b[0] >= kMixerPerfStates || b[1] >= kMixerPerfEncoders)
        return false;
    uint8_t value = b[2];
    mixer_page.perf_locks[b[0]][b[1]] = value < 128 ? value : 0xFF;
    mixer_page.redraw_mutes = true;
    return true;
}

#endif  // !defined(__AVR__)
