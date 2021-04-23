#include "MCL_impl.h"
#include "ResourceManager.h"

void MenuPageBase::init() {
  DEBUG_PRINTLN("MenuPageBase::init");
  R.Clear();
  R.use_machine_names_short(); // for grid page
  R.use_menu_options();
  R.use_menu_layouts();
  DEBUG_PRINT("R.Size() = ");
  DEBUG_PRINTLN(R.Size());
  R.restore_menu_layout_deps();
  ((MCLEncoder *)encoders[1])->max = get_menu()->get_number_of_items() - 1;
  if (((MCLEncoder *)encoders[1])->cur > ((MCLEncoder *)encoders[1])->max) {
    ((MCLEncoder *)encoders[1])->cur = 0;
  }
  ((MCLEncoder *)encoders[0])->max =
      get_menu()->get_option_range(encoders[1]->cur) - 1;
  ((MCLEncoder *)encoders[0])->min =
      get_menu()->get_option_min(encoders[1]->cur);

  uint8_t *dest_var = get_menu()->get_dest_variable(encoders[1]->cur);
  if (dest_var != NULL) {
    encoders[0]->setValue(*dest_var);
  }
  encoders[0]->old = encoders[0]->cur;
  encoders[1]->old = encoders[1]->cur;
}


void MenuPageBase::setup() {
#ifdef OLED_DISPLAY
  classic_display = false;
#endif
}

void MenuPageBase::loop() {

  if (encoders[1]->hasChanged()) {
    ((MCLEncoder *)encoders[0])->max =
        get_menu()->get_option_range(encoders[1]->cur) - 1;
    ((MCLEncoder *)encoders[0])->min =
        get_menu()->get_option_min(encoders[1]->cur);

    uint8_t diff = encoders[1]->cur - encoders[1]->old;
    int8_t new_val = cur_row + diff;
#ifdef OLED_DISPLAY
    if (new_val > visible_rows - 1) {
      new_val = visible_rows - 1;
    }
    if (new_val < 0) {
      new_val = 0;
    }
#endif
    // MD.assignMachine(0, encoders[1]->cur);
    cur_row = new_val;
    uint8_t *dest_var = get_menu()->get_dest_variable(encoders[1]->cur);
    if (dest_var != NULL) {
      encoders[0]->setValue(*dest_var);
    } else {
      encoders[0]->setValue(0);
    }
  }
  if (encoders[0]->hasChanged()) {
    uint8_t *dest_var = get_menu()->get_dest_variable(encoders[1]->cur);
    if (dest_var != NULL) {
      *dest_var = encoders[0]->cur;
    }
  }
}

void MenuPageBase::draw_scrollbar(uint8_t x_offset) {
#ifdef OLED_DISPLAY
  mcl_gui.draw_vertical_scrollbar(x_offset, get_menu()->get_number_of_items(),
                                  visible_rows, encoders[1]->cur - cur_row);
#endif
}

void MenuPageBase::draw_item(uint8_t item_n, uint8_t row) {
#ifdef OLED_DISPLAY
  const char* name = get_menu()->get_item_name(item_n);
  if (name != nullptr) {
    oled_display.print(name);
  }
  uint8_t number_of_items = get_menu()->get_number_of_items();

  if (item_n > number_of_items - 1) {
    return;
  }

  uint8_t number_of_options = get_menu()->get_number_of_options(item_n);
  if (get_menu()->get_option_range(item_n) > 0) {

    oled_display.print(" ");
    uint8_t *pdest = get_menu()->get_dest_variable(item_n);
    const char* option_name = get_menu()->get_option_name(item_n, *pdest);
    if (option_name == NULL) {
      oled_display.println(*pdest);
    } else {
      oled_display.println(option_name);
    }
  }
#endif
}

void MenuPageBase::draw_menu(uint8_t x_offset, uint8_t y_offset,
                             uint8_t width) {
#ifdef OLED_DISPLAY
  oled_display.setCursor(x_offset, y_offset);
  uint8_t number_of_items = get_menu()->get_number_of_items();
  uint8_t max_items;
  if (number_of_items > visible_rows) {
    max_items = visible_rows;
  } else {
    max_items = number_of_items;
  }
  for (uint8_t n = 0; n < max_items; n++) {

    oled_display.setCursor(x_offset, y_offset + 8 * n);
    if (n == cur_row) {
      oled_display.setTextColor(BLACK, WHITE);
      oled_display.fillRect(oled_display.getCursorX() - 3,
                            oled_display.getCursorY() - 6, width, 7, WHITE);
    } else {
      oled_display.setTextColor(WHITE, BLACK);
    }
    draw_item(encoders[1]->cur - cur_row + n, n);
  }

  // draw_item.read(getRow());

  oled_display.setTextColor(WHITE, BLACK);
#endif
}

void MenuPageBase::display() {

  uint8_t number_of_items = get_menu()->get_number_of_items();
#ifdef OLED_DISPLAY
  uint8_t x_offset = 43;
  oled_display.clearDisplay();
  oled_display.setTextColor(WHITE, BLACK);
  oled_display.setFont(&TomThumb);
  oled_display.setCursor(0, 8);
  oled_display.println(get_menu()->get_name());
  mcl_gui.draw_vertical_dashline(x_offset - 6);

  draw_menu(x_offset, 8);

  if (number_of_items > visible_rows) {
    draw_scrollbar(120);
  }
  oled_display.display();

#else
  GUI.setLine(GUI.LINE1);
  GUI.put_string_at(0, "[");
  GUI.put_string_at(1, str);

  GUI.put_string_at(m_strlen(str), "]");
  const char* item_name = get_menu()->get_item_name(cur_row);

  GUI.setLine(GUI.LINE2);
  if (item_name != NULL) {
    GUI.put_string_at_fill(0, item_name);
  }

  if (cur_row > number_of_items - 1) {
    return true;
  }

  uint8_t number_of_options = get_menu()->get_number_of_options(cur_row);
  if (get_menu()->get_option_range(cur_row) > 0) {

    const char* opt_name = get_menu()->get_option_name(
        cur_row, *(get_menu()->get_dest_variable(cur_row)));
    if (pgp == NULL) {
      GUI.put_value_at(10, *(get_menu()->get_dest_variable(cur_row)));
    } else {
      GUI.put_string_at(10, opt_name);
    }
  }

#endif
}

bool MenuPageBase::enter() {
  DEBUG_PRINT_FN();
  void (*row_func)() = get_menu()->get_row_function(encoders[1]->cur);
  LightPage *page_callback = get_menu()->get_page_callback(encoders[1]->cur);
  if (page_callback != NULL) {
    DEBUG_PRINTLN("setting page");
    DEBUG_PRINTLN((uint16_t)page_callback);
    GUI.pushPage(page_callback);
    return false;
  }
  if (row_func != NULL) {
    DEBUG_PRINTLN(F("calling callback func"));
    (*row_func)();
  }
}

bool MenuPageBase::exit() {
  // Page *exit_page_callback = get_menu()->get_exit_page_callback();
  void (*exit_func)() = get_menu()->get_exit_function();
  if (exit_func != NULL) {
    (*exit_func)();
    //
  }
  // if (exit_page_callback != NULL) {
  GUI.popPage();
  //}
}

bool MenuPageBase::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {

    return true;
  }
  if (EVENT_PRESSED(event, Buttons.BUTTON4)) {
    enter();
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON1)) {
    GUI.ignoreNextEvent(event->source);
    exit();
    return true;
  }
}
