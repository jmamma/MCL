#include "LFOTrackRef.h"

#include "DeviceParamResolver.h"
#include "DeviceManager.h"
#include "SeqPage.h"
#include "SeqPages.h"
#include "SeqTrackUtil.h"
#include "../Drivers/MidiDevice.h"

#if defined(__AVR__)
#include "../Drivers/MD/MD.h"
#endif

namespace {

LFOSeqTrack &lfo_track_for_slot(bool primary_tracks) {
#ifdef EXT_TRACKS
  if (!primary_tracks) {
    return SeqTrackUtil::get_lfo_track(false, last_ext_track);
  }
#else
  (void)primary_tracks;
#endif
  return SeqTrackUtil::get_lfo_track(true, last_md_track);
}

} // namespace

LFOSeqTrack &LFOTrackRef::current_track() {
#ifdef EXT_TRACKS
  return lfo_track_for_slot(SeqPage::current_device_idx() == 1);
#else
  return lfo_track_for_slot(true);
#endif
}

bool LFOTrackRef::select_track(uint8_t track) {
  bool primary_tracks = SeqPage::current_device_idx() == 1;
#ifndef EXT_TRACKS
  primary_tracks = true;
#endif
  uint8_t count = primary_tracks ? NUM_GRID_X_LFO_TRACKS
                                 : NUM_GRID_Y_LFO_TRACKS;
  if (track >= count) {
    return false;
  }
  if (primary_tracks) {
    last_md_track = track;
  } else {
#ifdef EXT_TRACKS
    last_ext_track = track;
#endif
  }
  return true;
}

uint8_t LFOTrackRef::track_count(uint8_t device_idx) {
#ifdef EXT_TRACKS
  return device_idx == 0 ? NUM_GRID_X_LFO_TRACKS : NUM_GRID_Y_LFO_TRACKS;
#else
  (void)device_idx;
  return NUM_GRID_X_LFO_TRACKS;
#endif
}

uint8_t LFOTrackRef::target_count(uint8_t device_idx) {
  return DeviceParamResolver::target_count_for_idx(device_idx);
}

uint8_t LFOTrackRef::param_count(uint8_t device_idx, uint8_t dest) {
  return DeviceParamResolver::target_for_idx(device_idx, dest).param_count();
}

bool LFOTrackRef::get_param(uint8_t device_idx, uint8_t dest, uint8_t param,
                            uint8_t *value) {
  return DeviceParamResolver::target_for_idx(device_idx, dest).get_param(param, value);
}

void LFOTrackRef::set_key_repeat(uint8_t enabled) {
#if defined(__AVR__)
  MD.set_key_repeat(enabled);
#else
  device_manager.primary_device()->panel()->set_key_repeat(enabled);
#endif
}

void LFOTrackRef::sync_panel(const LFOSeqTrack &track) {
#if defined(__AVR__)
  MD.sync_seqtrack(track.length, track.speed, track.step_count);
#else
  device_manager.primary_device()->panel()->sync_seqtrack(
      track.length, track.speed, track.step_count);
#endif
}

bool LFOTrackRef::supports_trig_port(uint8_t port) {
  return device_manager.port_supports(port,
                                      MidiDeviceCapability::MdTrigInterface);
}
