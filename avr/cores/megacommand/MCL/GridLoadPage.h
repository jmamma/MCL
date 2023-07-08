/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef GRIDCHAINPAGE_H__
#define GRIDCHAINPAGE_H__

#include "GridIOPage.h"

class GridLoadPage : public GridIOPage {
 public:
 GridLoadPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL, Encoder *e4 = NULL) : GridIOPage(e1, e2, e3 ,e4) {

 }
 uint8_t track_select_array[NUM_SLOTS];
 uint8_t load_row;
 void get_modestr(char *modestr);
 void load();
 void group_select();
 void group_load(uint8_t row, bool silent = false);
 bool handleEvent(gui_event_t *event);
 void get_mode_str(char *str);
 void display_load();
 void draw_popup();
 void draw_popup_title();
 void display();
 void action() { load(); }
 void init();
 void setup();
};

#endif /* GRIDCHAINPAGE_H__ */
