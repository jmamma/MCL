#pragma once

#include "Sequencer/MCLSeq.h"
#include "SeqExtStepTrackApi.h"
#include "SeqTrack.h"
#include <stdint.h>

class MidiUartClass;

class GenericMidiTrackLegacyBackend {
public:
  static uint8_t grid_track_type() { return EXT_TRACK_TYPE; }

  static uint8_t channel(uint8_t track) {
    return mcl_seq.ext_tracks[track].channel;
  }

  static SeqTrack *seq_track(uint8_t track) {
    if (track >= NUM_EXT_TRACKS) {
      return nullptr;
    }
    return &mcl_seq.ext_tracks[track];
  }

  static void init_runtime_track(uint8_t track) {
    (void)track;
  }

  static void clear_mute(uint8_t track) {
    if (track < NUM_EXT_TRACKS) {
      mcl_seq.ext_tracks[track].clear_mute();
    }
  }

  static SeqExtStepTrackApi step_track(uint8_t track) {
    if (track >= NUM_EXT_TRACKS) {
      track = 0;
    }
    return SeqExtStepTrackApi(mcl_seq.ext_tracks[track]);
  }

  static void send_cc(uint8_t track, uint8_t param, uint8_t value,
                      MidiUartClass *uart) {
    if (track < NUM_EXT_TRACKS) {
      mcl_seq.ext_tracks[track].send_cc(param, value, uart);
    }
  }
};
