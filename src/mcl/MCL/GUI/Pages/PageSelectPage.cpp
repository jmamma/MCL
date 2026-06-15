#include "GUI/Pages/PageSelectPage.h"
#include "ResourceManager.h"
#include "MCLDefines.h"
#include "MCLGUI.h"
#include "DeviceManager.h"
#include "MCLStrings.h"
#include "../../../Drivers/MidiDevice.h"
#include "../../../Drivers/PageRegistry.h"
#include <stddef.h>

constexpr uint8_t n_category = 4;

namespace {

const char page_name_perf[] PROGMEM = "PERF";
const char page_name_step_edit[] PROGMEM = "STEP EDIT";
const char page_name_lfo[] PROGMEM = "LFO";
const char page_name_piano_roll[] PROGMEM = "PIANO ROLL";
const char page_name_chromatic[] PROGMEM = "CHROMATIC";
const char page_name_sample_manager[] PROGMEM = "SAMPLE MANAGER";
const char page_name_wav_designer[] PROGMEM = "WAV DESIGNER";

const char page_category_main[] PROGMEM = "MAIN";
const char page_category_seq[] PROGMEM = "SEQ ";
const char page_category_snd[] PROGMEM = "SND ";
const char page_category_aux[] PROGMEM = "AUX ";
const char *const page_category_names[] PROGMEM = {
    page_category_main,
    page_category_seq,
    page_category_snd,
    page_category_aux,
};
const uint8_t page_category_label_x[] PROGMEM = {30, 57, 81, 104};

constexpr uint8_t kPageIconWidth = 24;

#define PAGE_ENTRY(name, icon, page, slot, height) \
  {name, PageRegistry::icon_meta(offsetof(__T_icons_page, icon), height), page, slot}

const PageRegistry::Entry common_page_entries[] PROGMEM = {
    PAGE_ENTRY(mclstr_grid, icon_grid, GRID_PAGE, 0, 15),
    PAGE_ENTRY(mclstr_mixer, icon_mixer, MIXER_PAGE, 1, 16),
    PAGE_ENTRY(page_name_perf, icon_perf, PERF_PAGE_0, 2, 18),
    PAGE_ENTRY(page_name_step_edit, icon_step, SEQ_STEP_PAGE, 4, 21),
    PAGE_ENTRY(page_name_lfo, icon_lfo, LFO_PAGE, 5, 24),
    PAGE_ENTRY(page_name_piano_roll, icon_pianoroll, SEQ_EXTSTEP_PAGE, 6, 25),
    PAGE_ENTRY(page_name_chromatic, icon_chroma, SEQ_PTC_PAGE, 7, 25),
#ifdef SOUND_PAGE
    PAGE_ENTRY(page_name_sample_manager, icon_sample, SAMPLE_BROWSER, 8, 25),
#endif
#ifdef WAV_DESIGNER
    PAGE_ENTRY(page_name_wav_designer, icon_wavd, WD_PAGE_0, 9, 19),
#endif
};

#undef PAGE_ENTRY

static uint8_t category_for_slot(uint8_t slot) {
  return slot >> 2;
}

static uint8_t category_first_page(uint8_t cat) { return cat * 4; }

static uint8_t category_page_count(uint8_t cat) {
  return cat == 2 ? 3 : 4;
}

static MidiDevice *nonnull_device(MidiDevice *device) {
  return (device == nullptr || device == &null_midi_device) ? nullptr : device;
}

uint8_t next_page(const PageSelectPage *page, int8_t step) NOINLINE();
uint8_t next_page(const PageSelectPage *page, int8_t step) {
  int8_t slot = (int8_t)page->page_select + step;
  while (slot >= 0 && slot < (int8_t)PageRegistry::kMaxPageSlots) {
    if (page->get_page((uint8_t)slot, nullptr) != NULL_PAGE) {
      return (uint8_t)slot;
    }
    slot += step;
  }
  return page->page_select;
}

uint8_t next_category_page(const PageSelectPage *page, int8_t step) NOINLINE();
uint8_t next_category_page(const PageSelectPage *page, int8_t step) {
  int8_t cat = (int8_t)category_for_slot(page->page_select) + step;
  while (cat >= 0 && cat < (int8_t)n_category) {
    uint8_t slot = page->category_page_in((uint8_t)cat, 0);
    if (slot != page->page_select) {
      return slot;
    }
    cat += step;
  }
  return page->page_select;
}

} // namespace

PageIndex PageSelectPage::get_page(uint8_t page_number, char *str) const {
  if (page_number < PageRegistry::kMaxPageSlots &&
      page_entries[page_number].Page != NULL_PAGE) {
    if (str) {
      strncpy_P(str, page_entries[page_number].Name, 16);
      str[15] = '\0';
    }
    return page_entries[page_number].Page;
  } else {
    if (str) {
      strcpy_P(str, mclstr_four_dashes);
    }
    return NULL_PAGE;
  }
}

static void print_category_name_by_idx(uint8_t catidx) {
  mcl_print_P((const char *)pgm_read_ptr(&page_category_names[catidx]));
}

