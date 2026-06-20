/* Justin Mammarella jmamma@gmail.com 2018 */

#include "Sequencer/SeqExtStepTrackGenericBackend.h"

#ifdef PLATFORM_TBD
#include "../Drivers/Generic/GenericMidiDevice.h"
#include "Midi.h"

MidiClass *SeqExtStepTrackGenericBackend::input_midi() {
  MidiClass *m =
      device_manager.secondary_device()->ext_step_tracks()->input_midi(
          secondary_ctx());
  if (m != nullptr) {
    return m;
  }
  return generic_midi_device.midi;
}

bool SeqExtStepTrackGenericBackend::track_for_channel(uint8_t channel,
                                                       uint8_t *track_index) {
  if (track_index == nullptr) {
    return false;
  }
  return device_manager.secondary_device()->ext_step_tracks()->track_for_channel(
      secondary_ctx(), channel, track_index);
}

bool SeqExtStepTrackGenericBackend::is_mute_cc(uint8_t cc) {
  return device_manager.secondary_device()->ext_step_tracks()->is_mute_cc(
      secondary_ctx(), cc);
}
#endif
