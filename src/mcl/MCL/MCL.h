/* Copyright Justin Mammarella jmamma@gmail.com 2018 */

#pragma once

#include "Arduino.h"
#include "global.h"
#include "PageIndex.h"
#include "MCLDefines.h"

#define VERSION 4070
#define VERSION_STR "D4.70"

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
  void loop();
};

extern MCL mcl;

#ifdef PLATFORM_TBD
bool tbd_handleEvent(gui_event_t *event);
#endif

bool mcl_handleEvent(gui_event_t *event);

