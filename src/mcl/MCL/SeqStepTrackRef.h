/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SEQSTEPTRACKREF_H__
#define SEQSTEPTRACKREF_H__

#include "../Drivers/MidiDeviceParam.h"
#include "../Drivers/MidiDevice.h"
#include "DeviceManager.h"
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
  using SeqStepTrackBackend::SeqStepTrackBackend;
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

inline MidiClass *seq_step_tracks_midi() {
  return MD.midi;
}
#else
inline DeviceContext seq_step_track_context() {
  return device_manager.primary_context();
}

inline DeviceStepTrackCapability *seq_step_track_capability() {
  return seq_step_track_context().device()->step_tracks();
}

inline bool seq_step_tracks_available() {
  return seq_step_track_capability()->available(seq_step_track_context());
}

inline SeqStepTrackRef seq_step_track_for(uint8_t track) {
  return seq_step_track_capability()->track(seq_step_track_context(), track);
}

inline SeqStepTrackRef seq_step_active_track() {
  return seq_step_track_capability()->active_track(seq_step_track_context());
}

inline uint8_t seq_step_track_count() {
  return seq_step_track_capability()->track_count(seq_step_track_context());
}

inline bool seq_step_tracks_parse_kit_cc(uint8_t channel, uint8_t cc,
                                         uint8_t *track, uint8_t *param) {
  return seq_step_track_capability()->parse_kit_cc(seq_step_track_context(),
                                                   channel, cc, track, param);
}

inline bool seq_step_tracks_parse_kit_cc_enabled() {
  return seq_step_track_capability()->parses_kit_cc(seq_step_track_context());
}

inline MidiClass *seq_step_tracks_midi() {
  MidiDevice *device = seq_step_track_context().device();
  return device != nullptr ? device->midi : nullptr;
}
#endif

inline bool seq_step_tracks_supports_trig_port(uint8_t port) {
  return device_manager.port_supports(port,
                                      MidiDeviceCapability::MdTrigInterface);
}

inline MidiDevice *seq_step_tracks_device_for_port(uint8_t port) {
  return device_manager.device_for_port(port);
}

inline void seq_step_tracks_trigger(uint8_t port, uint8_t track,
                                    uint8_t velocity) {
  MidiDevice *device = seq_step_tracks_device_for_port(port);
  if (device != nullptr) {
    device->triggerTrack(track, velocity);
  }
}

#endif /* SEQSTEPTRACKREF_H__ */
