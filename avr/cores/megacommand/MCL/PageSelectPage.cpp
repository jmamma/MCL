#include "MCL.h"
#include "PageSelectPage.h"
#include <avr/pgmspace.h>

struct PageCategory {
  char Name[8];
  uint8_t PageCount;
  uint8_t FirstPage;
};

struct PageSelectEntry {
  char Name[16];
  LightPage *Page;
  uint8_t PageNumber; // same as trig id
  uint8_t CategoryId;
};

const PageCategory Categories[] PROGMEM = {
    {"MAIN", 4, 0},
    {"SEQ", 4, 4},
    {"SND", 3, 8},
    {"AUX", 4, 12},
};

const PageSelectEntry Entries[] PROGMEM = {
    {"GRID", &grid_page, 0, 0},
    {"MIXER", &mixer_page, 1, 0},
    {"ROUTE", &route_page, 2, 0},
    {"LFO", &lfo_page, 3, 0},

    {"STEP EDIT", &seq_step_page, 4, 1},
    {"RECORD", &seq_rtrk_page, 5, 1},
    {"LOCKS", &seq_param_page[0], 6, 1},
    {"CHROMA", &seq_ptc_page, 7, 1},

    {"SOUND MANAGER", &sound_browser, 8, 2},
    {"WAV DESIGNER", &wd.pages[0], 9, 2},
    {"LOUDNESS", &loudness_page, 10, 2},

    {"DELAY", &fx_page_a, 12, 3},
    {"REVERB", &fx_page_b, 13, 3},
    {"RAM-1", &ram_page_a, 14, 3},
    {"RAM-2", &ram_page_b, 15, 3},
};

constexpr uint8_t n_category = sizeof(Categories) / sizeof(PageCategory);
constexpr uint8_t n_entry = sizeof(Entries) / sizeof(PageSelectEntry);

static uint8_t get_pageidx(uint8_t page_number) {
  uint8_t i = 0;
  for (; i < n_entry; ++i) {
    if (page_number == pgm_read_byte(&Entries[i].PageNumber)) {
      break;
    }
  }
  return i;
}

static LightPage *get_page(uint8_t page_number, char *str) {
  uint8_t pageidx = get_pageidx(page_number);
  if (pageidx < n_entry) {
    if (str) {
      m_strncpy_p(str, (PGM_P) & (Entries[pageidx].Name), 16);
    }
    return pgm_read_word(&Entries[pageidx].Page);
  } else {
    if (str) {
      strncpy(str, "----", 5);
    }
    return NULL;
  }
}

static void get_category_name(uint8_t page_number, char *str) {
  uint8_t pageidx, catidx;

  pageidx= get_pageidx(page_number);
  if (pageidx >= n_entry) {
    goto get_category_name_fail;
  }
  catidx = pgm_read_byte(&Entries[pageidx].CategoryId);
  if (catidx >= n_category) {
    goto get_category_name_fail;
  }
  if (str) {
    m_strncpy_p(str, (PGM_P) & (Categories[catidx].Name), 16);
  }
  return;

get_category_name_fail:
  if (str) {
    strncpy(str, "----", 5);
  }
  return;
}

void PageSelectPage::setup() {}
void PageSelectPage::init() {
#ifdef OLED_DISPLAY
  oled_display.clearDisplay();
#endif
  md_exploit.on();
  note_interface.state = true;
}
void PageSelectPage::cleanup() { note_interface.init_notes(); }

uint8_t PageSelectPage::get_nextpage_down() {
  for (int8_t i = page_select - 1; i >= 0; i--) {
    if (get_page(i, nullptr)) {
      return i;
    }
  }
  return page_select;
}

uint8_t PageSelectPage::get_nextpage_up() {
  for (uint8_t i = page_select + 1; i < 16; i++) {
    if (get_page(i, nullptr)) {
      return i;
    }
  }
  return page_select;
}

uint8_t PageSelectPage::get_nextpage_catdown() {
  auto page_id = get_pageidx(page_select);
  auto cat_id = pgm_read_byte(&Entries[page_id].CategoryId);
  if (cat_id > 0) {
    return pgm_read_byte(&Categories[cat_id - 1].FirstPage);
  } else {
    return page_select;
  }
}

uint8_t PageSelectPage::get_nextpage_catup() {
  auto page_id = get_pageidx(page_select);
  auto cat_id = pgm_read_byte(&Entries[page_id].CategoryId);
  if (cat_id < n_category - 1) {
    return pgm_read_byte(&Categories[cat_id + 1].FirstPage);
  } else {
    return page_select;
  }
}

uint8_t PageSelectPage::get_category_page(uint8_t offset) {
  auto page_id = get_pageidx(page_select);
  auto cat_id = pgm_read_byte(&Entries[page_id].CategoryId);
  auto cat_start = pgm_read_byte(&Categories[cat_id].FirstPage);
  auto cat_size = pgm_read_byte(&Categories[cat_id].PageCount);
  if (offset >= cat_size) {
    return page_select;
  } else {
    return cat_start + offset;
  }
}

void PageSelectPage::loop() {
  MCLEncoder *enc_ = &enc1;
  int8_t diff = enc_->cur - enc_->old;
  if ((diff > 0) && (page_select < 16)) {
    page_select = get_nextpage_up();
  }
  if ((diff < 0) && (page_select > 0)) {
    page_select = get_nextpage_down();
  }

  enc_->cur = 64 + diff;
  enc_->old = 64;

  enc_ = &enc2;
  diff = enc_->cur - enc_->old;
  if ((diff > 0) && (page_select < 16)) {
    page_select = get_nextpage_catup();
  }
  if ((diff < 0) && (page_select > 0)) {
    page_select = get_nextpage_catdown();
  }

  enc_->cur = 64 + diff;
  enc_->old = 64;
}

void PageSelectPage::display() {
#ifdef OLED_DISPLAY
  oled_display.clearDisplay();
#endif
  GUI.setLine(GUI.LINE1);
  char str[16];
  GUI.put_string_at_fill(0, "Page Select:");
  get_category_name(page_select, str);
  GUI.put_string_at(12, str);

  GUI.setLine(GUI.LINE2);
  get_page(page_select, str);
  GUI.put_string_at_fill(0, str);
}

bool PageSelectPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {
    uint8_t mask = event->mask;
    uint8_t port = event->port;
    uint8_t device = midi_active_peering.get_device(port);

    uint8_t track = event->source - 128;
    // note interface presses select corresponding page
    if (mask == EVENT_BUTTON_PRESSED) {
      if (device != DEVICE_MD) {
        return false;
      }
      page_select = track;
      return true;
    }
    if (mask == EVENT_BUTTON_RELEASED) {
      if (device != DEVICE_MD) {
        return false;
      }
      return true;
    }

    return true;
  }
  if (EVENT_RELEASED(event, Buttons.BUTTON2)) {
    LightPage *p;
    p = get_page(page_select, nullptr);
    if (p) {
      GUI.setPage(p);
    } else {
      md_exploit.off();
      GUI.setPage(&grid_page);
    }
    return true;
  }

  if (EVENT_RELEASED(event, Buttons.ENCODER1)) {
    page_select = get_category_page(0);
    return true;
  }
  if (EVENT_RELEASED(event, Buttons.ENCODER2)) {
    page_select = get_category_page(1);
    return true;
  }
  if (EVENT_RELEASED(event, Buttons.ENCODER3)) {
    page_select = get_category_page(2);
    return true;
  }
  if (EVENT_RELEASED(event, Buttons.ENCODER4)) {
    page_select = get_category_page(3);
    return true;
  }

  return false;
}

PageSelectPage page_select_page;
