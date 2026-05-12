#include "LFOTrackRef.h"

#include "DeviceParamResolver.h"
#include "SeqPage.h"
#include "SeqPages.h"
#include "SeqTrackUtil.h"

#if defined(__AVR__)
#include "../Drivers/MD/MD.h"
#else
#include "DeviceManager.h"
#include "../Drivers/MidiDevice.h"
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
  return lfo_track_for_slot(SeqPage::current_device_slot() == 1);
#else
  return lfo_track_for_slot(true);
#endif
}

bool LFOTrackRef::select_track(uint8_t track) {
  bool primary_tracks = SeqPage::current_device_slot() == 1;
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

uint8_t LFOTrackRef::track_count(uint8_t device_slot) {
#ifdef EXT_TRACKS
  return device_slot == 1 ? NUM_GRID_X_LFO_TRACKS : NUM_GRID_Y_LFO_TRACKS;
#else
  (void)device_slot;
  return NUM_GRID_X_LFO_TRACKS;
#endif
}

uint8_t LFOTrackRef::target_count(uint8_t device_slot) {
  return DeviceParamResolver::slot_target_count(device_slot);
}

uint8_t LFOTrackRef::param_count(uint8_t device_slot, uint8_t dest) {
  return DeviceParamResolver::slot(device_slot, dest).param_count();
}

bool LFOTrackRef::get_param(uint8_t device_slot, uint8_t dest, uint8_t param,
                            uint8_t *value) {
  return DeviceParamResolver::slot(device_slot, dest).get_param(param, value);
}

void LFOTrackRef::sync_panel(const LFOSeqTrack &track) {
#if defined(__AVR__)
  MD.sync_seqtrack(track.length, track.speed, track.step_count);
#else
  device_manager.primary_device()->panel()->sync_seqtrack(
      track.length, track.speed, track.step_count);
#endif
}
