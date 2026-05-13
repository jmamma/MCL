#pragma once

#include "MenuTypes.h"
#include <stdint.h>

namespace PageRegistry {

static constexpr uint8_t kMaxPageSlots = 16;

void clear(PageSelectEntry *entries, uint8_t max_entries);
bool add_P(PageSelectEntry *entries, uint8_t max_entries, const char *name_P,
           PageIndex page, uint8_t slot, uint8_t icon_width,
           uint8_t icon_height, PageSelectIcon icon);

} // namespace PageRegistry
