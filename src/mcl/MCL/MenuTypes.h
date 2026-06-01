#pragma once

#include "Arduino.h"
#include "PageIndex.h"

enum PageSelectIcon : uint8_t {
  PAGE_ICON_NONE = 0,
  PAGE_ICON_GRID,
  PAGE_ICON_MIXER,
  PAGE_ICON_PERF,
  PAGE_ICON_ROUTE,
  PAGE_ICON_STEP,
  PAGE_ICON_LFO,
  PAGE_ICON_PIANOROLL,
  PAGE_ICON_CHROMA,
  PAGE_ICON_SAMPLE,
  PAGE_ICON_WAVD,
  PAGE_ICON_RHYTMECHO,
  PAGE_ICON_GATEBOX,
  PAGE_ICON_RAM1,
  PAGE_ICON_RAM2,
};

struct PageSelectEntry {
  const char *Name;
  PageIndex Page;
  // Packed as: bits 0..9 icon resource offset + 1, 10..14 height.
  uint16_t IconMeta;
};

typedef void (*menu_function_t)();

union uint8_ptr_t {
    uint8_t* ptr;
    #if defined(__AVR__)
        uint16_t word;  // AVR uses 16-bit pointers
    #else
        struct {
            uint16_t low;
            uint16_t high;
        } words;        // 32-bit architectures
    #endif
};

// Menu function id 0 means no callback. Non-zero ids are one-based indexes
// into menu_target_functions.
extern const menu_function_t menu_target_functions[] PROGMEM;
extern const uint8_t* const menu_target_param[] PROGMEM;

#define MENU_OPTIONS_PERCENT 191

struct menu_option_t {
  uint8_t pos;
  char name[8];
};

struct menu_item_t {
  char name[14];
  uint8_t min;
  uint8_t range;
  uint8_t number_of_options;
  uint8_t destination_var_id; // look up the value in menu_target_param
  PageIndex page_callback_id;   // look up the page callback in menu_target_pages
  uint8_t row_function_id;    // 0 or one-based menu_target_functions id
  uint8_t options_begin;
};

template <uint8_t N> struct menu_t {
  char name[10];
  menu_item_t items[N];
  uint8_t exit_function_id;   // 0 or one-based menu_target_functions id
};
