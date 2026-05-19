#include "../MD.h"

#include "MCLStrings.h"
#include "MCLSeq.h"
#include "MidiClock.h"
#include "PageRegistry.h"
#include "ResourceManager.h"
#include <stddef.h>

namespace {

class MDPageSelectKitCallback : public SysexCallback {
public:
  bool state;

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

#define MD_PAGE_ENTRY(name, icon, page, slot, height) \
  {name, PageRegistry::icon_meta(offsetof(__T_icons_page, icon), height), page, slot}

const PageRegistry::Entry md_page_entries[] PROGMEM = {
    MD_PAGE_ENTRY(mclstr_route, icon_route, ROUTE_PAGE, 3, 14),
    MD_PAGE_ENTRY(page_name_delay, icon_rhytmecho, FX_PAGE_A, 12, 25),
    MD_PAGE_ENTRY(page_name_reverb, icon_gatebox, FX_PAGE_B, 13, 25),
    MD_PAGE_ENTRY(page_name_ram1, icon_ram1, RAM_PAGE_A, 14, 25),
    MD_PAGE_ENTRY(page_name_ram2, icon_ram2, RAM_PAGE_B, 15, 25),
};

#undef MD_PAGE_ENTRY

} // namespace

uint8_t MDClass::register_page_select_entries(PageSelectEntry *entries,
                                              uint8_t max_entries) const {
  return PageRegistry::add_entries_P(
      entries, max_entries, md_page_entries,
      sizeof(md_page_entries) / sizeof(md_page_entries[0]));
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
