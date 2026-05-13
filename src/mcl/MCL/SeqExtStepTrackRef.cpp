/* Justin Mammarella jmamma@gmail.com 2018 */

#include "SeqExtStepTrackRef.h"

#include "DeviceManager.h"
#include "MCLSeq.h"
#include "SeqPage.h"
#include "../Drivers/MidiDevice.h"

#if defined(__AVR__)
#include "../Drivers/MD/MD.h"
#endif

#ifdef PLATFORM_TBD
#include "../Drivers/Generic/GenericMidiDevice.h"
#endif

#if !defined(__AVR__)
namespace {
DeviceContext secondary_ctx() {
  return device_manager.context_for_device(DeviceIdx::Secondary);
}
} // namespace
#endif

MidiClass *SeqExtStepTrackRef::input_midi() {
#ifdef PLATFORM_TBD
  MidiClass *m =
      device_manager.secondary_device()->ext_step_tracks()->input_midi(
          secondary_ctx());
  if (m != nullptr) {
    return m;
  }
  return generic_midi_device.midi;
#else
  return nullptr;
#endif
}

MidiDevice *SeqExtStepTrackRef::mute_mask_device() {
  return SeqPage::device_for_seq_idx(DeviceIdx::Secondary);
}

bool SeqExtStepTrackRef::supports_trig_port(uint8_t port) {
  return device_manager.port_supports(port,
                                      MidiDeviceCapability::MdTrigInterface);
}

bool SeqExtStepTrackRef::track_for_channel(uint8_t channel,
                                           uint8_t *track_index) {
  if (track_index == nullptr) {
    return false;
  }
#if defined(__AVR__)
  *track_index = mcl_seq.find_ext_track(channel);
  return *track_index != 255;
#else
  return device_manager.secondary_device()->ext_step_tracks()->track_for_channel(
      secondary_ctx(), channel, track_index);
#endif
}

bool SeqExtStepTrackRef::is_mute_cc(uint8_t cc) {
#if defined(__AVR__)
  MidiDevice *device = device_manager.secondary_device();
  return device != nullptr && cc == device->get_mute_cc();
#else
  return device_manager.secondary_device()->ext_step_tracks()->is_mute_cc(
      secondary_ctx(), cc);
#endif
}

void SeqExtStepTrackRef::set_panel_rec_mode(uint8_t mode) {
#if defined(__AVR__)
  MD.set_rec_mode(mode);
#else
  device_manager.primary_device()->panel()->set_rec_mode(mode);
#endif
}
