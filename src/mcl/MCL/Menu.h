#pragma once

#include "MCL.h"
#include "MenuTypes.h"

#define MAX_MENU_ITEMS 32

constexpr uint32_t menu_entry_mask(uint8_t entry) { return 1UL << entry; }

#if !defined(__AVR__)
typedef bool (*menu_option_name_override_t)(uint8_t entry_index,
                                            uint8_t option_n, char *dst,
                                            uint8_t dst_len);
#endif

class MenuBase {
public:
  // Zero-initialized: no disabled bits means all menu entries are enabled.
  uint8_t disabled_entry_mask[4];
  menu_option_t* custom_options[2];

  // Set by Menu<N>::set_layout — avoids per-N virtual method duplication.
  const void* layout_base = nullptr;  // points to SRAM-unpacked menu_t<N>
  uint8_t entry_count = 0;
  uint8_t exit_fn_id = 0;

  MenuBase() = default;

  /// use a custom options name lookup table.
  /// the table can be dynamically generated, so it is not limited
  /// to PROGMEM content.
  void set_custom_options(menu_option_t* p, uint8_t num) {
    custom_options[num] = p;
  }

#if !defined(__AVR__)
  void set_option_name_override(menu_option_name_override_t override);
  menu_option_name_override_t get_option_name_override() const;
#endif

  void enable_entry(uint8_t entry_index, bool en);
  bool is_entry_enable(uint8_t entry_index);
  void set_entry_name(uint8_t entry_index, const char *name);
  void set_enabled_entry_mask(uint32_t mask) {
    disabled_entry_mask[0] = ~((uint8_t)mask);
    disabled_entry_mask[1] = ~((uint8_t)(mask >> 8));
    disabled_entry_mask[2] = ~((uint8_t)(mask >> 16));
    disabled_entry_mask[3] = ~((uint8_t)(mask >> 24));
  }

  uint8_t *get_dest_variable(uint8_t item_n);
  uint8_t *get_dest_variable(const menu_item_t *item);
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
  const char* get_option_name(const menu_item_t *item, uint8_t option_n);
  menu_function_t get_row_function(uint8_t item_n);

  // Non-virtual: implemented once using layout_base/entry_count/exit_fn_id.
  const char* get_name() { return (const char*)layout_base; }
  menu_function_t get_exit_function();  // defined in Menu.cpp

protected:
  // name[10] is at offset 0 in menu_t<N>; items[] follow immediately at offset 10.
  const menu_item_t* get_entry_address(uint8_t i) {
    return (const menu_item_t*)((const uint8_t*)layout_base + 10) + i;
  }
  uint8_t get_entry_count() { return entry_count; }
};

template <int N> class Menu : public MenuBase {
  static_assert(N <= MAX_MENU_ITEMS, "Menu exceeds MAX_MENU_ITEMS");
  static_assert(offsetof(menu_t<1>, items) == 10,
                "menu_t::name size changed — update get_entry_address offset");
public:
  Menu() : MenuBase(){};

  void set_layout(const menu_t<N>* layout) {
    layout_base = layout;
    entry_count = N;
    exit_fn_id = layout->exit_function_id;
  }
};
