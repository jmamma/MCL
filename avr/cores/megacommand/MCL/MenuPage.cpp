
#include "MCL.h"
#include "MenuPage.h"

void MenuPage::init() {
  ((MCLEncoder *)encoders[1])->max = menu.get_number_of_items() - 1;
  if (((MCLEncoder *)encoders[1])->cur > ((MCLEncoder *)encoders[1])->max) {
   ((MCLEncoder *)encoders[1])->cur = 0;
  }
   ((MCLEncoder *)encoders[0])->max =
      menu.get_option_range(encoders[1]->cur) - 1;
  ((MCLEncoder *)encoders[0])->min = menu.get_option_min(encoders[1]->cur);

 uint8_t *dest_var = menu.get_dest_variable(encoders[1]->cur);
  if (dest_var != NULL) {
    encoders[0]->setValue(*dest_var);
  }
  encoders[0]->old = encoders[0]->cur;
  encoders[1]->old = encoders[1]->cur;
}
void MenuPage::setup() {
#ifdef OLED_DISPLAY
  classic_display = false;
#endif
}

void MenuPage::loop() {

    if (encoders[1]->hasChanged()) {
  ((MCLEncoder *)encoders[0])->max =
      menu.get_option_range(encoders[1]->cur) - 1;
  ((MCLEncoder *)encoders[0])->min = menu.get_option_min(encoders[1]->cur);


    uint8_t diff = encoders[1]->cur - encoders[1]->old;
    int8_t new_val = cur_row + diff;
    #ifdef OLED_DISPLAY
    if (new_val > MAX_VISIBLE_ROWS - 1) {
      new_val = MAX_VISIBLE_ROWS - 1;
    }
    if (new_val < 0) {
      new_val = 0;
    }
    #endif
    // MD.assignMachine(0, encoders[1]->cur);
    cur_row = new_val;
    uint8_t *dest_var = menu.get_dest_variable(encoders[1]->cur);
    if (dest_var != NULL) {
      encoders[0]->setValue(*dest_var);
    } else {
      encoders[0]->setValue(0);
    }
  }
  if (encoders[0]->hasChanged()) {
    uint8_t *dest_var = menu.get_dest_variable(encoders[1]->cur);
    if (dest_var != NULL) {
      *dest_var = encoders[0]->cur;
    }
  }
}

void MenuPage::draw_scrollbar(uint8_t x_offset) {
#ifdef OLED_DISPLAY
  uint8_t number_of_items = menu.get_number_of_items();
  uint8_t length =
      round(((float)(MAX_VISIBLE_ROWS - 1) / (float)(number_of_items - 1)) * 32);
  uint8_t y =
      round(((float)(encoders[1]->cur - cur_row) / (float)(number_of_items - 1)) * 32);
  for (uint8_t n = 0; n < 32; n++) {
    if (n % 2 == 0) {
      oled_display.drawPixel(x_offset + 1, n, WHITE);
      oled_display.drawPixel(x_offset + 3, n, WHITE);

    } else {
      oled_display.drawPixel(x_offset + 2, n, WHITE);
    }
  }

  oled_display.fillRect(x_offset + 1, y + 1, 3, length - 2, BLACK);
  oled_display.drawRect(x_offset, y, 5, length, WHITE);
#endif
}

void MenuPage::draw_item(uint8_t item_n, uint8_t row) {
#ifdef OLED_DISPLAY
  char str[17];
  PGM_P pgp = menu.get_item_name(item_n);
  if (pgp != NULL) {
    m_strncpy_p(str, pgp, 16);
    oled_display.print(str);
  }
  uint8_t number_of_items = menu.get_number_of_items();

  if (item_n > number_of_items - 1) {
    return true;
  }

  uint8_t number_of_options = menu.get_number_of_options(item_n);
  if (menu.get_option_range(item_n) > 0) {

    oled_display.print(" ");
    pgp = menu.get_option_name(item_n, *(menu.get_dest_variable(item_n)));
    if (pgp == NULL) {
      oled_display.println(*(menu.get_dest_variable(item_n)));
    } else {
      m_strncpy_p(str, pgp, 11);
      oled_display.println(str);
    }
  }
#endif
}

