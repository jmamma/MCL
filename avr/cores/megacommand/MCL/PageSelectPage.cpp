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
  uint8_t IconWidth;
  uint8_t IconHeight;
  uint8_t *IconData;
};

const PageCategory Categories[] PROGMEM = {
    {"MAIN", 4, 0},
    {"SEQ", 4, 4},
    {"SND", 3, 8},
    {"AUX", 4, 12},
};

const PageSelectEntry Entries[] PROGMEM = {
    {"GRID", &grid_page, 0, 0, 24, 15, (uint8_t *)icon_grid},
    {"MIXER", &mixer_page, 1, 0, 24, 16, (uint8_t *)icon_mixer},
    {"ROUTE", &route_page, 2, 0, 24, 16, (uint8_t *)icon_route},
    {"LFO", &lfo_page, 3, 0, 24, 24, (uint8_t *)icon_lfo},

    {"STEP EDIT", &seq_step_page, 4, 1, 24, 25, (uint8_t *)icon_step},
    {"RECORD", &seq_rtrk_page, 5, 1, 24, 15, (uint8_t *)icon_rec},
    {"LOCKS", &seq_param_page[0], 6, 1, 24, 19, (uint8_t *)icon_para},
    {"CHROMA", &seq_ptc_page, 7, 1, 24, 25, (uint8_t *)icon_chroma},
#ifdef SOUND_PAGE
    {"SOUND MANAGER", &sound_browser, 8, 2, 24, 19, (uint8_t *)icon_sound},
#endif
#ifdef WAV_DESIGNER
    {"WAV DESIGNER", &wd.pages[0], 9, 2, 24, 19, (uint8_t *)icon_wavd},
#endif
#ifdef LOUDNESS_PAGE
    {"LOUDNESS", &loudness_page, 10, 2, 24, 16, (uint8_t *)icon_loudness},
#endif
    {"DELAY", &fx_page_a, 12, 3, 24, 25, (uint8_t *)icon_rhytmecho},
    {"REVERB", &fx_page_b, 13, 3, 24, 25, (uint8_t *)icon_gatebox},
    {"RAM-1", &ram_page_a, 14, 3, 19, 19, (uint8_t *)wheel_top},
    {"RAM-2", &ram_page_b, 15, 3, 19, 19, (uint8_t *)wheel_angle},
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

static LightPage *get_page(uint8_t pageidx, char *str) {
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

static void get_page_icon(uint8_t pageidx, const uint8_t *&icon, uint8_t &w,
                          uint8_t &h) {
  if (pageidx < n_entry) {
    icon = pgm_read_word(&Entries[pageidx].IconData);
    w = pgm_read_byte(&Entries[pageidx].IconWidth);
    h = pgm_read_byte(&Entries[pageidx].IconHeight);
  } else {
    icon = nullptr;
    w = h = 0;
  }
}

static void get_category_name_by_idx(uint8_t catidx, char *str) {
  if (str) {
    m_strncpy_p(str, (PGM_P) & (Categories[catidx].Name), 16);
  }
}

static void get_category_name(uint8_t page_number, char *str) {
  uint8_t pageidx, catidx;

  pageidx = get_pageidx(page_number);
  if (pageidx >= n_entry) {
    goto get_category_name_fail;
  }
  catidx = pgm_read_byte(&Entries[pageidx].CategoryId);
  if (catidx >= n_category) {
    goto get_category_name_fail;
  }
  get_category_name_by_idx(catidx, str);
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
  oled_display.fillRect(0, 0, 128, 7, WHITE);
  oled_display.setFont(&TomThumb);
  oled_display.setTextColor(BLACK);
  oled_display.setCursor(47, 6);
  oled_display.print("PAGE SELECT");
  oled_display.setTextColor(WHITE);
  char str[16];
  uint8_t label_pos[4] = {30, 57, 81, 104};
  for (uint8_t i = 0; i < 4; ++i) {
    get_category_name_by_idx(i, str);
    oled_display.setCursor(label_pos[i], 31);
    oled_display.print(str);
  }
  classic_display = false;
#endif
  last_md_track = MD.getCurrentTrack(CALLBACK_TIMEOUT);
  loop_init = true;
}

void PageSelectPage::md_prepare() {
#ifndef USE_BLOCKINGKIT
  kit_cb.init();

  MDSysexListener.addOnKitMessageCallback(
      &kit_cb,
      (md_callback_ptr_t)&MDBlockCurrentStatusCallback::onSysexReceived);
#endif
  if (MD.connected) {
    MD.currentKit = MD.getCurrentKit(CALLBACK_TIMEOUT);
    if ((mcl_cfg.auto_save == 1)) {
      MD.saveCurrentKit(MD.currentKit);
#ifdef USE_BLOCKINGKIT
      MD.getBlockingKit(MD.currentKit, CALLBACK_TIMEOUT);
      if (MidiClock.state == 2) {
        // Restore kit param values that are being modulaated by locks
        mcl_seq.update_kit_params();
      }
#else
      MD.requestKit(MD.currentKit);
      delay(20);
#endif
    }
  }
}

void PageSelectPage::cleanup() {
#ifndef USE_BLOCKINGKIT
  uint16_t myclock = slowclock;
  if (!loop_init) {
    while (!kit_cb.received && (clock_diff(myclock, slowclock) < 400))
      ;
    if (kit_cb.received) {
      MD.kit.fromSysex(MD.midi);
      if (MidiClock.state == 2) {
        mcl_seq.update_kit_params();
      }
    }
  }
  MDSysexListener.removeOnKitMessageCallback(&kit_cb);
#endif
  note_interface.init_notes();
}

uint8_t PageSelectPage::get_nextpage_down() {
  for (int8_t i = page_select - 1; i >= 0; i--) {
    if (get_page(get_pageidx(i), nullptr)) {
      return i;
    }
  }
  return page_select;
}

uint8_t PageSelectPage::get_nextpage_up() {
  for (uint8_t i = page_select + 1; i < 16; i++) {
    if (get_page(get_pageidx(i), nullptr)) {
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
  if (loop_init) {
    bool switch_tracks = false;
    //md_exploit.off(switch_tracks);
    md_prepare();
    //md_exploit.on(switch_tracks);
    trig_interface.on();
    note_interface.state = true;
    loop_init = false;
  }

  auto enc_ = (MCLEncoder *)encoders[0];
  int8_t diff = enc_->cur - enc_->old;
  if ((diff > 0) && (page_select < 16)) {
    page_select = get_nextpage_up();
  }
  if ((diff < 0) && (page_select > 0)) {
    page_select = get_nextpage_down();
  }

  enc_->cur = 64 + diff;
  enc_->old = 64;

  enc_ = (MCLEncoder *)encoders[1];
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
  char str[16];
  const uint8_t *icon;
  uint8_t iconw, iconh;
  uint8_t pageidx;
  uint8_t catidx;

  pageidx = get_pageidx(page_select);
  get_page_icon(pageidx, icon, iconw, iconh);
  get_page(pageidx, str);

  if (pageidx < n_entry) {
    catidx = pgm_read_byte(&Entries[pageidx].CategoryId);
  } else {
    catidx = 0xFF;
  }

  oled_display.fillRect(28, 7, 100, 16, BLACK);
  oled_display.fillRect(0, 7, 25, 25, BLACK);

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

  oled_display.display();
#else
  GUI.setLine(GUI.LINE1);
  char str[16];
  GUI.put_string_at_fill(0, "Page Select:");
  get_category_name(page_select, str);
  GUI.put_string_at(12, str);

  GUI.setLine(GUI.LINE2);
  get_page(get_pageidx(page_select), str);
  GUI.put_string_at_fill(0, str);
#endif
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
    p = get_page(get_pageidx(page_select), nullptr);
    if (BUTTON_DOWN(Buttons.BUTTON1) || (!p)) {
      GUI.ignoreNextEvent(Buttons.BUTTON1);
      trig_interface.off();
    //  md_exploit.off();
      GUI.setPage(&grid_page);
    } else {
      GUI.setPage(p);
    }
    return true;
  }

  if (EVENT_RELEASED(event, Buttons.BUTTON1)) {
    trig_interface.off();
    GUI.ignoreNextEvent(event->source);
    GUI.setPage(&grid_page);
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

  return false;
}

MCLEncoder page_select_param1(0, 127);
MCLEncoder page_select_param2(0, 127);
PageSelectPage page_select_page(&page_select_param1, &page_select_param2);
