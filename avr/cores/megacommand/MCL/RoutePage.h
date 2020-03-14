/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef ROUTEPAGE_H__
#define ROUTEPAGE_H__

#include "GUI.h"

class RoutePage : public LightPage {
 public:
 bool hasChanged = false;
 RoutePage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL, Encoder *e4 = NULL) : LightPage( e1, e2, e3 ,e4) {

 }

 char info_line2[9];

 virtual bool handleEvent(gui_event_t *event);
 virtual void display();
 virtual void setup();
 virtual void init();
 virtual void cleanup();

 void toggle_route(uint8_t i, uint8_t routing);
 void toggle_routes_batch(bool solo = false);
 void set_level(uint8_t curtrack, uint8_t value);
#ifndef OLED_DISPLAY
 void draw_routes(uint8_t line_number);
#else
 void draw_routes();
#endif
 void update_globals();
};
#endif /* ROUTEPAGE_H__ */
