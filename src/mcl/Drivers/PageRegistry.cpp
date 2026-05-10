#include "PageRegistry.h"

#include "MCLStrings.h"

namespace PageRegistry {

void clear(PageSelectEntry *entries, uint8_t max_entries) {
  for (uint8_t i = 0; i < max_entries; ++i) {
    entries[i].Name = nullptr;
    entries[i].Page = NULL_PAGE;
    entries[i].PageNumber = i;
    entries[i].CategoryId = i / 4;
    entries[i].IconWidth = 0;
    entries[i].IconHeight = 0;
    entries[i].Icon = PAGE_ICON_NONE;
  }
}

bool add_P(PageSelectEntry *entries, uint8_t max_entries, const char *name_P,
           PageIndex page, uint8_t slot, uint8_t category,
           uint8_t icon_width, uint8_t icon_height, PageSelectIcon icon) {
  if (entries == nullptr || name_P == nullptr || slot >= max_entries) {
    return false;
  }

  PageSelectEntry &entry = entries[slot];
  entry.Name = name_P;
  entry.Page = page;
  entry.PageNumber = slot;
  entry.CategoryId = category;
  entry.IconWidth = icon_width;
  entry.IconHeight = icon_height;
  entry.Icon = icon;
  return true;
}

} // namespace PageRegistry
