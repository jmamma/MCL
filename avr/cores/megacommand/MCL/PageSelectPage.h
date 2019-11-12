/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef PageSelectPAGE_H__
#define PageSelectPAGE_H__

#include "GUI.h"

class PageSelectPage : public LightPage {
public:
  MCLEncoder enc1;
  MCLEncoder enc2;
  uint8_t page_select;
  PageSelectPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
                 Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {
    encoders[0] = &enc1;
    encoders[1] = &enc2;
  }

  virtual void display();
  virtual void setup();
  virtual void init();
  virtual void loop();
  virtual void cleanup();
  virtual bool handleEvent(gui_event_t *event);

  // get a page in the current category.
  uint8_t get_category_page(uint8_t offset);
  uint8_t get_nextpage_down();
  uint8_t get_nextpage_up();
  uint8_t get_nextpage_catup();
  uint8_t get_nextpage_catdown();
};

extern PageSelectPage page_select_page;

#endif /* PageSelectPAGE_H__ */
