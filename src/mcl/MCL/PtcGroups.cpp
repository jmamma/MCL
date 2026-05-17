#include "PtcGroups.h"

void PtcGroups::clear() {
  for (uint8_t i = 0; i < PTC_GROUP_TRACKS; ++i) {
    group[i] = PTC_GROUP_OFF;
  }
}

void PtcGroups::load(const uint8_t *src) {
  if (src == nullptr) {
    clear();
    return;
  }
  for (uint8_t i = 0; i < PTC_GROUP_TRACKS; ++i) {
    group[i] = valid_group_or_off(src[i]);
  }
}

void PtcGroups::store(uint8_t *dst) const {
  if (dst == nullptr) {
    return;
  }
  memcpy(dst, group, PTC_GROUP_TRACKS);
}

uint8_t PtcGroups::valid_group_or_off(uint8_t value) const {
  if (value >= PTC_MIDI_GROUP_MIN && value <= PTC_MIDI_GROUP_MAX) {
    return value;
  }
  if (value == PTC_GROUP_LOCAL) {
    return value;
  }
  return PTC_GROUP_OFF;
}

uint8_t PtcGroups::group_from_legacy_channel(uint8_t poly_channel) const {
  if (poly_channel >= PTC_MIDI_GROUP_MIN &&
      poly_channel <= PTC_MIDI_GROUP_MAX) {
    return poly_channel;
  }
  return PTC_GROUP_LOCAL;
}

void PtcGroups::load_legacy_poly_mask(uint16_t poly_mask,
                                      uint8_t poly_channel) {
  uint8_t value = group_from_legacy_channel(poly_channel);
  for (uint8_t i = 0; i < PTC_GROUP_TRACKS; ++i) {
    group[i] = IS_BIT_SET16(poly_mask, i) ? value : PTC_GROUP_OFF;
  }
}

uint8_t PtcGroups::group_for_track(uint8_t track) const {
  if (track >= PTC_GROUP_TRACKS) {
    return PTC_GROUP_OFF;
  }
  return group[track];
}

uint8_t PtcGroups::group_for_midi_channel(uint8_t channel) const {
  if (channel >= PTC_GROUP_TRACKS) {
    return PTC_GROUP_OFF;
  }
  uint8_t value = channel + 1;
  return mask_for_group(value) ? value : PTC_GROUP_OFF;
}

void PtcGroups::set_track_group(uint8_t track, uint8_t value) {
  if (track < PTC_GROUP_TRACKS) {
    group[track] = valid_group_or_off(value);
  }
}

bool PtcGroups::track_has_group(uint8_t track) const {
  return group_for_track(track) != PTC_GROUP_OFF;
}

uint16_t PtcGroups::mask_for_group(uint8_t value) const {
  if (value == PTC_GROUP_OFF) {
    return 0;
  }

  uint16_t mask = 0;
  for (uint8_t i = 0; i < PTC_GROUP_TRACKS; ++i) {
    if (group[i] == value) {
      SET_BIT16(mask, i);
    }
  }
  return mask;
}

uint16_t PtcGroups::mask_for_track(uint8_t track) const {
  return mask_for_group(group_for_track(track));
}

uint16_t PtcGroups::mask_for_midi_channel(uint8_t channel) const {
  return mask_for_group(group_for_midi_channel(channel));
}

uint16_t PtcGroups::legacy_poly_mask() const {
  uint16_t mask = 0;
  for (uint8_t i = 0; i < PTC_GROUP_TRACKS; ++i) {
    if (track_has_group(i)) {
      SET_BIT16(mask, i);
    }
  }
  return mask;
}

uint8_t PtcGroups::first_track(uint16_t mask) const {
  for (uint8_t i = 0; i < PTC_GROUP_TRACKS; ++i) {
    if (IS_BIT_SET16(mask, i)) {
      return i;
    }
  }
  return 255;
}

PtcGroups ptc_groups;