void MenuPage::draw_menu(uint8_t x_offset, uint8_t y_offset, uint8_t width) {
#ifdef OLED_DISPLAY
  oled_display.setCursor(x_offset, y_offset);
  uint8_t number_of_items = menu.get_number_of_items();
  uint8_t max_items;
  if (number_of_items > MAX_VISIBLE_ROWS) {
    max_items = MAX_VISIBLE_ROWS;
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

void MenuPage::display() {

  char str[17];
  PGM_P pgp;
  pgp = menu.get_name();

  m_strncpy_p(str, pgp, 16);


  uint8_t number_of_items = menu.get_number_of_items();
  #ifdef OLED_DISPLAY
  uint8_t x_offset = 43;
  oled_display.clearDisplay();
  oled_display.setTextColor(WHITE, BLACK);
  oled_display.setFont(&TomThumb);
  oled_display.setCursor(0, 8);
  oled_display.println(str);
  for (uint8_t n = 0; n < 32; n++) {
    if (n % 2 != 0) {
      oled_display.drawPixel(x_offset - 6, n, WHITE);
    }
  }

  draw_menu(x_offset, 8);

  if (number_of_items > MAX_VISIBLE_ROWS) {
  draw_scrollbar(120);
  }
  oled_display.display();

  #else
   GUI.setLine(GUI.LINE1);
   GUI.put_string_at(0,"[");
   GUI.put_string_at(1, str);

   GUI.put_string_at(m_strlen(str),"]");
   pgp = menu.get_item_name(cur_row);

   GUI.setLine(GUI.LINE2);
   if (pgp != NULL) {
    m_strncpy_p(str, pgp, 16);
    GUI.put_string_at_fill(0,str);
  }


  if (cur_row > number_of_items - 1) {
    return true;
  }

  uint8_t number_of_options = menu.get_number_of_options(cur_row);
  if (menu.get_option_range(cur_row) > 0) {

    pgp = menu.get_option_name(cur_row, *(menu.get_dest_variable(cur_row)));
    if (pgp == NULL) {
      GUI.put_value_at(10,*(menu.get_dest_variable(cur_row)));
    } else {
      m_strncpy_p(str, pgp, 11);
      GUI.put_string_at(10,str);
    }
  }

#endif
}

bool MenuPage::enter() {
    DEBUG_PRINT_FN();
    void (*row_func)() = menu.get_row_function(encoders[1]->cur);
    Page *page_callback = menu.get_page_callback(encoders[1]->cur);
    if (page_callback != NULL) {
      DEBUG_PRINTLN("setting page");
      DEBUG_PRINTLN((uint16_t)page_callback);
      GUI.pushPage(page_callback);
      return;
    }
    if (row_func != NULL) {
      DEBUG_PRINTLN("calling callback func");
      (*row_func)();
    }
 
}

bool MenuPage::exit() {
    // Page *exit_page_callback = menu.get_exit_page_callback();
    void (*exit_func)() = menu.get_exit_function();
    if (exit_func != NULL) {
      (*exit_func)();
      //
    }
    // if (exit_page_callback != NULL) {
    GUI.popPage();
    //}

}

bool MenuPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {

    return true;
  }
  if (EVENT_PRESSED(event, Buttons.ENCODER1) ||
      EVENT_PRESSED(event, Buttons.ENCODER2) ||
      EVENT_PRESSED(event, Buttons.ENCODER3) ||
      EVENT_PRESSED(event, Buttons.ENCODER4)) {
    enter();
    return true;
  }

  if (EVENT_PRESSED(event, Buttons.BUTTON1) ||
      EVENT_PRESSED(event, Buttons.BUTTON2) ||
      EVENT_PRESSED(event, Buttons.BUTTON3) ||
      EVENT_PRESSED(event, Buttons.BUTTON4)) {
    exit();
    return true;
  }
}
