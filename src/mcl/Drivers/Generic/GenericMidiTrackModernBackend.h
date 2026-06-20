#pragma once

#if !defined(__AVR__)

#include "Sequencer/MCLSeq.h"
#include "Sequencer/SeqExtStepTrackApi.h"
#include "Sequencer/SeqTrack.h"
#include <stdint.h>

class MidiUartClass;

class GenericMidiTrackModernBackend {
public:
  static uint8_t grid_track_type() { return MIDI_TRACK_TYPE; }

  static uint8_t channel(uint8_t track) {
    return mcl_seq.midi_tracks[track].channel();
  }

  static SeqTrack *seq_track(uint8_t track) {
    if (track >= NUM_EXT_TRACKS) {
      return nullptr;
    }
    return &mcl_seq.midi_tracks[track];
  }

  static void init_runtime_track(uint8_t track) {
    if (track >= NUM_EXT_TRACKS) {
      return;
    }
    mcl_seq.midi_tracks[track].active = MIDI_TRACK_TYPE;
    mcl_seq.midi_tracks[track].seq_data.channel = track;
    mcl_seq.midi_tracks[track].set_channel(track);
  }

  static void clear_mute(uint8_t track) {
    if (track < NUM_EXT_TRACKS) {
      mcl_seq.midi_tracks[track].clear_mute();
    }
  }

  static SeqExtStepTrackApi step_track(uint8_t track) {
    if (track >= NUM_EXT_TRACKS) {
      track = 0;
    }
    return SeqExtStepTrackApi(mcl_seq.midi_tracks[track]);
  }

  static void send_cc(uint8_t track, uint8_t param, uint8_t value,
                      MidiUartClass *uart) {
    if (track < NUM_EXT_TRACKS) {
      mcl_seq.midi_tracks[track].send_cc(param, value, uart);
    }
  }
};

#endif // !defined(__AVR__)
