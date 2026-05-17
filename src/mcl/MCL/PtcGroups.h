/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#pragma once

#include "MCLMemory.h"
#include "helpers.h"
#include <string.h>

static constexpr uint8_t PTC_GROUP_OFF = 0;
static constexpr uint8_t PTC_GROUP_LOCAL = 255;
static constexpr uint8_t PTC_MIDI_GROUP_MIN = 1;
static constexpr uint8_t PTC_MIDI_GROUP_MAX = 16;
static constexpr uint8_t PTC_GROUP_TRACKS = 16;

class PtcGroups {
public:
  void clear();
  void load(const uint8_t *src);
  void store(uint8_t *dst) const;
  void load_legacy_poly_mask(uint16_t poly_mask, uint8_t poly_channel);

  uint8_t valid_group_or_off(uint8_t value) const;
  uint8_t group_from_legacy_channel(uint8_t poly_channel) const;
  uint8_t group_for_track(uint8_t track) const;
  uint8_t group_for_midi_channel(uint8_t channel) const;
  void set_track_group(uint8_t track, uint8_t value);

  bool track_has_group(uint8_t track) const;
  uint16_t mask_for_group(uint8_t value) const;
  uint16_t mask_for_track(uint8_t track) const;
  uint16_t mask_for_midi_channel(uint8_t channel) const;
  uint16_t legacy_poly_mask() const;
  uint8_t first_track(uint16_t mask) const;

private:
  uint8_t group[PTC_GROUP_TRACKS];
};

extern PtcGroups ptc_groups;
