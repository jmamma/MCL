/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef GRIDLOADPAGE_H__
#define GRIDLOADPAGE_H__

#include "GridIOPage.h"

#define LOAD_PAGE 0
#define CHAIN_PAGE 1

class GridLoadPage : public GridIOPage {
 public:
 GridLoadPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL, Encoder *e4 = NULL) : GridIOPage(e1, e2, e3 ,e4) {

 }
 void get_modestr(char *modestr);
 void load();
 void group_select();
 void group_load(uint8_t row);
 bool handleEvent(gui_event_t *event);
 void get_mode_str(char *str);
 void draw_popup();
 void draw_popup_title();
 void display();
 void init();
 void setup();
};

#endif /* GRIDLOADPAGE_H__ */
