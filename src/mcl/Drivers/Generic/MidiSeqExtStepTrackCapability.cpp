#include "MidiSeqExtStepTrackCapability.h"

#if !defined(__AVR__)

#include "Sequencer/MCLSeq.h"
#include "SeqExtStepTrackApi.h"

uint8_t MidiSeqExtStepTrackCapability::track_count(
    const DeviceContext &ctx) const {
  (void)ctx;
  return NUM_EXT_TRACKS;
}

SeqExtStepTrackApi MidiSeqExtStepTrackCapability::track(
    const DeviceContext &ctx, uint8_t i) const {
  (void)ctx;
  if (i >= NUM_EXT_TRACKS) {
    i = 0;
  }
  return SeqExtStepTrackApi(mcl_seq.midi_tracks[i]);
}

bool MidiSeqExtStepTrackCapability::track_for_channel(
    const DeviceContext &ctx, uint8_t channel, uint8_t *track_index) const {
  (void)ctx;
  if (track_index == nullptr) {
    return false;
  }
  for (uint8_t i = 0; i < NUM_EXT_TRACKS; i++) {
    if (mcl_seq.midi_tracks[i].channel() == channel) {
      *track_index = i;
      return true;
    }
  }
  *track_index = 255;
  return false;
}

#endif // !defined(__AVR__)
