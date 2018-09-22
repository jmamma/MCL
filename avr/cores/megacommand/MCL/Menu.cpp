#include "MCL.h"
#include "Menu.h"

void Menu::set_layout(menu_t *menu_layout) { layout = menu_layout; }

PGM_P Menu::get_name() { return layout->name; }

uint8_t Menu::get_number_of_items() {
  return pgm_read_byte(&(layout->number_of_items));
}

menu_item_t *Menu::get_item(uint8_t item_n) {
  if (item_n > get_number_of_items()) {
    return &(layout->items[get_number_of_items() - 1]);
  }
  return &(layout->items[item_n]);
}

PGM_P Menu::get_item_name(uint8_t item_n) {
  menu_item_t *item = get_item(item_n);
  return item->name;
}
Page *Menu::get_page_callback(uint8_t item_n) {
  menu_item_t *item = get_item(item_n);
  return pgm_read_word(&(item->page_callback));
}

uint8_t *Menu::get_dest_variable(uint8_t item_n) {
  menu_item_t *item = get_item(item_n);
  return pgm_read_word(&(item->destination_var));
}
uint8_t Menu::get_option_range(uint8_t item_n) {
  menu_item_t *item = get_item(item_n);
  return pgm_read_byte(&(item->range));
}

uint8_t Menu::get_number_of_options(uint8_t item_n) {
  menu_item_t *item = get_item(item_n);
  return pgm_read_byte(&(item->number_of_options));
}

PGM_P Menu::get_option_name(uint8_t item_n, uint8_t option_n) {
  menu_item_t *item = get_item(item_n);
  menu_option_t *option;
  uint8_t num_of_options = get_number_of_options(item_n);
  for (uint8_t a = 0; a < num_of_options; a++) {
    option = &(item->options[a]);
    if (pgm_read_byte(&(option->pos)) == option_n) {
      return option->name;
    }
  }
  return NULL;
}
