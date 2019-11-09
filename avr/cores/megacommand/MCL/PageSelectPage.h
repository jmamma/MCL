/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef PageSelectPAGE_H__
#define PageSelectPAGE_H__

#include "GUI.h"

class PageSelectPage : public LightPage {
 public:
 MCLEncoder enc1;
 uint8_t page_select;
 PageSelectPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL, Encoder *e4 = NULL) : LightPage( e1, e2, e3 ,e4) {
   encoders[0] = &enc1;
 }
 void display();
 void setup();
 void init();
 void loop();
 void cleanup();
 LightPage *get_page(uint8_t page_number, char *str = NULL);
 uint8_t get_nextpage_down();
 uint8_t get_nextpage_up();
 virtual bool handleEvent(gui_event_t *event);
};

extern PageSelectPage page_select_page;

#endif /* PageSelectPAGE_H__ */
