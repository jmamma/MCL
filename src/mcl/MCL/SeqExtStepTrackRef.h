/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef SEQEXTSTEPTRACKREF_H__
#define SEQEXTSTEPTRACKREF_H__

#include "SeqExtStepTrackApi.h"
#include "SeqPage.h"
#include "SeqPages.h"
#include <stdint.h>

#if defined(__AVR__)
#include "SeqExtStepTrackAvrBackend.h"
using SeqExtStepTrackBackend = SeqExtStepTrackAvrBackend;
#else
#include "SeqExtStepTrackGenericBackend.h"
using SeqExtStepTrackBackend = SeqExtStepTrackGenericBackend;
#endif

class MidiClass;

class SeqExtStepTrackRef : public SeqExtStepTrackBackend {
public:
  using SeqExtStepTrackBackend::track;
  using SeqExtStepTrackBackend::runtime_track;
  using SeqExtStepTrackBackend::track_count;
  using SeqExtStepTrackBackend::set_panel_rec_mode;
  using SeqExtStepTrackBackend::record_note_on;
  using SeqExtStepTrackBackend::record_note_off;
  using SeqExtStepTrackBackend::record_cc_lock;
  using SeqExtStepTrackBackend::record_pitch_bend_lock;
  using SeqExtStepTrackBackend::record_channel_pressure_lock;
#ifdef PLATFORM_TBD
  using SeqExtStepTrackBackend::input_midi;
  using SeqExtStepTrackBackend::track_for_channel;
  using SeqExtStepTrackBackend::is_mute_cc;
#endif

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

  static MidiDevice *mute_mask_device();
  static bool supports_trig_port(uint8_t port);
};

#endif /* SEQEXTSTEPTRACKREF_H__ */
