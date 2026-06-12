#if !defined(__AVR__)

#include "Host/SpsHostSeqBridge.h"
#include "SpsHostSeqBridge_Internal.h"

#include "GUI/Pages/Sequencer/SeqPages.h"
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

#endif  // !defined(__AVR__)
