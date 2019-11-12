#include "MCL.h"
#include "PageSelectPage.h"
#include <avr/pgmspace.h>

struct PageCategory {
  char Name[16];
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
    {"GRID", 1, 0}, {"SEQ", 4, 1},  {"MIX", 3, 5},  {"SOUND", 2, 8},
    {"FX", 2, 10},  {"RAM", 2, 12}, {"LFO", 1, 14}, {"CONFIG", 1, 15},
};

const PageSelectEntry Entries[] PROGMEM = { 
    {"GRID", &grid_page, 0, 0},
    {"MIXER", &mixer_page, 1, 2},
    {"ROUTE", &route_page, 2, 2},
    {"LFO", &lfo_page, 3, 6},

    {"STEP EDIT", &seq_step_page, 4, 1},
    {"RECORD", &seq_rtrk_page, 5, 1},
    {"LOCKS", &seq_param_page[0], 6, 1},
    {"CHROMA", &seq_ptc_page, 7, 1},


    {"WAV DESIGNER", &wd.pages[0], 8, 3},
    {"SOUND MANAGER", &sound_browser, 7, 3},
    {"LOUDNESS", &loudness_page, 9, 2},

    {"DELAY", &fx_page_a, 12, 4},
    {"REVERB", &fx_page_b, 13, 4},

    {"RAM-1", &ram_page_a, 14, 5},
    {"RAM-2", &ram_page_b, 15, 5},


    {"CONFIG", &system_page, 0xFF, 7},
};

constexpr uint8_t n_category = sizeof(Categories) / sizeof(PageCategory);
constexpr uint8_t n_entry = sizeof(Entries) / sizeof(PageSelectEntry);

void PageSelectPage::setup() {}
void PageSelectPage::init() {
#ifdef OLED_DISPLAY
  oled_display.clearDisplay();
#endif
  md_exploit.on();
  note_interface.state = true;
}
void PageSelectPage::cleanup() { note_interface.init_notes(); }

LightPage *PageSelectPage::get_page(uint8_t page_number, char *str) {
  for (uint8_t i = 0; i < n_entry; ++i) {
    if (page_number == pgm_read_byte(&Entries[i].PageNumber)) {
      if (str) {
        m_strncpy_p(str, (PGM_P) & (Entries[i].Name), 16);
      }
      return pgm_read_word(&Entries[i].Page);
    }
  }
  if (str) {
    strncpy(str, "----", 5);
  }
  return NULL;
}

uint8_t PageSelectPage::get_nextpage_down() {
  for (int8_t i = page_select - 1; i >= 0; i--) {
    if (get_page(i)) {
      return i;
    }
  }
  return page_select;
}

uint8_t PageSelectPage::get_nextpage_up() {
  for (uint8_t i = page_select + 1; i < 16; i++) {
    if (get_page(i)) {
      return i;
    }
  }
  return page_select;
}

void PageSelectPage::loop() {
  MCLEncoder *enc_ = &enc1;
  // largest_sine_peak = 1.0 / 16.00;
  int dir = 0;
  int16_t newval;
  int8_t diff = enc_->cur - enc_->old;
  if ((diff > 0) && (page_select < 16)) {
    page_select = get_nextpage_up();
  }
  if ((diff < 0) && (page_select > 0)) {
    page_select = get_nextpage_down();
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
  get_page(page_select, str);
  LightPage *temp = NULL;
  GUI.put_string_at_fill(0, "Page Select:");
  GUI.setLine(GUI.LINE2);
  GUI.put_string_at_fill(0, str);
}

bool PageSelectPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {
    uint8_t mask = event->mask;
    uint8_t port = event->port;
    uint8_t device = midi_active_peering.get_device(port);

    uint8_t track = event->source - 128;
    // note interface presses are treated as musical notes here
    if (event->mask == EVENT_BUTTON_PRESSED) {
      if (device != DEVICE_MD) {
        return false;
      }
      page_select = track;
      return true;
    }
    if (event->mask == EVENT_BUTTON_RELEASED) {
      if (device != DEVICE_MD) {
        return true;
      }
      //  LightPage *p;
      //  p = get_page(page_select);
      // if (p)
      //  GUI.setPage(p);

      return true;
    }

    return true;
  }
  if (EVENT_RELEASED(event, Buttons.BUTTON2)) {
    LightPage *p;
    p = get_page(page_select);
    if (p) {
      GUI.setPage(p);
    } else {
      md_exploit.off();
      GUI.setPage(&grid_page);
    }
    return true;
  }
  if (EVENT_RELEASED(event, Buttons.ENCODER1) ||
      EVENT_RELEASED(event, Buttons.ENCODER2) ||
      EVENT_RELEASED(event, Buttons.ENCODER3) ||
      EVENT_RELEASED(event, Buttons.ENCODER1)) {
    return true;
  }
  return false;
}

PageSelectPage page_select_page;
