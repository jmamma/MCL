#ifndef MENU_H__
#define MENU_H__

#define MAX_MENU_ITEMS 16

typedef void (*FP)();

struct menu_option_t {
  uint8_t pos;
  char name[8];
};

struct menu_item_t {
  char name[14];
  uint8_t min;
  uint8_t range;
  uint8_t number_of_options;
  uint8_t *destination_var; // pointer to variable to be updated by param change
  Page *page_callback;
  void (*row_function)();
  uint8_t options_begin;
};

template <uint8_t N> struct menu_t {
  char name[10];
  menu_item_t items[N];
  void (*exit_function)();
  Page *exit_page_callback;
};

class MenuBase {
public:
  uint8_t entry_mask[4];

  MenuBase() { memset(entry_mask, 0xFF, sizeof(entry_mask)); }

  void enable_entry(uint8_t entry_index, bool en);
  bool is_entry_enable(uint8_t entry_index);

  uint8_t *get_dest_variable(uint8_t item_n);
  uint8_t get_option_min(uint8_t item_n);
  uint8_t get_option_range(uint8_t item_n);
  uint8_t get_number_of_options(uint8_t item_n);
  uint8_t get_options_offset(uint8_t item_n);
  LightPage *get_page_callback(uint8_t item_n);
  uint8_t get_number_of_items();
  const menu_item_t *get_item(uint8_t item_n);
  PGM_P get_item_name(uint8_t item_n);
  uint8_t get_item_index(uint8_t item_n);
  const char* get_option_name(uint8_t item_n, uint8_t option_n);
  FP get_row_function(uint8_t item_n);

  virtual PGM_P get_name() = 0;
  virtual FP get_exit_function() = 0;

protected:
  virtual const menu_item_t *get_entry_address(uint8_t) = 0;
  virtual uint8_t get_entry_count() = 0;
};

// TODO raise error if N > MAX_MENU_ITEMS
template <int N> class Menu : public MenuBase {

public:
  Menu() : MenuBase(){};
  const menu_t<N> *layout;

  void set_layout(const menu_t<N> *menu_layout) {
    layout = menu_layout;
  }
  virtual PGM_P get_name() { return layout->name; }
  virtual FP get_exit_function() {
    return (FP)pgm_read_word(&(layout->exit_function));
  }
  virtual const menu_item_t *get_entry_address(uint8_t i) { return layout->items + i; }
  virtual uint8_t get_entry_count() { return N; };
};

#endif /* MENU_H__ */
