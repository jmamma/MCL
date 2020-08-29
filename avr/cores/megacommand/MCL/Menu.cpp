#include "MCL_impl.h"

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

FP MenuBase::get_row_function(uint8_t item_n) {
  const menu_item_t *item = get_item(item_n);
  return pgm_read_word(&(item->row_function));
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

PGM_P MenuBase::get_item_name(uint8_t item_n) {
  auto *item = get_item(item_n);
  return item->name;
}

Page *MenuBase::get_page_callback(uint8_t item_n) {
  auto *item = get_item(item_n);
  return pgm_read_word(&(item->page_callback));
}

uint8_t *MenuBase::get_dest_variable(uint8_t item_n) {
  auto *item = get_item(item_n);
  return pgm_read_word(&(item->destination_var));
}

uint8_t MenuBase::get_option_range(uint8_t item_n) {
  auto *item = get_item(item_n);
  return pgm_read_byte(&(item->range));
}

uint8_t MenuBase::get_option_min(uint8_t item_n) {
  auto *item = get_item(item_n);
  return pgm_read_byte(&(item->min));
}

uint8_t MenuBase::get_number_of_options(uint8_t item_n) {
  auto *item = get_item(item_n);
  return pgm_read_byte(&(item->number_of_options));
}

uint8_t MenuBase::get_options_offset(uint8_t item_n) {
  auto *item = get_item(item_n);
  return pgm_read_byte(&(item->options_begin));
}

PGM_P MenuBase::get_option_name(uint8_t item_n, uint8_t option_n) {
  auto *item = get_item(item_n);
  const menu_option_t *option;
  uint8_t num_of_options = get_number_of_options(item_n);
  uint8_t options_offset = get_options_offset(item_n);
  for (uint8_t a = 0; a < num_of_options; a++) {
    option = MENU_OPTIONS+a+options_offset;
    if (pgm_read_byte(&(option->pos)) == option_n) {
      return option->name;
    }
  }
  return NULL;
}
