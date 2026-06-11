#include "Menu.h"
#include "ResourceManager.h"
#include "SeqTrack.h"

namespace {

char generated_option_name[8];
#if !defined(__AVR__)
const MenuBase *option_name_override_owner = nullptr;
menu_option_name_override_t option_name_override = nullptr;
char option_name_override_buf[8];
#endif

const char *row_option_name(uint8_t row_id) {
  if (row_id >= 128) {
    return nullptr;
  }

  generated_option_name[0] = 'A' + (row_id >> 4);
  uint8_t row = (row_id & 0x0F) + 1;
  if (row < 10) {
    generated_option_name[1] = '0';
    generated_option_name[2] = '0' + row;
  } else {
    generated_option_name[1] = '1';
    generated_option_name[2] = '0' + row - 10;
  }
  generated_option_name[3] = '\0';
  return generated_option_name;
}

const char *transpose_option_name(uint8_t option_id) {
  if (option_id >= 50) {
    return nullptr;
  }

  bool all = option_id >= 25;
  int8_t semitone = (int8_t)option_id - (all ? 37 : 12);
  uint8_t idx = 0;
  if (all) {
    generated_option_name[idx++] = 'A';
    generated_option_name[idx++] = 'L';
    generated_option_name[idx++] = 'L';
    generated_option_name[idx++] = ' ';
  }
  if (semitone < 0) {
    generated_option_name[idx++] = '-';
    semitone = -semitone;
  } else {
    generated_option_name[idx++] = '+';
  }
  uint8_t num = (uint8_t)semitone;
  if (num >= 10) {
    generated_option_name[idx++] = '1';
    num -= 10;
  }
  generated_option_name[idx++] = '0' + num;
  generated_option_name[idx] = '\0';
  return generated_option_name;
}

const char *condition_option_name(uint8_t option_id) {
  if (option_id > SEQ_COND_MAX) {
    return nullptr;
  }
  seq_condition_label(option_id, false, false, generated_option_name);
  return generated_option_name;
}

} // namespace

#if !defined(__AVR__)
void MenuBase::set_option_name_override(
    menu_option_name_override_t override) {
  if (override == nullptr) {
    if (option_name_override_owner == this) {
      option_name_override_owner = nullptr;
      option_name_override = nullptr;
    }
    return;
  }
  option_name_override_owner = this;
  option_name_override = override;
}

menu_option_name_override_t MenuBase::get_option_name_override() const {
  return option_name_override_owner == this ? option_name_override : nullptr;
}
#endif

void MenuBase::enable_entry(uint8_t entry_index, bool en) {
  auto midx = entry_index / 8;
  auto bit = entry_index % 8;

  if (en) {
    disabled_entry_mask[midx] &= ~_BV(bit);
  } else {
    disabled_entry_mask[midx] |= _BV(bit);
  }
}

bool MenuBase::is_entry_enable(uint8_t entry_index) {
  auto midx = entry_index / 8;
  auto bit = entry_index % 8;
  return !IS_BIT_SET(disabled_entry_mask[midx], bit);
}

void MenuBase::set_entry_name(uint8_t entry_index, const char *name) {
  if (layout_base == nullptr || name == nullptr ||
      entry_index >= get_entry_count()) {
    return;
  }
  auto *item = const_cast<menu_item_t *>(get_entry_address(entry_index));
  strncpy(item->name, name, sizeof(item->name) - 1);
  item->name[sizeof(item->name) - 1] = '\0';
}

static menu_function_t read_menu_target_function(uint8_t function_index) {
    #if defined(__AVR__)
        uintptr_t p = pgm_read_word(&menu_target_functions[function_index]);
        return (menu_function_t)p;
    #else
        return menu_target_functions[function_index];
    #endif
}

menu_function_t MenuBase::get_exit_function() {
    if (layout_base == nullptr || exit_fn_id == 0) return nullptr;
    return read_menu_target_function(exit_fn_id - 1);
}

menu_function_t MenuBase::get_row_function(uint8_t item_n) {
    const menu_item_t *item = get_item(item_n);
    if (item == nullptr || item->row_function_id == 0) { return nullptr; }
    return read_menu_target_function(item->row_function_id - 1);
}


uint8_t MenuBase::get_number_of_items() {
  uint8_t entry_cnt = get_entry_count();
  uint8_t item_cnt = 0;
  for (uint8_t i = 0; i < entry_cnt; ++i) {
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

PageIndex MenuBase::get_page_callback(uint8_t item_n) {
  DEBUG_PRINTLN("get page callback");
  auto *item = get_item(item_n);
  DEBUG_PRINTLN(item->page_callback_id);
  if (item == nullptr) { return NULL_PAGE; }
  return (PageIndex) item->page_callback_id;
}

uint8_t *MenuBase::get_dest_variable(uint8_t item_n) {
    const menu_item_t *item = get_item(item_n);
    return get_dest_variable(item);
}

uint8_t *MenuBase::get_dest_variable(const menu_item_t *item) {
    if (item == nullptr) { return nullptr; }

    uintptr_t p;
    #if defined(__AVR__)
        p = pgm_read_word(&menu_target_param[item->destination_var_id]);
    #elif defined(PLATFORM_DESKTOP) || (UINTPTR_MAX > 0xFFFFFFFFu)
        p = (uintptr_t)menu_target_param[item->destination_var_id];
    #else
        memcpy(&p, &menu_target_param[item->destination_var_id], sizeof(p));
    #endif
    return (uint8_t *)p;
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
  return get_option_name(get_item(item_n), option_n);
}

const char* MenuBase::get_option_name(const menu_item_t *item,
                                      uint8_t option_n) {
  if (item == nullptr) {
    return nullptr;
  }
#if !defined(__AVR__)
  if (option_name_override_owner == this && option_name_override != nullptr) {
    uint8_t entry_index = item - get_entry_address(0);
    if (option_name_override(entry_index, option_n, option_name_override_buf,
                             sizeof(option_name_override_buf))) {
      return option_name_override_buf;
    }
  }
#endif

  uint8_t num_of_options = item->number_of_options;
  uint8_t options_offset = item->options_begin;
  if (options_offset == MENU_OPTIONS_CONDITIONS) {
    return condition_option_name(option_n);
  }
  menu_option_t* base = R.menu_options->MENU_OPTIONS;
  if (options_offset >= 192) {
    uint8_t custom_options_idx = options_offset - 192;
    base = custom_options[custom_options_idx];
    if (base == nullptr) {
      if (custom_options_idx == 0 && num_of_options == 128) {
        return row_option_name(option_n);
      }
      if (custom_options_idx == 1 && num_of_options == 51) {
        return transpose_option_name(option_n);
      }
      return nullptr;
    }
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
