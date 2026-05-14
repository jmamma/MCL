#pragma once

#if !defined(__AVR__)

#include "../Drivers/DeviceCapabilities.h"
#include "../Drivers/DeviceContext.h"
#include "../Drivers/MidiDevice.h"
#include "DeviceManager.h"
#include "SeqExtStepTrackApi.h"
#include <stdint.h>

class MidiClass;

class SeqExtStepTrackGenericBackend {
public:
  static SeqExtStepTrackApi track(uint8_t i) {
    return device_manager.secondary_device()->ext_step_tracks()->track(
        secondary_ctx(), i);
  }

  static uint8_t track_count() {
    return device_manager.secondary_device()->ext_step_tracks()->track_count(
        secondary_ctx());
  }

  static void set_panel_rec_mode(uint8_t mode) {
    device_manager.primary_device()->panel()->set_rec_mode(mode);
  }

#ifdef PLATFORM_TBD
  static MidiClass *input_midi();
  static bool track_for_channel(uint8_t channel, uint8_t *track_index);
  static bool is_mute_cc(uint8_t cc);
#endif

protected:
  static DeviceContext secondary_ctx() {
    return device_manager.context_for_device(DeviceIdx::Secondary);
  }
};

#endif // !defined(__AVR__)
