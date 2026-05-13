/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SEQEXTSTEPTRACKREF_H__
#define SEQEXTSTEPTRACKREF_H__

#include "DeviceManager.h"
#include "SeqExtStepTrackApi.h"
#include "SeqPage.h"
#include "SeqPages.h"
#include "SeqTrackUtil.h"
#include "../Drivers/MidiDevice.h"
#include <stdint.h>

class MidiClass;

class SeqExtStepTrackRef {
public:
  static SeqExtStepTrackApi track(uint8_t track_index) {
#if defined(__AVR__)
    return SeqTrackUtil::get_ext_step_track(track_index);
#else
    return device_manager.secondary_device()->ext_step_tracks()->track(
        device_manager.context_for_device(DeviceIdx::Secondary), track_index);
#endif
  }

  static SeqExtStepTrackApi active_track() {
    return track(last_ext_track);
  }

  static uint8_t active_track_index() {
    return last_ext_track;
  }

  static bool select_track(uint8_t track_index) {
    if (track_index >= track_count()) {
      return false;
    }
    last_ext_track = track_index;
    SeqPage::select_device_idx(DeviceIdx::Secondary);
    return true;
  }

  static uint8_t track_count() {
#if defined(__AVR__)
    return SeqTrackUtil::track_count(false);
#else
    return device_manager.secondary_device()->ext_step_tracks()->track_count(
        device_manager.context_for_device(DeviceIdx::Secondary));
#endif
  }

  static MidiClass *input_midi();
  static MidiDevice *mute_mask_device();
  static bool supports_trig_port(uint8_t port);
  static bool track_for_channel(uint8_t channel, uint8_t *track_index);
  static bool is_mute_cc(uint8_t cc);
  static void set_panel_rec_mode(uint8_t mode);
};

#endif /* SEQEXTSTEPTRACKREF_H__ */
