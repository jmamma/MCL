#pragma once

#include "mcl.h"
#include "MenuTypes.h"

#define MAX_MENU_ITEMS 16

class MenuBase {
private:
public:
  uint8_t entry_mask[4];
  menu_option_t* custom_options[2];

  MenuBase() {
    memset(entry_mask, 0xFF, sizeof(entry_mask));
    memset(custom_options,0,sizeof(custom_options));
  }

  /// use a custom options name lookup table.
  /// the table can be dynamically generated, so it is not limited
  /// to PROGMEM content.
  void set_custom_options(menu_option_t* p, uint8_t num) {
    custom_options[num] = p;
  }

  void enable_entry(uint8_t entry_index, bool en);
  bool is_entry_enable(uint8_t entry_index);

  uint8_t *get_dest_variable(uint8_t item_n);
  uint8_t get_option_min(uint8_t item_n);
  uint8_t get_option_range(uint8_t item_n);
  uint8_t get_number_of_options(uint8_t item_n);
  uint8_t get_options_offset(uint8_t item_n);
  PageIndex get_page_callback(uint8_t item_n);
  uint8_t get_number_of_items();
  const menu_item_t *get_item(uint8_t item_n);
  const char* get_item_name(uint8_t item_n);
  uint8_t get_item_index(uint8_t item_n);
  const char* get_option_name(uint8_t item_n, uint8_t option_n);
  menu_function_t get_row_function(uint8_t item_n);

  virtual const char* get_name() = 0;
  virtual menu_function_t get_exit_function() = 0;

protected:
  virtual const menu_item_t *get_entry_address(uint8_t) = 0;
  virtual uint8_t get_entry_count() = 0;
};

// TODO raise error if N > MAX_MENU_ITEMS
template <int N> class Menu : public MenuBase {

public:
  Menu() : MenuBase(){};
  const menu_t<N> *layout;

  void set_layout(const menu_t<N> *layout) {
    this->layout = layout;
  }
  virtual const char* get_name() { return layout->name; }
  virtual menu_function_t get_exit_function() override {
        if (layout == nullptr) return nullptr;
        menu_function_ptr_t fn;
        #if defined(__AVR__)
            // On AVR, single 16-bit read
            fn.word = pgm_read_word(&menu_target_functions[layout->exit_function_id].word);
        #else
            // On 32-bit architectures, read both words
            fn.words.low = pgm_read_word(&menu_target_functions[layout->exit_function_id].words.low);
            fn.words.high = pgm_read_word(&menu_target_functions[layout->exit_function_id].words.high);
        #endif
        return fn.fn;
  }
  virtual const menu_item_t *get_entry_address(uint8_t i) { return layout->items + i; }
  virtual uint8_t get_entry_count() { return N; };
};

