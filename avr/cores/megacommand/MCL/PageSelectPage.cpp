#include "MCL_impl.h"
#include "ResourceManager.h"
#include <avr/pgmspace.h>

const PageCategory Categories[] PROGMEM = {
    {"MAIN", 4, 0},
    {"SEQ", 4, 4},
    {"SND", 3, 8},
    {"AUX", 4, 12},
};

constexpr uint8_t n_category = sizeof(Categories) / sizeof(PageCategory);

static uint8_t get_pageidx(uint8_t page_number) {
  uint8_t i = 0;
  for (; i < R.page_entries->countof_Entries; ++i) {
    if (page_number == R.page_entries->Entries[i].PageNumber) {
      return i;
    }
  }
  return NULL_PAGE;
}

static PageIndex get_page(uint8_t pageidx, char *str) {
  if (pageidx < R.page_entries->countof_Entries) {
    if (str) {
      strcpy(str, R.page_entries->Entries[pageidx].Name);
    }
    return R.page_entries->Entries[pageidx].Page;
  } else {
    if (str) {
      strcpy(str, "----");
    }
    return NULL_PAGE;
  }
}

static void get_page_icon(uint8_t pageidx, uint8_t *&icon, uint8_t &w,
                          uint8_t &h) {
  if (pageidx < R.page_entries->countof_Entries) {
    icon = R.page_entries->Entries[pageidx].IconData;
    w = R.page_entries->Entries[pageidx].IconWidth;
    h = R.page_entries->Entries[pageidx].IconHeight;
  } else {
    icon = nullptr;
    w = h = 0;
  }
}

static void get_category_name_by_idx(uint8_t catidx, char *str) {
  if (str) {
    strncpy_P(str, (PGM_P) & (Categories[catidx].Name), 16);
  }
}

static void get_category_name(uint8_t page_number, char *str) {
  uint8_t pageidx, catidx;

  pageidx = get_pageidx(page_number);
  if (pageidx >= R.page_entries->countof_Entries) {
    goto get_category_name_fail;
  }
  catidx = R.page_entries->Entries[pageidx].CategoryId;
  if (catidx >= n_category) {
    goto get_category_name_fail;
  }
  get_category_name_by_idx(catidx, str);
  return;

get_category_name_fail:
  if (str) {
    strcpy(str, "----");
  }
  return;
}

void PageSelectPage::setup() {}
void PageSelectPage::init() {
  DEBUG_PRINTLN("page select init");
  trig_interface.on();
  uint8_t _midi_lock_tmp = MidiUartParent::handle_midi_lock;
  MidiUartParent::handle_midi_lock = 0;
  R.Clear();
  R.use_icons_page();
  R.use_page_entries();
  R.restore_page_entry_deps();
  oled_display.fillRect(0, 0, 128, 7, WHITE);
  oled_display.setFont(&TomThumb);
  oled_display.setTextColor(BLACK);
  oled_display.setCursor(47, 6);
  oled_display.print(F("PAGE SELECT"));
  oled_display.setTextColor(WHITE);

  loop_init = true;
  // md_exploit.on(switch_tracks);
  // clear trigled so it's always sent on first run
  trigled_mask = 0;
  draw_popup();
  md_prepare();
  MidiUartParent::handle_midi_lock = _midi_lock_tmp;
}

void PageSelectPage::draw_popup() {
  char str[16];
  uint8_t pageidx = get_pageidx(page_select);
  get_page(pageidx, str);
  MD.popup_text(str, true);
}

void PageSelectPage::md_prepare() {
  kit_cb.init();
  auto listener = MD.getSysexListener();
  listener->addOnMessageCallback(
      &kit_cb, (sysex_callback_ptr_t)&MDCallback::onReceived);
  MD.requestKit(0x7F);
}

void PageSelectPage::cleanup() {
  note_interface.init_notes();
  MD.set_trigleds(0, TRIGLED_OVERLAY);
  MD.popup_text(127, 2);
}

uint8_t PageSelectPage::get_nextpage_down() {
  for (int8_t i = page_select - 1; i >= 0; i--) {
    if (get_page(get_pageidx(i), nullptr) != NULL_PAGE) {
      return i;
    }
  }
  return page_select;
}

uint8_t PageSelectPage::get_nextpage_up() {
  for (uint8_t i = page_select + 1; i < 16; i++) {
    if (get_page(get_pageidx(i), nullptr) != NULL_PAGE) {
      return i;
    }
  }
  return page_select;
}

