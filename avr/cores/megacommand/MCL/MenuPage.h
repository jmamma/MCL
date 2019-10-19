/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef MENUPAGE_H__
#define MENUPAGE_H__

#include "GUI.h"
#include "Menu.h"

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

  MenuPageBase(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
               Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {}

  void draw_scrollbar(uint8_t x_offset);
  void draw_item(uint8_t item_n, uint8_t row);
  void draw_menu(uint8_t x_offset, uint8_t y_offset,
                 uint8_t width = MENU_WIDTH,
                 uint8_t rows = MAX_VISIBLE_ROWS);
  void loop();
  void display();
  void setup();
  void init();
  bool enter();
  bool exit();
  virtual bool handleEvent(gui_event_t *event);

protected:
  virtual MenuBase *get_menu() = 0;
};

template <int N> class MenuPage : public MenuPageBase {
public:
  Menu<N> menu;

  MenuPage(const menu_t<N> *layout, Encoder *e1 = NULL, Encoder *e2 = NULL,
           Encoder *e3 = NULL, Encoder *e4 = NULL)
      : MenuPageBase(e1, e2, e3, e4) {
    menu.set_layout(layout);
  }

protected:
  virtual MenuBase* get_menu() { return &menu; }
};

#endif /* MENUPAGE_H__ */
