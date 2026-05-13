#include "PageRegistry.h"

#include "MCLStrings.h"

namespace PageRegistry {

void clear(PageSelectEntry *entries, uint8_t max_entries) {
  for (uint8_t i = 0; i < max_entries; ++i) {
    entries[i].Name = nullptr;
    entries[i].Page = NULL_PAGE;
    entries[i].IconMeta = 0;
  }
}

uint8_t add_entries_P(PageSelectEntry *entries, uint8_t max_entries,
                      const Entry *items, uint8_t item_count) {
  uint8_t count = 0;
  for (uint8_t i = 0; i < item_count; i++) {
    const Entry *item = &items[i];
    const char *name = (const char *)pgm_read_ptr(&item->name);
    uint16_t icon_meta = pgm_read_word(&item->icon_meta);
    PageIndex page = (PageIndex)pgm_read_byte(&item->page);
    uint8_t slot = pgm_read_byte(&item->slot);
    if (slot < max_entries) {
      PageSelectEntry &entry = entries[slot];
      entry.Name = name;
      entry.Page = page;
      entry.IconMeta = icon_meta;
      count++;
    }
  }
  return count;
}

} // namespace PageRegistry
