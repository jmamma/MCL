#pragma once

#if !defined(__AVR__)
#include "GenericMidiTrackModernBackend.h"
using GenericMidiTrackBackend = GenericMidiTrackModernBackend;
#else
#include "GenericMidiTrackLegacyBackend.h"
using GenericMidiTrackBackend = GenericMidiTrackLegacyBackend;
#endif

class GenericMidiTrackRef : public GenericMidiTrackBackend {
public:
  using GenericMidiTrackBackend::channel;
  using GenericMidiTrackBackend::clear_mute;
  using GenericMidiTrackBackend::grid_track_type;
  using GenericMidiTrackBackend::init_runtime_track;
  using GenericMidiTrackBackend::send_cc;
  using GenericMidiTrackBackend::seq_track;
  using GenericMidiTrackBackend::step_track;
};
