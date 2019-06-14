/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef ROUTEPAGE_H__
#define ROUTEPAGE_H__

#include "GUI.h"

class RoutePage : public LightPage {
 public:
 bool hasChanged = false;
 RoutePage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL, Encoder *e4 = NULL) : LightPage( e1, e2, e3 ,e4) {

 }
 void toggle_route(int i, uint8_t routing);
 void toggle_routes_batch();
 void set_level(int curtrack, int value);
 void draw_routes(uint8_t line_number);
 bool handleEvent(gui_event_t *event);
 void display();
 void setup();
 void init();
 void update_globals();
 void cleanup();
};
#endif /* ROUTEPAGE_H__ */