uint8_t PageSelectPage::get_nextpage_catdown() {
  auto page_id = get_pageidx(page_select);
  auto cat_id = R.page_entries->Entries[page_id].CategoryId;
  if (cat_id > 0) {
    return pgm_read_byte(&Categories[cat_id - 1].FirstPage);
  } else {
    return page_select;
  }
}

uint8_t PageSelectPage::get_nextpage_catup() {
  auto page_id = get_pageidx(page_select);
  auto cat_id = R.page_entries->Entries[page_id].CategoryId;
  if (cat_id < n_category - 1) {
    return pgm_read_byte(&Categories[cat_id + 1].FirstPage);
  } else {
    return page_select;
  }
}

uint8_t PageSelectPage::get_category_page(uint8_t offset) {
  auto page_id = get_pageidx(page_select);
  auto cat_id = R.page_entries->Entries[page_id].CategoryId;
  auto cat_start = pgm_read_byte(&Categories[cat_id].FirstPage);
  auto cat_size = pgm_read_byte(&Categories[cat_id].PageCount);
  if (offset >= cat_size) {
    return page_select;
  } else {
    return cat_start + offset;
  }
}

void PageSelectPage::loop() {
  /*  if (loop_init) {
      bool switch_tracks = false;
      // md_exploit.off(switch_tracks);
      trig_interface.on();
      md_prepare();
      // md_exploit.on(switch_tracks);
      loop_init = false;
    } */
  uint8_t last_page_select = page_select;
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

  if (last_page_select != page_select) {
    draw_popup();
    last_page_select = page_select;
  }
}

void PageSelectPage::display() {
  char str[16];
  uint8_t *icon;
  uint8_t iconw, iconh;
  uint8_t pageidx;
  uint8_t catidx;

  oled_display.fillRect(0, 7, 128, 25, BLACK);
  oled_display.setFont(&TomThumb);
  oled_display.setTextColor(BLACK);
  oled_display.setCursor(47, 6);
  oled_display.print(F("PAGE SELECT"));
  oled_display.setTextColor(WHITE);
  uint8_t label_pos[4] = {30, 57, 81, 104};
  for (uint8_t i = 0; i < 4; ++i) {
    get_category_name_by_idx(i, str);
    oled_display.setCursor(label_pos[i], 31);
    oled_display.print(str);
  }
  pageidx = get_pageidx(page_select);
  get_page_icon(pageidx, icon, iconw, iconh);
  get_page(pageidx, str);

  if (pageidx < R.page_entries->countof_Entries) {
    catidx = R.page_entries->Entries[pageidx].CategoryId;
  } else {
    catidx = 0xFF;
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
    MD.set_trigleds(trigled_mask, TRIGLED_EXCLUSIVE);
  }
}

bool PageSelectPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {
    uint8_t mask = event->mask;
    uint8_t port = event->port;
    uint8_t device = midi_active_peering.get_device(port)->id;

    uint8_t track = event->source - 128;
    // note interface presses select corresponding page
    if (mask == EVENT_BUTTON_PRESSED) {
      if (device != DEVICE_MD) {
        return false;
      }
      if (page_select != track) {
        page_select = track;
        draw_popup();
      }
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
  if (EVENT_CMD(event)) {
    uint8_t key = event->source - 64;
    if (event->mask == EVENT_BUTTON_RELEASED) {
      switch (key) {
      case MDX_KEY_BANKGROUP: {
        goto release;
      }
      }
    } else {
      uint8_t inc = 1;
      if (trig_interface.is_key_down(MDX_KEY_FUNC)) {
        inc = 8;
      }
      switch (key) {
      case MDX_KEY_YES:
        //  trig_interface.ignoreNextEvent(MDX_KEY_YES);
        break;
      case MDX_KEY_NO:
        //  trig_interface.ignoreNextEvent(MDX_KEY_NO);
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
  if (EVENT_RELEASED(event, Buttons.BUTTON2)) {
  release:
    PageIndex p;
    p = get_page(get_pageidx(page_select), nullptr);
    if (BUTTON_DOWN(Buttons.BUTTON1) || (p == NULL_PAGE)) {
      GUI.ignoreNextEvent(Buttons.BUTTON1);
      //  md_exploit.off();
      mcl.setPage(GRID_PAGE);
    } else {
      mcl.setPage(p);
    }
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
  load_grid:
    GUI.ignoreNextEvent(event->source);
    mcl.setPage(GRID_PAGE);
    return true;
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON3)) {
    //PAT SONG
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

  return false;
}

MCLEncoder page_select_param1(0, 127);
MCLEncoder page_select_param2(0, 127);
PageSelectPage page_select_page(&page_select_param1, &page_select_param2);
