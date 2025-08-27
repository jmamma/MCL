/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MENUPAGE_H__
#define MENUPAGE_H__

#include "GUI.h"
#include "Menu.h"
#include "MCLGfx.h"

#ifdef OLED_DISPLAY
#define MAX_VISIBLE_ROWS 4
#else
#define MAX_VISIBLE_ROWS 1
#endif

#define MENU_WIDTH 78

class MenuPageBase : public LightPage {
public:
  uint8_t cur_col = 0;
  uint8_t cur_row = 0;

  uint8_t visible_rows = MAX_VISIBLE_ROWS;

  MenuPageBase(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
               Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {}

  void draw_scrollbar(uint8_t x_offset);
  void draw_item(uint8_t item_n, uint8_t row);
  void draw_menu(uint8_t x_offset, uint8_t y_offset,
                 uint8_t width = MENU_WIDTH);
  void select_item(uint8_t item = 0) {
  cur_row = 0;
  encoders[1]->cur = 0;
  encoders[1]->old = 0;
  }
  void loop();
  virtual void display();
  void setup();
  void init();
  bool enter();
  void exit();
  void cleanup();
  void gen_menu_device_names();
  void gen_menu_row_names();
  void gen_menu_transpose_names();
  virtual bool handleEvent(gui_event_t *event);

protected:
  virtual MenuBase *get_menu() = 0;
};

template <int N> class MenuPage : public MenuPageBase {
public:
  Menu<N> menu;

  MenuPage(Encoder *e1 = NULL, Encoder *e2 = NULL,
           Encoder *e3 = NULL, Encoder *e4 = NULL)
      : MenuPageBase(e1, e2, e3, e4) {
  }

protected:
  virtual MenuBase* get_menu() { return &menu; }
public:
  void set_layout(menu_t<N>* layout) {
    menu.set_layout(layout);
  }
};

template <int N> class BootMenuPage : public MenuPage<N> {
public:

  BootMenuPage(Encoder *e1 = NULL, Encoder *e2 = NULL,
           Encoder *e3 = NULL, Encoder *e4 = NULL)
      : MenuPage<N>(e1, e2, e3, e4) {
  }
  virtual void display() {
    MenuPage<N>::display();
    oled_display.setCursor(0, 25);
    oled_display.setTextSize(1);
    oled_display.print(VERSION_STR);
    oled_display.setCursor(0, 32);
  //uint32_t checksum_addr = (uint32_t)&firmware_checksum;
#if defined(__AVR__)
    uint32_t checksum_addr = pgm_get_far_address(firmware_checksum);
    uint16_t checksum_value = pgm_read_word_far(checksum_addr);
#else
    uint16_t checksum_value = firmware_checksum;
#endif
    oled_display.print("0x");
    oled_display.print(checksum_value,HEX);
  }

};

#endif /* MENUPAGE_H__ */
