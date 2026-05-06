#pragma once

#include <inttypes.h>

enum class MidiDeviceCapability : uint8_t {
  MdTrigInterface,
  MdSequencerTracks,
};
