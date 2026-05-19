#pragma once

#include "platform.h"
#include <stdint.h>

enum SeqTransitionCacheSplay : uint8_t {
  SEQ_TRANSITION_CACHE_NONE = 0,
  SEQ_TRANSITION_CACHE_MD_MACHINE,
  SEQ_TRANSITION_CACHE_MIDI_LINEAR,
};

// Transition cache splay policy:
//   MD/SPSX machine-backed tracks: hardware data is pre-cached, then seq data
//   is loaded four tracks per tick near the transition boundary.
//   MIDI-backed tracks: seq data is loaded one track per tick, with a small
//   fixed lead so track 0 is not loaded on the boundary itself.
class SeqTrackTransition {
public:
  static constexpr uint8_t kMdMachineTracksPerTick = 4;
  static constexpr uint8_t kMidiLeadTicks = 5;

  static ALWAYS_INLINE() uint8_t cache_lead_ticks(
      SeqTransitionCacheSplay splay, uint8_t track_number) {
    switch (splay) {
    case SEQ_TRANSITION_CACHE_MD_MACHINE:
      return track_number / kMdMachineTracksPerTick + 1;
    case SEQ_TRANSITION_CACHE_MIDI_LINEAR:
      return track_number + kMidiLeadTicks;
    case SEQ_TRANSITION_CACHE_NONE:
    default:
      return 0;
    }
  }

  template <typename CountDown>
  static ALWAYS_INLINE() bool cache_due(SeqTransitionCacheSplay splay,
                                        CountDown count_down,
                                        bool cache_loaded,
                                        uint8_t track_number) {
    return !cache_loaded &&
           count_down <= cache_lead_ticks(splay, track_number);
  }
};
