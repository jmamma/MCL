#pragma once

#if !defined(__AVR__)

#include "../Drivers/DeviceCapabilities.h"
#include "../Drivers/DeviceContext.h"
#include "../Drivers/MidiDevice.h"
#include "Devices/DeviceManager.h"
#include "Sequencer/SeqExtStepTrackApi.h"
#include <stdint.h>

class MidiClass;

class SeqExtStepTrackGenericBackend {
public:
  static SeqExtStepTrackApi track(uint8_t i) {
    return device_manager.secondary_device()->ext_step_tracks()->track(
        secondary_ctx(), i);
  }

  static SeqExtStepTrackApi runtime_track(uint8_t i) {
    return track(i);
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

  static void record_note_on(SeqExtStepTrackApi &track, uint8_t note,
                             uint8_t velocity) {
    track.record_note_on(note, velocity);
  }

  static void record_note_off(SeqExtStepTrackApi &track, uint8_t note) {
    track.record_note_off(note);
  }

  static void record_cc_lock(SeqExtStepTrackApi &track, uint8_t param,
                             uint8_t value, bool slide) {
    track.record_cc_lock(param, value, slide);
  }

  static void record_pitch_bend_lock(SeqExtStepTrackApi &track,
                                     uint16_t value14, bool slide) {
    track.record_pitch_bend_lock(value14, slide);
  }

  static void record_channel_pressure_lock(SeqExtStepTrackApi &track,
                                           uint8_t value) {
    track.record_channel_pressure_lock(value);
  }

protected:
  static DeviceContext secondary_ctx() {
    return device_manager.context_for_device(DeviceIdx::Secondary);
  }
};

#endif // !defined(__AVR__)
