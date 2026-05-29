/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MENUPAGE_H__
#define MENUPAGE_H__

#include "GUI.h"
#include "Menu.h"
#include "MCLGfx.h"
#include "MCLStrings.h"

// Forward declaration for mcl_print_P used in template
void mcl_print_P(const char* str_P);

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
  void draw_item(MenuBase *menu, uint8_t item_n);
  uint8_t draw_menu(uint8_t x_offset, uint8_t y_offset,
                    uint8_t width = MENU_WIDTH, uint8_t scrollbar_width = 0);
  void select_item(uint8_t item = 0) {
    cur_row = 0;
    selected_item = item;
    encoders[1]->cur = item;
  }
  void loop() override;
  void display() override;
  void init() override;
  void init(bool generate_row_names);
  bool enter();
  void exit();
  void cleanup() override;
  void gen_menu_device_names();
  bool handleEvent(gui_event_t *event) override;
#if defined(MCL_HAS_DESKTOP_MOUSE)
  virtual bool handleMouseEvent(mcl_mouse_event_t *event) override;
#endif

protected:
  uint8_t selected_item = 0;

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
    mcl_print_P(mclstr_hex_prefix);
    oled_display.print(checksum_value,HEX);
  }

};

#endif /* MENUPAGE_H__ */
