/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#pragma once

#include "Arduino.h"
#include "global.h"

#define WAV_DESIGNER
#define SOUND_PAGE

#define VERSION 4060
#define VERSION_STR "B4.61"

#define CALLBACK_TIMEOUT 500
#define GUI_NAME_TIMEOUT 800

#define MD_KITBUF_POS 63

extern void mcl_setup();

union lightpage_ptr_t {
    LightPage* ptr;
    #if defined(__AVR__)
        uint16_t word;  // AVR uses 16-bit pointers
    #else
        struct {
            uint16_t low;
            uint16_t high;
        } words;        // 32-bit architectures
    #endif
};
enum PageIndex {
    // Core pages
    GRID_PAGE = 0,
    PAGE_SELECT_PAGE,
    SYSTEM_PAGE,
    MIXER_PAGE,
    GRID_SAVE_PAGE,
    GRID_LOAD_PAGE,
    // Main sequence pages
    SEQ_STEP_PAGE,
    SEQ_EXTSTEP_PAGE,
    SEQ_PTC_PAGE,
    // UI pages
    TEXT_INPUT_PAGE,
    POLY_PAGE,
    SAMPLE_BROWSER,
    QUESTIONDIALOG_PAGE,
    START_MENU_PAGE,
    BOOT_MENU_PAGE,
    // Effect pages
    FX_PAGE_A,
    FX_PAGE_B,
    ROUTE_PAGE,
    LFO_PAGE,
    // Memory pages
    RAM_PAGE_A,
    RAM_PAGE_B,
    // Configuration pages
    LOAD_PROJ_PAGE,
    MIDI_CONFIG_PAGE,
    MD_CONFIG_PAGE,
    CHAIN_CONFIG_PAGE,
    AUX_CONFIG_PAGE,
    MCL_CONFIG_PAGE,
    // Additional feature pages
    ARP_PAGE,
    MD_IMPORT_PAGE,
    // MIDI menu pages
    MIDIPORT_MENU_PAGE,
    MIDIPROGRAM_MENU_PAGE,
    MIDICLOCK_MENU_PAGE,
    MIDIROUTE_MENU_PAGE,
    MIDIMACHINEDRUM_MENU_PAGE,
    MIDIGENERIC_MENU_PAGE,
    // Browser pages
    SOUND_BROWSER,
    // Performance page
    PERF_PAGE_0,
#ifdef WAV_DESIGNER
    // WAV Designer pages - grouped together at the end
    WD_MIXER_PAGE,
    WD_PAGE_0,
    WD_PAGE_1,
    WD_PAGE_2,
#endif
    // Special values
    NUM_PAGES,  // Automatically tracks total number of pages
    NULL_PAGE = 255
};

class MCL {
public:

  static const lightpage_ptr_t pages_table[NUM_PAGES] PROGMEM;

  PageIndex current_page = GRID_PAGE;

  LightPage *getPage(PageIndex page) {
    if (page >= NUM_PAGES) return nullptr;

    lightpage_ptr_t p;
#if defined(__AVR__)
    p.word = pgm_read_word(&pages_table[page].word);
#else
    p.words.low = pgm_read_word(&pages_table[page].words.low);
    p.words.high = pgm_read_word(&pages_table[page].words.high);
    #endif
    return p.ptr;
  }


  void setPage(PageIndex page) {
    if (page >= NUM_PAGES) {
      page = GRID_PAGE;
    }
    current_page = page;
    GUI.setPage(getPage(page));
  }

  void pushPage(PageIndex page) {
    if (page >= NUM_PAGES) {
      page = GRID_PAGE;
    }
    current_page = page;
    GUI.pushPage(getPage(page));
  }

  void popPage() {
    GUI.popPage();
    for (uint8_t n = 0; n < NUM_PAGES; n++) {
      if (GUI.currentPage() == getPage((PageIndex) n)) {
        current_page = (PageIndex) n;
        return;
      }
    }
    current_page = NULL_PAGE;
  }

  bool isSeqPage() {
     return current_page == SEQ_STEP_PAGE || current_page == SEQ_PTC_PAGE || current_page == SEQ_EXTSTEP_PAGE;
  }

  PageIndex currentPage() { return current_page; }

  void setup();
};

extern MCL mcl;

bool mcl_handleEvent(gui_event_t *event);

