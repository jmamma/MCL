#include "MCL.h"
#include "MenuPage.h"

void MenuPage::init() { oled_display.setFont(&TomThumb); }
void MenuPage::setup() {
#ifdef OLED_DISPLAY
  classic_display = false;
#endif
}

void MenuPage::loop() {
  if (encoders[0]->hasChanged()) {
    uint8_t diff = encoders[0]->cur - encoders[0]->old;
    int8_t new_val = cur_row + diff;

    if (new_val > MAX_VISIBLE_ROWS - 1) {
      new_val = MAX_VISIBLE_ROWS - 1;
    }
    if (new_val < 0) {
      new_val = 0;
    }

    // MD.assignMachine(0, encoders[1]->cur);
    cur_row = new_val;
    uint8_t *dest_var = menu.get_dest_variable(cur_row);
    if (dest_var != NULL) {
      encoders[1]->setValue(*dest_var);
    } else {
      encoders[1]->setValue(0);
    }
    ((MCLEncoder *)encoders[1])->max = menu.get_option_range(cur_row) - 1;

  } else if (encoders[1]->hasChanged()) {
    uint8_t *dest_var = menu.get_dest_variable(cur_row);
    if (dest_var != NULL) {
      *dest_var = encoders[1]->cur;
    }
  }
}

void MenuPage::draw_scrollbar(uint8_t x_offset) {
  uint8_t number_of_items = menu.get_number_of_items();
  uint8_t length =
      ((float)(MAX_VISIBLE_ROWS - 1) / (float)number_of_items) * 32;
  uint8_t y =
      ((float)(encoders[0]->cur - cur_row) / (float)number_of_items) * 32;
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
}

void MenuPage::draw_item(uint8_t item_n, uint8_t row) {
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

  if (number_of_options > 0) {
    oled_display.print(" ");

    pgp = menu.get_option_name(item_n, *(menu.get_dest_variable(item_n)));
    if (pgp == NULL) {
      oled_display.println(*(menu.get_dest_variable(item_n)));
    } else {
      m_strncpy_p(str, pgp, 11);
      oled_display.println(str);
    }
  }
}

void MenuPage::draw_menu(uint8_t x_offset, uint8_t y_offset) {
  oled_display.setCursor(x_offset, y_offset);
#ifdef OLED_DISPLAY
  for (uint8_t n = 0; n < MAX_VISIBLE_ROWS; n++) {

    oled_display.setCursor(x_offset, y_offset + 8 * n);
    if (n == cur_row) {
      oled_display.setTextColor(BLACK, WHITE);
      oled_display.fillRect(oled_display.getCursorX() - 3,
                            oled_display.getCursorY() - 6, 78, 7, WHITE);
    } else {
      oled_display.setTextColor(WHITE, BLACK);
    }
    draw_item(encoders[0]->cur - cur_row + n, n);
  }
#else

  // draw_item.read(getRow());

#endif

  oled_display.setTextColor(WHITE, BLACK);
}
void MenuPage::display() {

  uint8_t x_offset = 43;
  oled_display.clearDisplay();
  char str[17];
  PGM_P pgp;
  pgp = menu.get_name();

  m_strncpy_p(str, pgp, 16);

  oled_display.setCursor(0, 8);
  oled_display.println(str);
  for (uint8_t n = 0; n < 32; n++) {
    if (n % 2 != 0) {
      oled_display.drawPixel(x_offset - 6, n, WHITE);
    }
  }

  draw_menu(x_offset, 8);
  draw_scrollbar(120);
  oled_display.display();
}

bool MenuPage::handleEvent(gui_event_t *event) {
  if (note_interface.is_event(event)) {

    return true;
  }
  if (EVENT_PRESSED(event, Buttons.ENCODER1) ||
      EVENT_PRESSED(event, Buttons.ENCODER2) ||
      EVENT_PRESSED(event, Buttons.ENCODER3) ||
      EVENT_PRESSED(event, Buttons.ENCODER1)) {
    Page *page_callback = menu.get_page_callback(encoders[0]->cur);
    if (page_callback != NULL) {
      GUI.setPage(page_callback);
    }
    return true;
  }
}
