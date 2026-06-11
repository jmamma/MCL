#pragma once

#include "../Drivers/MD/MD.h"
#include "Sequencer/MCLSeq.h"
#include "SeqExtStepTrackApi.h"
#include "SeqTrackUtil.h"
#include <stdint.h>

class SeqExtStepTrackAvrBackend {
public:
  static SeqExtStepTrackApi track(uint8_t i) {
    return SeqTrackUtil::get_ext_step_track(i);
  }

  static ExtSeqTrack &runtime_track(uint8_t i) {
    return mcl_seq.ext_tracks[i];
  }

  static uint8_t track_count() {
    return SeqTrackUtil::track_count(false);
  }

  static void set_panel_rec_mode(uint8_t mode) {
    MD.set_rec_mode(mode);
  }

  static void record_note_on(ExtSeqTrack &track, uint8_t note,
                             uint8_t velocity) {
    track.record_track_noteon(note, velocity);
  }

  static void record_note_off(ExtSeqTrack &track, uint8_t note) {
    track.record_track_noteoff(note);
  }

  static void record_cc_lock(ExtSeqTrack &track, uint8_t param, uint8_t value,
                             bool slide) {
    track.record_track_locks(param, value, slide);
  }

  static void record_pitch_bend_lock(ExtSeqTrack &track, uint16_t value14,
                                     bool slide) {
    track.record_track_locks(PARAM_PB, (uint8_t)(value14 >> 7), slide);
  }

  static void record_channel_pressure_lock(ExtSeqTrack &track, uint8_t value) {
    track.record_track_locks(PARAM_CHP, value, false);
  }
};