void PageSelectPage::init() {
  DEBUG_PRINTLN("page select init");
  key_interface.on();
  uint8_t _midi_lock_tmp = MidiUartParent::handle_midi_lock;
  MidiUartParent::handle_midi_lock = 0;
  R.Clear();
  R.use_icons_page();
  rebuild_entries();

  // clear trigled so it's always sent on first run
  trigled_mask = 0;
  draw_popup();
  MidiUartParent::handle_midi_lock = 1;
  md_prepare();
  MidiUartParent::handle_midi_lock = _midi_lock_tmp;
}

void PageSelectPage::draw_popup() {
  MidiDevice *device = nonnull_device(page_select_ui_device);
  if (device != nullptr) {
    char str[16];
    get_page(page_select, str);
    device->page_select_popup(str);
  }
}

void PageSelectPage::md_prepare() {
  MidiDevice *device = nonnull_device(page_select_ui_device);
  if (device == nullptr) {
    device = nonnull_device(device_manager.primary_device());
  }
  if (device != nullptr) {
    device->page_select_prepare();
  }
}

void PageSelectPage::cleanup() {
  note_interface.init_notes();
  MidiDevice *device = nonnull_device(page_select_ui_device);
  if (device != nullptr) {
    device->page_select_cleanup();
  }
  mcl_gui.reset_trigleds();
}

uint8_t PageSelectPage::category_page_in(uint8_t cat_id,
                                         uint8_t offset) const {
  uint8_t cat_start = category_first_page(cat_id);
  uint8_t cat_size = category_page_count(cat_id);
  for (uint8_t i = 0; i < cat_size; i++) {
    uint8_t slot = cat_start + i;
    if (slot < PageRegistry::kMaxPageSlots &&
        page_entries[slot].Page != NULL_PAGE) {
      if (offset == 0) {
        return slot;
      }
      offset--;
    }
  }
  return page_select;
}

uint8_t PageSelectPage::get_category_page(uint8_t offset) {
  auto cat_id = category_for_slot(page_select);
  return category_page_in(cat_id, offset);
}

void PageSelectPage::rebuild_entries() {
  PageRegistry::clear(page_entries, PageRegistry::kMaxPageSlots);
  page_select_ui_device = nullptr;

  PageRegistry::add_entries_P(
      page_entries, PageRegistry::kMaxPageSlots, common_page_entries,
      sizeof(common_page_entries) / sizeof(common_page_entries[0]));

  MidiDevice *primary = nonnull_device(device_manager.primary_device());
  MidiDevice *secondary = nonnull_device(device_manager.secondary_device());
  if (primary != nullptr) {
    if (primary->register_page_select_entries(
            page_entries, PageRegistry::kMaxPageSlots) > 0) {
      page_select_ui_device = primary;
    }
  }
  if (secondary != nullptr && secondary != primary) {
    if (secondary->register_page_select_entries(
            page_entries, PageRegistry::kMaxPageSlots) > 0 &&
        page_select_ui_device == nullptr) {
      page_select_ui_device = secondary;
    }
  }
}

void PageSelectPage::close_to_selection() {
  PageIndex p = get_page(page_select, nullptr);
  if (BUTTON_DOWN(Buttons.BUTTON1) || (p == NULL_PAGE)) {
    GUI.ignoreNextEvent(Buttons.BUTTON1);
    mcl.setPage(GRID_PAGE);
  } else {
    mcl.setPage(p);
  }
}

void PageSelectPage::loop() {
  uint8_t last_page_select = page_select;
  auto enc_ = (MCLEncoder *)encoders[0];
  int8_t diff = enc_->cur;
  if (diff != 0) {
    page_select = next_page(this, diff > 0 ? 1 : -1);
  }

  enc_ = (MCLEncoder *)encoders[1];
  diff = enc_->cur;
  if (diff != 0) {
    page_select = next_category_page(this, diff > 0 ? 1 : -1);
  }

  if (last_page_select != page_select) {
    draw_popup();
  }
}

