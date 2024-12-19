/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#pragma once

#include "Arduino.h"
#include "global.h"

#ifdef MEGACOMMAND
#define WAV_DESIGNER
#define SOUND_PAGE
#endif

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
    GRID_PAGE,           // Index: 0
    PAGE_SELECT_PAGE,    // Index: 1
    SYSTEM_PAGE,         // Index: 2
    MIXER_PAGE,          // Index: 3
    GRID_SAVE_PAGE,      // Index: 4
    GRID_LOAD_PAGE,      // Index: 5
#ifdef WAV_DESIGNER
    WD_MIXER_PAGE,       // Index: 6
#endif
    SEQ_STEP_PAGE,       // Index: 7
    SEQ_EXTSTEP_PAGE,    // Index: 8
    SEQ_PTC_PAGE,        // Index: 9
    TEXT_INPUT_PAGE,     // Index: 10
    POLY_PAGE,           // Index: 11
    SAMPLE_BROWSER,       // Index: 12
    QUESTIONDIALOG_PAGE, // Index: 13
    START_MENU_PAGE,     // Index: 14
    BOOT_MENU_PAGE,      // Index: 15
    FX_PAGE_A,           // Index: 16
    FX_PAGE_B,           // Index: 17
#ifdef WAV_DESIGNER
    WD_PAGE_0, // Index: 18
    WD_PAGE_1, // Index: 19
    WD_PAGE_2, // Index: 20
#endif
    ROUTE_PAGE, // Index: 21
    LFO_PAGE,   // Index: 22
    RAM_PAGE_A, // Index: 23
    RAM_PAGE_B, // Index: 24

    LOAD_PROJ_PAGE,           // Index: 25
    MIDI_CONFIG_PAGE,         // Index: 26
    MD_CONFIG_PAGE,           // Index: 27
    CHAIN_CONFIG_PAGE,        // Index: 28
    AUX_CONFIG_PAGE,          // Index: 29
    MCL_CONFIG_PAGE,          // Index: 30
    ARP_PAGE,                 // Index: 31
    MD_IMPORT_PAGE,           // Index: 32
    MIDIPORT_MENU_PAGE,       // Index: 33
    MIDIPROGRAM_MENU_PAGE,    // Index: 34
    MIDICLOCK_MENU_PAGE,      // Index: 35
    MIDIROUTE_MENU_PAGE,      // Index: 36
    MIDIMACHINEDRUM_MENU_PAGE,// Index: 37
    MIDIGENERIC_MENU_PAGE,    // Index: 38
    SOUND_BROWSER,            // Index: 39
    PERF_PAGE_0,             // Index: 40
    NULL_PAGE = 255 
};

class MCL {
public:
  static constexpr uint8_t NUM_PAGES = static_cast<uint8_t>(PageIndex::PERF_PAGE_0) + 1;

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

