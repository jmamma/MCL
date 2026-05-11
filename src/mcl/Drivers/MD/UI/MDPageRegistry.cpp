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

#if !defined(__AVR__)
const char page_name_delay[] PROGMEM = "DELAY";
const char page_name_reverb[] PROGMEM = "REVERB";
const char page_name_ram1[] PROGMEM = "RAM-1";
const char page_name_ram2[] PROGMEM = "RAM-2";

static uint8_t add_md_page(PageSelectEntry *entries, uint8_t max_entries,
                           const char *name_P, PageIndex page, uint8_t slot,
                           uint8_t category, uint8_t icon_width,
                           uint8_t icon_height, PageSelectIcon icon) {
  return PageRegistry::add_P(entries, max_entries, name_P, page, slot,
                             category, icon_width, icon_height, icon)
             ? 1
             : 0;
}
#endif

} // namespace

#if !defined(__AVR__)
uint8_t MDClass::register_page_select_entries(PageSelectEntry *entries,
                                              uint8_t max_entries) const {
  uint8_t count = 0;
  count += add_md_page(entries, max_entries, mclstr_route, ROUTE_PAGE, 3, 0,
                       24, 14, PAGE_ICON_ROUTE);
  count += add_md_page(entries, max_entries, page_name_delay, FX_PAGE_A, 12, 3,
                       24, 25, PAGE_ICON_RHYTMECHO);
  count += add_md_page(entries, max_entries, page_name_reverb, FX_PAGE_B, 13,
                       3, 24, 25, PAGE_ICON_GATEBOX);
  count += add_md_page(entries, max_entries, page_name_ram1, RAM_PAGE_A, 14, 3,
                       24, 25, PAGE_ICON_RAM1);
  count += add_md_page(entries, max_entries, page_name_ram2, RAM_PAGE_B, 15, 3,
                       24, 25, PAGE_ICON_RAM2);
  return count;
}
#endif

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
