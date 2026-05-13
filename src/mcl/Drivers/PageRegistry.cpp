#include "PageRegistry.h"

#include "MCLStrings.h"

namespace PageRegistry {

static uint16_t pack_icon_meta(uint8_t icon_width, uint8_t icon_height,
                               PageSelectIcon icon) {
  return ((uint16_t)(icon_width & 0x1F) << 9) |
         ((uint16_t)(icon_height & 0x1F) << 4) | (icon & 0x0F);
}

void clear(PageSelectEntry *entries, uint8_t max_entries) {
  for (uint8_t i = 0; i < max_entries; ++i) {
    entries[i].Name = nullptr;
    entries[i].Page = NULL_PAGE;
    entries[i].IconMeta = 0;
  }
}

bool add_P(PageSelectEntry *entries, uint8_t max_entries, const char *name_P,
           PageIndex page, uint8_t slot, uint8_t icon_width,
           uint8_t icon_height, PageSelectIcon icon) {
  if (slot >= max_entries) {
    return false;
  }

  PageSelectEntry &entry = entries[slot];
  entry.Name = name_P;
  entry.Page = page;
  entry.IconMeta = pack_icon_meta(icon_width, icon_height, icon);
  return true;
}

} // namespace PageRegistry
