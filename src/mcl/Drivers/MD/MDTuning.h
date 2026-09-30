#pragma once
#include <stdint.h>

// Keep tuning in the existing raw model word; stored track sizes do not grow.
enum : uint8_t {
  MD_TUNING_DEFAULT = 0, MD_TUNING_TONAL = 1,
  MD_TUNING_DOWN = 2, MD_TUNING_UP = 3
};
constexpr uint32_t MD_MODEL_TUNING_MASK = 0x000e00ffUL;

inline uint8_t mdTuningFromModel(uint32_t model) {
  if ((model & 0x20000UL) == 0) return MD_TUNING_DEFAULT;
  const uint32_t octave = model & 0xc0000UL;
  return octave == 0x40000UL ? MD_TUNING_DOWN :
         octave == 0x80000UL ? MD_TUNING_UP : MD_TUNING_TONAL;
}

inline uint32_t mdTuningModelFlags(uint8_t tuning) {
  return tuning == MD_TUNING_DEFAULT ? 0UL : 0x20000UL |
      (tuning >= MD_TUNING_DOWN ? uint32_t(tuning - 1u) << 18u : 0UL);
}

// Bit 0: high model; bit 1: tonal; bit 2: route (owned by the caller);
// bit 3: octave down; bit 4: octave up. Every byte remains seven-bit MIDI.
inline uint8_t mdMachineTuningFlags(uint32_t model, bool octaves) {
  const uint8_t tuning = mdTuningFromModel(model);
  return uint8_t((model >> 7u) & 1u) | (tuning ? 2u : 0u) |
      (octaves && tuning >= MD_TUNING_DOWN ? uint8_t((tuning - 1u) << 3u) : 0u);
}

inline uint32_t mdModelFromMachineFlags(uint8_t model, uint8_t flags) {
  const uint8_t tuning = (flags & 2u) == 0 ? MD_TUNING_DEFAULT :
      (flags & 24u) == 8u ? MD_TUNING_DOWN :
      (flags & 24u) == 16u ? MD_TUNING_UP : MD_TUNING_TONAL;
  return uint32_t((model & 127u) | ((flags & 1u) << 7u)) | mdTuningModelFlags(tuning);
}
