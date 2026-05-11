#include "PageSelectPage.h"
#include "ResourceManager.h"
#include "MCLGUI.h"
#include "DeviceManager.h"
#include "MCLStrings.h"
#include "../Drivers/MidiDevice.h"
#include "../Drivers/PageRegistry.h"

const PageCategory Categories[] PROGMEM = {
    {"MAIN", 4, 0},
    {"SEQ", 4, 4},
    {"SND", 3, 8},
    {"AUX", 4, 12},
};

constexpr uint8_t n_category = sizeof(Categories) / sizeof(PageCategory);

namespace {

const char page_name_perf[] PROGMEM = "PERF";
const char page_name_step_edit[] PROGMEM = "STEP EDIT";
const char page_name_lfo[] PROGMEM = "LFO";
const char page_name_piano_roll[] PROGMEM = "PIANO ROLL";
const char page_name_chromatic[] PROGMEM = "CHROMATIC";
const char page_name_sample_manager[] PROGMEM = "SAMPLE MANAGER";
const char page_name_wav_designer[] PROGMEM = "WAV DESIGNER";

static uint8_t category_for_slot(uint8_t slot) {
  uint8_t cat = slot / 4;
  return cat < n_category ? cat : (n_category - 1);
}

static MidiDevice *nonnull_device(MidiDevice *device) {
  return (device == nullptr || device == &null_midi_device) ? nullptr : device;
}

static void add_common_page(PageSelectEntry *entries, const char *name_P,
                            PageIndex page, uint8_t slot, uint8_t category,
                            uint8_t icon_width, uint8_t icon_height,
                            PageSelectIcon icon) {
  PageRegistry::add_P(entries, PageRegistry::kMaxPageSlots, name_P, page,
                      slot, category, icon_width, icon_height, icon);
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

void PageSelectPage::get_page_icon(uint8_t page_number, uint8_t *&icon,
                                   uint8_t &w, uint8_t &h) const {
  if (page_number >= PageRegistry::kMaxPageSlots ||
      page_entries[page_number].Page == NULL_PAGE) {
    icon = nullptr;
    w = h = 0;
    return;
  }

  const PageSelectEntry &entry = page_entries[page_number];
  w = entry.IconWidth;
  h = entry.IconHeight;

  switch (entry.Icon) {
  case PAGE_ICON_GRID:
    icon = R.icons_page->icon_grid;
    break;
  case PAGE_ICON_MIXER:
    icon = R.icons_page->icon_mixer;
    break;
  case PAGE_ICON_PERF:
    icon = R.icons_page->icon_perf;
    break;
  case PAGE_ICON_ROUTE:
    icon = R.icons_page->icon_route;
    break;
  case PAGE_ICON_STEP:
    icon = R.icons_page->icon_step;
    break;
  case PAGE_ICON_LFO:
    icon = R.icons_page->icon_lfo;
    break;
  case PAGE_ICON_PIANOROLL:
    icon = R.icons_page->icon_pianoroll;
    break;
  case PAGE_ICON_CHROMA:
    icon = R.icons_page->icon_chroma;
    break;
  case PAGE_ICON_SAMPLE:
    icon = R.icons_page->icon_sample;
    break;
  case PAGE_ICON_WAVD:
    icon = R.icons_page->icon_wavd;
    break;
  case PAGE_ICON_RHYTMECHO:
    icon = R.icons_page->icon_rhytmecho;
    break;
  case PAGE_ICON_GATEBOX:
    icon = R.icons_page->icon_gatebox;
    break;
  case PAGE_ICON_RAM1:
    icon = R.icons_page->icon_ram1;
    break;
  case PAGE_ICON_RAM2:
    icon = R.icons_page->icon_ram2;
    break;
  default:
    icon = nullptr;
    w = h = 0;
    break;
  }
}

static void get_category_name_by_idx(uint8_t catidx, char *str) {
  if (str) {
    strncpy_P(str, (PGM_P) & (Categories[catidx].Name), 16);
  }
}

void PageSelectPage::init() {
  DEBUG_PRINTLN("page select init");
  key_interface.on();
  uint8_t _midi_lock_tmp = MidiUartParent::handle_midi_lock;
  MidiUartParent::handle_midi_lock = 0;
  R.Clear();
  R.use_icons_page();
  rebuild_entries();
  oled_display.fillRect(0, 0, 128, 7, WHITE);
  oled_display.setFont(&TomThumb);
  oled_display.setTextColor(BLACK);
  oled_display.setCursor(47, 6);
  mcl_print_P(mclstr_page_select);
  oled_display.setTextColor(WHITE);

  loop_init = true;
  // md_exploit.on(switch_tracks);
  // clear trigled so it's always sent on first run
  trigled_mask = 0;
  draw_popup();
  MidiUartParent::handle_midi_lock = 1;
  md_prepare();
  MidiUartParent::handle_midi_lock = _midi_lock_tmp;
}

void PageSelectPage::draw_popup() {
  char str[16];
  get_page(page_select, str);
  MidiDevice *device = nonnull_device(page_select_ui_device);
  if (device != nullptr) {
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

uint8_t PageSelectPage::get_nextpage_down() {
  for (int8_t i = page_select - 1; i >= 0; i--) {
    if (get_page(i, nullptr) != NULL_PAGE) {
      return i;
    }
  }
  return page_select;
}

uint8_t PageSelectPage::get_nextpage_up() {
  for (uint8_t i = page_select + 1; i < PageRegistry::kMaxPageSlots; i++) {
    if (get_page(i, nullptr) != NULL_PAGE) {
      return i;
    }
  }
  return page_select;
}

uint8_t PageSelectPage::get_nextpage_catdown() {
  auto cat_id = category_for_slot(page_select);
  if (cat_id > 0) {
    return pgm_read_byte(&Categories[cat_id - 1].FirstPage);
  } else {
    return page_select;
  }
}

uint8_t PageSelectPage::get_nextpage_catup() {
  auto cat_id = category_for_slot(page_select);
  if (cat_id < n_category - 1) {
    return pgm_read_byte(&Categories[cat_id + 1].FirstPage);
  } else {
    return page_select;
  }
}

uint8_t PageSelectPage::get_category_page(uint8_t offset) {
  auto cat_id = category_for_slot(page_select);
  auto cat_start = pgm_read_byte(&Categories[cat_id].FirstPage);
  auto cat_size = pgm_read_byte(&Categories[cat_id].PageCount);
  if (offset >= cat_size) {
    return page_select;
  } else {
    return cat_start + offset;
  }
}

void PageSelectPage::rebuild_entries() {
  PageRegistry::clear(page_entries, PageRegistry::kMaxPageSlots);
  page_select_ui_device = nullptr;

  add_common_page(page_entries, mclstr_grid, GRID_PAGE, 0, 0, 24, 15,
                  PAGE_ICON_GRID);
  add_common_page(page_entries, mclstr_mixer, MIXER_PAGE, 1, 0, 24, 16,
                  PAGE_ICON_MIXER);
  add_common_page(page_entries, page_name_perf, PERF_PAGE_0, 2, 0, 24, 18,
                  PAGE_ICON_PERF);

  add_common_page(page_entries, page_name_step_edit, SEQ_STEP_PAGE, 4, 1, 24,
                  21, PAGE_ICON_STEP);
  add_common_page(page_entries, page_name_lfo, LFO_PAGE, 5, 1, 24, 24,
                  PAGE_ICON_LFO);
  add_common_page(page_entries, page_name_piano_roll, SEQ_EXTSTEP_PAGE, 6, 1,
                  24, 25, PAGE_ICON_PIANOROLL);
  add_common_page(page_entries, page_name_chromatic, SEQ_PTC_PAGE, 7, 1, 24,
                  25, PAGE_ICON_CHROMA);

#ifdef SOUND_PAGE
  add_common_page(page_entries, page_name_sample_manager, SAMPLE_BROWSER, 8, 2,
                  24, 25, PAGE_ICON_SAMPLE);
#endif
#ifdef WAV_DESIGNER
  add_common_page(page_entries, page_name_wav_designer, WD_PAGE_0, 9, 2, 24,
                  19, PAGE_ICON_WAVD);
#endif

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
  /*  if (loop_init) {
      bool switch_tracks = false;
      // md_exploit.off(switch_tracks);
      key_interface.on();
      md_prepare();
      // md_exploit.on(switch_tracks);
      loop_init = false;
    } */
  uint8_t last_page_select = page_select;
  auto enc_ = (MCLEncoder *)encoders[0];
  int8_t diff = enc_->cur;
  if ((diff > 0) && (page_select < 16)) {
    page_select = get_nextpage_up();
  }
  if ((diff < 0) && (page_select > 0)) {
    page_select = get_nextpage_down();
  }

  enc_ = (MCLEncoder *)encoders[1];
  diff = enc_->cur;
  if ((diff > 0) && (page_select < 16)) {
    page_select = get_nextpage_catup();
  }
  if ((diff < 0) && (page_select > 0)) {
    page_select = get_nextpage_catdown();
  }

  if (last_page_select != page_select) {
    draw_popup();
    last_page_select = page_select;
  }
}

void PageSelectPage::display() {
  char str[16];
  uint8_t *icon;
  uint8_t iconw, iconh;
  uint8_t catidx;

  oled_display.fillRect(0, 7, 128, 25, BLACK);
  oled_display.setFont(&TomThumb);
  oled_display.setTextColor(BLACK);
  oled_display.setCursor(47, 6);
  mcl_print_P(mclstr_page_select);
  oled_display.setTextColor(WHITE);
  uint8_t label_pos[4] = {30, 57, 81, 104};
  for (uint8_t i = 0; i < 4; ++i) {
    get_category_name_by_idx(i, str);
    oled_display.setCursor(label_pos[i], 31);
    oled_display.print(str);
  }
  get_page_icon(page_select, icon, iconw, iconh);
  get_page(page_select, str);

  if (page_select < PageRegistry::kMaxPageSlots &&
      page_entries[page_select].Page != NULL_PAGE) {
    catidx = page_entries[page_select].CategoryId;
  } else {
    catidx = category_for_slot(page_select);
  }

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
  oled_display.print(str);

  if (icon != nullptr) {
    oled_display.drawBitmap(12 - iconw / 2, 19 - iconh / 2, icon, iconw, iconh,
                            WHITE);
  }

  uint16_t led_mask = 1 << page_select;
  if (trigled_mask != led_mask) {
    trigled_mask = led_mask;
    mcl_gui.set_trigleds(trigled_mask, TRIGLED_EXCLUSIVE);
  }
}

bool PageSelectPage::handleEvent(gui_event_t *event) {
  if (EVENT_NOTE(event)) {
    uint8_t mask = event->mask;
    uint8_t port = event->port;
    const bool is_md_port = device_manager.port_supports(
        port, MidiDeviceCapability::MdTrigInterface);

    uint8_t track = event->source;
    // note interface presses select corresponding page
    if (mask == EVENT_BUTTON_PRESSED) {
      if (!is_md_port) {
        return false;
      }
      if (page_select != track) {
        page_select = track;
        draw_popup();
      }
      return true;
    }
    if (mask == EVENT_BUTTON_RELEASED) {
      if (!is_md_port) {
        return false;
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
      uint8_t inc = 1;
      if (key_interface.is_key_down(MDX_KEY_FUNC)) {
        inc = 8;
      }
      switch (key) {
      case MDX_KEY_YES:
        //  key_interface.ignoreNextEvent(MDX_KEY_YES);
        break;
      case MDX_KEY_NO:
        //  key_interface.ignoreNextEvent(MDX_KEY_NO);
        goto load_grid;
      case MDX_KEY_UP:
        encoders[1]->cur -= inc;
        break;
      case MDX_KEY_DOWN:
        encoders[1]->cur += inc;
        break;
      case MDX_KEY_LEFT:
        encoders[0]->cur -= inc;
        break;
      case MDX_KEY_RIGHT:
        encoders[0]->cur += inc;
        break;
      }
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

    if (EVENT_PRESSED(event, Buttons.ENCODER1)) {
      page_select = get_category_page(0);
      return true;
    }
    if (EVENT_PRESSED(event, Buttons.ENCODER2)) {
      page_select = get_category_page(1);
      return true;
    }
    if (EVENT_PRESSED(event, Buttons.ENCODER3)) {
      page_select = get_category_page(2);
      return true;
    }
    if (EVENT_PRESSED(event, Buttons.ENCODER4)) {
      page_select = get_category_page(3);
      return true;
    }
#ifdef PLATFORM_TBD
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