void PageSelectPage::display() {
  uint8_t *icon;
  uint8_t iconh;
  uint8_t catidx;

  oled_display.fillRect(0, 0, 128, 7, WHITE);
  oled_display.setFont(&TomThumb);
  oled_display.setTextColor(BLACK);
  oled_display.setCursor(47, 6);
  mcl_print_P(mclstr_page_select);
  oled_display.fillRect(0, 7, 128, 25, BLACK);
  oled_display.setTextColor(WHITE);
  for (uint8_t i = 0; i < 4; ++i) {
    oled_display.setCursor(pgm_read_byte(&page_category_label_x[i]), 31);
    print_category_name_by_idx(i);
  }
  bool valid_page = page_select < PageRegistry::kMaxPageSlots &&
                    page_entries[page_select].Page != NULL_PAGE;
  const PageSelectEntry *entry =
      valid_page ? &page_entries[page_select] : nullptr;
  const char *name_P = entry ? entry->Name : mclstr_four_dashes;
  if (entry) {
    uint16_t meta = entry->IconMeta;
    uint16_t icon_offset = meta & PageRegistry::kIconOffsetMask;
    iconh = meta >> PageRegistry::kIconHeightShift;
    icon = (icon_offset != 0 && iconh != 0)
               ? reinterpret_cast<uint8_t *>(R.icons_page) + icon_offset - 1
               : nullptr;
  } else {
    icon = nullptr;
    iconh = 0;
  }

  catidx = category_for_slot(page_select);

//  oled_display.fillRect(28, 7, 100, 16, BLACK);
//  oled_display.fillRect(0, 7, 28, 25, BLACK);

  // 4x trig groups
  uint8_t group_x = 28;
  uint8_t pagenr = 0;
  for (uint8_t i = 0; i < 4; ++i) {
    uint8_t trig_x = group_x + 2;
    if (i == catidx) {
      oled_display.fillRect(group_x, 18, 23, 6, WHITE);
    } else {
      oled_display.drawRect(group_x, 18, 23, 6, WHITE);
    }

    for (uint8_t j = 0; j < 4; ++j) {
      if (pagenr == page_select) {
        oled_display.fillRect(trig_x, 19, 4, 4, BLACK);
      } else if (catidx == i) {
        oled_display.fillRect(trig_x + 1, 20, 2, 2, BLACK);
      } else {
        oled_display.fillRect(trig_x + 1, 20, 2, 2, WHITE);
      }

      ++pagenr;
      trig_x += 5;
    }

    group_x += 24;
  }

  oled_display.setFont();
  oled_display.setCursor(29, 9);
  mcl_print_P(name_P);

  if (icon != nullptr) {
    oled_display.drawBitmap(12 - kPageIconWidth / 2, 19 - iconh / 2, icon,
                            kPageIconWidth, iconh, WHITE);
  }

  uint16_t led_mask = (uint16_t)1 << page_select;
  if (trigled_mask != led_mask) {
    trigled_mask = led_mask;
    mcl_gui.set_trigleds(trigled_mask, TRIGLED_EXCLUSIVE);
  }
}

bool PageSelectPage::handleEvent(gui_event_t *event) {
  if (EVENT_NOTE(event)) {
    uint8_t mask = event->mask;
    if (mask == EVENT_BUTTON_PRESSED || mask == EVENT_BUTTON_RELEASED) {
      if (!device_manager.port_supports(
              event->port, MidiDeviceCapability::MdTrigInterface)) {
        return false;
      }
      if (mask == EVENT_BUTTON_PRESSED) {
        uint8_t track = event->source;
        if (page_select != track) {
          page_select = track;
          draw_popup();
        }
      }
      return true;
    }
    return true;
  }
  if (EVENT_CMD(event)) {
    uint8_t key = event->source;
    if (event->mask == EVENT_BUTTON_RELEASED) {
      switch (key) {
      case MDX_KEY_BANKGROUP: {
        goto release;
      }
      }
    } else {
      uint8_t last_page_select = page_select;
      int8_t step = 0;
      bool category_step = false;
      switch (key) {
      case MDX_KEY_NO:
        //  key_interface.ignoreNextEvent(MDX_KEY_NO);
        goto load_grid;
      case MDX_KEY_UP:
        category_step = true;
        step = -1;
        break;
      case MDX_KEY_DOWN:
        category_step = true;
        step = 1;
        break;
      case MDX_KEY_LEFT:
        step = -1;
        break;
      case MDX_KEY_RIGHT:
        step = 1;
        break;
      default:
        return false;
      }
      page_select = category_step ? next_category_page(this, step)
                                  : next_page(this, step);
      if (last_page_select != page_select) {
        draw_popup();
      }
      return true;
    }
  }
  if (EVENT_BUTTON(event)) {
    if (EVENT_RELEASED(event, Buttons.BUTTON2)) {
    release:
      close_to_selection();
      return true;
    }

    if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
    load_grid:
      GUI.ignoreNextEvent(event->source);
      mcl.setPage(GRID_PAGE);
      return true;
    }
    if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
      // PAT SONG
      mcl.setPage(GRID_PAGE);
      mcl.pushPage(SYSTEM_PAGE);
      return true;
    }

    if (event->mask == EVENT_BUTTON_PRESSED &&
        event->source >= Buttons.ENCODER1 &&
        event->source <= Buttons.ENCODER4) {
      page_select = get_category_page(event->source - Buttons.ENCODER1);
      return true;
    }
#ifdef MCL_HAS_EXTENDED_PANEL_INPUT
    // Trig N (0..15) highlights page slot N. Commit happens via the
    // usual close path (ENC1 tap or BUTTON2 release).
    if (event->mask == EVENT_BUTTON_PRESSED &&
        event->source >= Buttons.TRIG_BUTTON1 &&
        event->source < Buttons.TRIG_BUTTON1 + 16) {
      page_select = event->source - Buttons.TRIG_BUTTON1;
      return true;
    }
#endif
  }
  return false;
}

MCLRelativeEncoder page_select_param1(0, 127);
MCLRelativeEncoder page_select_param2(0, 127);
PageSelectPage page_select_page(&page_select_param1, &page_select_param2);
