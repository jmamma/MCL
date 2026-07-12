#pragma once

#include "GUI/PageSelectTypes.h"
#include <stdint.h>

namespace PageRegistry {

static constexpr uint8_t kMaxPageSlots = 16;
static constexpr uint16_t kIconOffsetMask = 0x03FF;
static constexpr uint8_t kIconHeightShift = 10;

static constexpr uint16_t icon_meta(uint16_t icon_offset,
                                    uint8_t icon_height) {
  return ((uint16_t)(icon_height & 0x1F) << kIconHeightShift) |
         ((icon_offset + 1) & kIconOffsetMask);
}

struct Entry {
  const char *name;
  uint16_t icon_meta;
  uint8_t page;
  uint8_t slot;
};

void clear(PageSelectEntry *entries, uint8_t max_entries);
uint8_t add_entries_P(PageSelectEntry *entries, uint8_t max_entries,
                      const Entry *items, uint8_t item_count);

} // namespace PageRegistry
