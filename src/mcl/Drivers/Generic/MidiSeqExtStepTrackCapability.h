#pragma once

#include "platform.h"

#if !defined(__AVR__)

#include "../DeviceCapabilities.h"

class MidiSeqExtStepTrackCapability : public DeviceExtStepTrackCapability {
public:
  explicit MidiSeqExtStepTrackCapability(MidiDevice &device)
      : DeviceExtStepTrackCapability(device) {}

  uint8_t track_count(const DeviceContext &ctx) const override;
  SeqExtStepTrackApi track(const DeviceContext &ctx, uint8_t i) const override;
  bool track_for_channel(const DeviceContext &ctx, uint8_t channel,
                         uint8_t *track_index) const override;
};

#endif // !defined(__AVR__)
