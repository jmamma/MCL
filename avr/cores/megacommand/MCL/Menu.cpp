#include "MCL_impl.h"
#include "ResourceManager.h"

void MenuBase::enable_entry(uint8_t entry_index, bool en) {
  auto midx = entry_index / 8;
  auto bit = entry_index % 8;

  if (en) {
    entry_mask[midx] |= _BV(bit);
  } else {
    entry_mask[midx] &= ~_BV(bit);
  }
}

bool MenuBase::is_entry_enable(uint8_t entry_index) {
  auto midx = entry_index / 8;
  auto bit = entry_index % 8;
  return bit_is_set(entry_mask[midx], bit);
}

menu_function_t MenuBase::get_row_function(uint8_t item_n) {
  const menu_item_t *item = get_item(item_n);
  return (menu_function_t)pgm_read_word(menu_target_functions + item->row_function_id);
}

uint8_t MenuBase::get_number_of_items() {
  uint8_t entry_cnt = get_entry_count();
  uint8_t item_cnt = 0;
  for (auto i = 0; i < entry_cnt; ++i) {
    if (is_entry_enable(i))
      ++item_cnt;
  }
  return item_cnt;
}

const menu_item_t *MenuBase::get_item(uint8_t item_n) {
  uint8_t entry_cnt = get_entry_count();
  for(uint8_t idx = 0; idx < entry_cnt; ++idx) {
    if(is_entry_enable(idx)) {
      if (item_n == 0) {
        return get_entry_address(idx);
      }else {
        --item_n;
      }
    }
  }
  return nullptr;
}

uint8_t MenuBase::get_item_index(uint8_t item_n)
{
  auto pentry = get_item(item_n);
  return pentry - get_entry_address(0);
}

const char* MenuBase::get_item_name(uint8_t item_n) {
  auto *item = get_item(item_n);
  return item->name;
}

LightPage *MenuBase::get_page_callback(uint8_t item_n) {
  auto *item = get_item(item_n);
  return (LightPage*)pgm_read_word(menu_target_pages + item->page_callback_id);
}

uint8_t *MenuBase::get_dest_variable(uint8_t item_n) {
  auto *item = get_item(item_n);
  return (uint8_t*)pgm_read_word(menu_target_param + item->destination_var_id);
}

uint8_t MenuBase::get_option_range(uint8_t item_n) {
  auto *item = get_item(item_n);
  return item->range;
}

uint8_t MenuBase::get_option_min(uint8_t item_n) {
  auto *item = get_item(item_n);
  return item->min;
}

uint8_t MenuBase::get_number_of_options(uint8_t item_n) {
  auto *item = get_item(item_n);
  return item->number_of_options;
}

uint8_t MenuBase::get_options_offset(uint8_t item_n) {
  auto *item = get_item(item_n);
  return item->options_begin;
}

// caller ensures menu_options is loaded in ResMan
const char* MenuBase::get_option_name(uint8_t item_n, uint8_t option_n) {
  uint8_t num_of_options = get_number_of_options(item_n);
  uint8_t options_offset = get_options_offset(item_n);
  menu_option_t* base = R.menu_options->MENU_OPTIONS;
  if (options_offset >= 128) {
    base = custom_options;
    options_offset = 0;
  }
  for (uint8_t a = 0; a < num_of_options; a++) {
    const menu_option_t *option = base + a + options_offset;
    if (option->pos == option_n) {
      return option->name;
    }
  }
  return nullptr;
}
