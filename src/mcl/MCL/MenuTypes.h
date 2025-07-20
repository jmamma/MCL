#pragma once

#include "Arduino.h"
#include "PageIndex.h"

struct PageCategory {
  char Name[8];
  uint8_t PageCount;
  uint8_t FirstPage;
};

struct PageSelectEntry {
  char Name[16];
  PageIndex Page;
  uint8_t PageNumber; // same as trig id
  uint8_t CategoryId;
  uint8_t IconWidth;
  uint8_t IconHeight;
  uint8_t *IconData;
};

typedef void (*menu_function_t)();

// Union to safely handle function pointers in PROGMEM
union menu_function_ptr_t {
    menu_function_t fn;
    #if defined(__AVR__)
        uint16_t word;  // AVR uses 16-bit function pointers
    #else
        struct {
            uint16_t low;
            uint16_t high;
        } words;        // 32-bit architectures
    #endif
};

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

extern const menu_function_ptr_t menu_target_functions[] PROGMEM;
extern const uint8_t* const menu_target_param[] PROGMEM;

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
  uint8_t row_function_id;    // look up the value in menu_target_functions
  uint8_t options_begin;
};

template <uint8_t N> struct menu_t {
  char name[10];
  menu_item_t items[N];
  uint8_t exit_function_id;   // look up the value in menu_target_functions
};
