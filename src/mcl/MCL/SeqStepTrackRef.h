/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SEQSTEPTRACKREF_H__
#define SEQSTEPTRACKREF_H__

#include "../Drivers/MidiDeviceParam.h"
#include "MCLSeq.h"
#include <stdint.h>

extern uint8_t last_md_track;

using SeqStepLockParamInfo = MidiDeviceParamInfo;

#if defined(__AVR__)
#include "MCLEncoder.h"
extern MCLEncoder ptc_param_fine_tune;
void seq_step_set_md_linked_param_update(bool enabled);
#include "SeqStepTrackMdBackend.h"
using SeqStepTrackBackend = SeqStepTrackMdBackend;
#else
#include "SeqStepTrackGenericBackend.h"
using SeqStepTrackBackend = SeqStepTrackGenericBackend;
#endif

class SeqStepTrackRef : public SeqStepTrackBackend {
public:
  explicit SeqStepTrackRef(MDSeqTrack &track, uint8_t device_slot = 1)
      : SeqStepTrackBackend(track, device_slot) {}

#if !defined(__AVR__)
  explicit SeqStepTrackRef(StepSeqDataTrack &track, uint8_t device_slot = 1)
      : SeqStepTrackBackend(track, device_slot) {}
#endif
};

#if defined(__AVR__)
inline bool seq_step_tracks_available() { return true; }

inline SeqStepTrackRef seq_step_track_for(uint8_t track) {
  if (track >= mcl_seq.num_md_tracks) {
    track = 0;
  }
  return SeqStepTrackRef(mcl_seq.md_tracks[track]);
}

inline SeqStepTrackRef seq_step_active_track() {
  return seq_step_track_for(last_md_track);
}

inline uint8_t seq_step_track_count() { return mcl_seq.num_md_tracks; }

inline bool seq_step_tracks_parse_kit_cc(uint8_t channel, uint8_t cc,
                                         uint8_t *track, uint8_t *param) {
  if (track == nullptr || param == nullptr) {
    return false;
  }
  MD.parseCC(channel, cc, track, param);
  return *track != 255;
}

inline bool seq_step_tracks_parse_kit_cc_enabled() { return true; }
#else
inline DeviceStepTrackCapability *seq_step_track_capability() {
  return DeviceParamResolver::slot_device(1)->step_tracks();
}

inline uint8_t seq_step_track_device_idx() {
  return DeviceParamResolver::slot_device_idx(1);
}

inline bool seq_step_tracks_available() {
  return seq_step_track_capability()->available(seq_step_track_device_idx());
}

inline SeqStepTrackRef seq_step_track_for(uint8_t track) {
  return seq_step_track_capability()->track(seq_step_track_device_idx(), track);
}

inline SeqStepTrackRef seq_step_active_track() {
  return seq_step_track_capability()->active_track(seq_step_track_device_idx());
}

inline uint8_t seq_step_track_count() {
  return seq_step_track_capability()->track_count(seq_step_track_device_idx());
}

inline bool seq_step_tracks_parse_kit_cc(uint8_t channel, uint8_t cc,
                                         uint8_t *track, uint8_t *param) {
  return seq_step_track_capability()->parse_kit_cc(seq_step_track_device_idx(),
                                                   channel, cc, track, param);
}

inline bool seq_step_tracks_parse_kit_cc_enabled() {
  return seq_step_track_capability()->parses_kit_cc(
      seq_step_track_device_idx());
}
#endif

#endif /* SEQSTEPTRACKREF_H__ */
