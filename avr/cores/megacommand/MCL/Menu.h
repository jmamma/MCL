#ifndef MENU_H__
#define MENU_H__

#define MAX_MENU_ITEMS 16
#define MAX_MENU_OPTIONS 16

typedef struct menu_option_s {
  uint8_t pos;
  char name[17];
} menu_option_t;

typedef struct menu_item_s {
  char name[17];
  uint8_t range;
  uint8_t number_of_options;
  uint8_t *destination_var; // pointer to variable to be updated by param change
  Page *page_callback;
  menu_option_t options[MAX_MENU_OPTIONS];
} menu_item_t;

typedef struct menu_s {
  char name[11];
  uint8_t number_of_items;
  menu_item_t items[MAX_MENU_ITEMS];
} menu_t;

class Menu {

public:
  menu_t *layout;
  uint8_t values[MAX_MENU_ITEMS];

  Menu() {}

  void set_layout(menu_t *menu_layout);

  PGM_P get_name();

  uint8_t get_number_of_items();
  menu_item_t *get_item(uint8_t item_n);
  PGM_P get_item_name(uint8_t item_n);
  Page *get_page_callback(uint8_t item_n);
  uint8_t *get_dest_variable(uint8_t item_n);
  uint8_t get_option_range(uint8_t item_n);
  uint8_t get_number_of_options(uint8_t item_n);
  PGM_P get_option_name(uint8_t item_n, uint8_t option_n);
};

#endif /* MENU_H__ */
