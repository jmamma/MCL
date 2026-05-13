#include "../MD.h"

#include "MCLStrings.h"
#include "MCLSeq.h"
#include "MidiClock.h"
#include "PageRegistry.h"

namespace {

class MDPageSelectKitCallback : public SysexCallback {
public:
  bool state = false;

  void init() { state = true; }

  void onReceived() {
    if (MD.kit.fromSysex(MD.midi)) {
      mcl_seq.update_kit_params();
    }
    auto listener = MD.getSysexListener();
    listener->removeOnMessageCallback(this);
    state = false;
  }
};

MDPageSelectKitCallback md_page_select_kit_cb;

const char page_name_delay[] PROGMEM = "DELAY";
const char page_name_reverb[] PROGMEM = "REVERB";
const char page_name_ram1[] PROGMEM = "RAM-1";
const char page_name_ram2[] PROGMEM = "RAM-2";

struct MDPageEntry {
  const char *name;
  uint8_t page;
  uint8_t slot;
  uint8_t icon_height;
  uint8_t icon;
};

const MDPageEntry md_page_entries[] PROGMEM = {
    {mclstr_route, ROUTE_PAGE, 3, 14, PAGE_ICON_ROUTE},
    {page_name_delay, FX_PAGE_A, 12, 25, PAGE_ICON_RHYTMECHO},
    {page_name_reverb, FX_PAGE_B, 13, 25, PAGE_ICON_GATEBOX},
    {page_name_ram1, RAM_PAGE_A, 14, 25, PAGE_ICON_RAM1},
    {page_name_ram2, RAM_PAGE_B, 15, 25, PAGE_ICON_RAM2},
};

} // namespace

uint8_t MDClass::register_page_select_entries(PageSelectEntry *entries,
                                              uint8_t max_entries) const {
  uint8_t count = 0;
  for (uint8_t i = 0; i < sizeof(md_page_entries) / sizeof(md_page_entries[0]);
       i++) {
    const MDPageEntry *entry = &md_page_entries[i];
    const char *name = (const char *)pgm_read_ptr(&entry->name);
    PageIndex page = (PageIndex)pgm_read_byte(&entry->page);
    uint8_t slot = pgm_read_byte(&entry->slot);
    uint8_t icon_height = pgm_read_byte(&entry->icon_height);
    PageSelectIcon icon = (PageSelectIcon)pgm_read_byte(&entry->icon);
    if (PageRegistry::add_P(entries, max_entries, name, page, slot, 24,
                            icon_height, icon)) {
      count++;
    }
  }
  return count;
}

void MDClass::page_select_prepare() {
  if (md_page_select_kit_cb.state) {
    return;
  }
  if (MidiClock.state == 2) {
    return;
  }

  md_page_select_kit_cb.init();
  auto listener = getSysexListener();
  listener->addOnMessageCallback(
      &md_page_select_kit_cb,
      (sysex_callback_ptr_t)&MDPageSelectKitCallback::onReceived);
  requestKit(0x7F);
}

void MDClass::page_select_popup(char *text) {
  if (text != nullptr) {
    popup_text(text, true);
  }
}

void MDClass::page_select_cleanup() {
  set_trigleds(0, TRIGLED_OVERLAY);
  popup_text(127, 2);
}
